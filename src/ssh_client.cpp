#include "ssh_client.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <SPIFFS.h>

static bool _libssh_initialized = false;
static bool _spiffs_initialized = false;

// SSH connection task - runs with larger stack for LibSSH
void ssh_connect_task(void* pvParameters) {
    SSHClient* client = (SSHClient*)pvParameters;
    client->do_connect_sync();
    vTaskDelete(NULL);
}

SSHClient::SSHClient() {
}

SSHClient::~SSHClient() {
	stop();
}

void SSHClient::set_credentials(const char* user, const char* password) {
	_username = user;
	_password = password ? password : "";
	_use_key_auth = false;
}

void SSHClient::set_key_file(const char* keyFilePath) {
	_key_file_path = keyFilePath;
	_use_key_auth = true;
}

bool SSHClient::key_file_exists(const char* path) {
	if (!_spiffs_initialized) {
		if (!SPIFFS.begin(true)) {
			return false;
		}
		_spiffs_initialized = true;
	}
	return SPIFFS.exists(path);
}

int SSHClient::connect(IPAddress ip, uint16_t port) {
	return connect(ip.toString().c_str(), port);
}

// Cleanup helper - resets connection state
void SSHClient::cleanup() {
	if (channel) {
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		channel = nullptr;
	}
	if (ssh) {
		ssh_disconnect(ssh);
		ssh_free(ssh);
		ssh = nullptr;
	}
}

// Key authentication helper - returns true on success
bool SSHClient::auth_with_key() {
	if (!SPIFFS.begin(true))
		return false;

	File key_file = SPIFFS.open(_key_file_path.c_str(), "r");
	if (!key_file)
		return false;

	size_t key_size = key_file.size();
	char* key_buffer = (char*)malloc(key_size + 1);
	if (!key_buffer) {
		key_file.close();
		return false;
	}

	key_file.readBytes(key_buffer, key_size);
	key_buffer[key_size] = '\0';
	key_file.close();

	ssh_key privkey = nullptr;
	int rc = ssh_pki_import_privkey_base64(key_buffer, nullptr, nullptr, nullptr, &privkey);
	free(key_buffer);

	if (rc != SSH_OK)
		return false;

	rc = ssh_userauth_publickey(ssh, nullptr, privkey);
	ssh_key_free(privkey);

	return rc == SSH_AUTH_SUCCESS;
}

void SSHClient::do_connect_sync() {
	if (!_libssh_initialized) {
		libssh_begin();
		_libssh_initialized = true;
	}

	ssh = ssh_new();
	if (!ssh)
		return;

	// Set options
	int port_int = _connect_port;
	int timeout_sec = 10;
	ssh_options_set(ssh, SSH_OPTIONS_HOST, _connect_host.c_str());
	ssh_options_set(ssh, SSH_OPTIONS_PORT, &port_int);
	ssh_options_set(ssh, SSH_OPTIONS_USER, _username.c_str());
	ssh_options_set(ssh, SSH_OPTIONS_TIMEOUT, &timeout_sec);

	// Connect
	if (ssh_connect(ssh) != SSH_OK) {
		cleanup();
		return;
	}

	// Authenticate
	bool auth_ok = _use_key_auth
		? auth_with_key()
		: ssh_userauth_password(ssh, nullptr, _password.c_str()) == SSH_AUTH_SUCCESS;

	if (!auth_ok) {
		cleanup();
		return;
	}

	// Open channel
	channel = ssh_channel_new(ssh);
	if (!channel || ssh_channel_open_session(channel) != SSH_OK) {
		cleanup();
		return;
	}

	// Request PTY with terminal type and size
	String term_type = g_terminal_type;
	if (term_type.endsWith("-ansi"))
		term_type = term_type.substring(0, term_type.length() - 5);

	if (ssh_channel_request_pty_size(channel, term_type.c_str(), g_terminal_cols, g_terminal_rows) != SSH_OK) {
		cleanup();
		return;
	}

	// Request login shell
	if (ssh_channel_request_shell(channel) != SSH_OK) {
		cleanup();
		return;
	}

	_connected = true;
}

int SSHClient::connect(const char* host, uint16_t port) {
	if (_connected)
		return 0;

	_connect_host = host;
	_connect_port = port;

	// Create task with large stack for SSH operations (LibSSH-ESP32 requires ~51KB)
	TaskHandle_t task_handle;
	BaseType_t result = xTaskCreatePinnedToCore(
		ssh_connect_task, "ssh_connect", 51200,
		this, tskIDLE_PRIORITY + 2, &task_handle, 1
	);

	if (result != pdPASS)
		return 0;

	// Wait for task to complete
	int initial_task_count = uxTaskGetNumberOfTasks();
	while (uxTaskGetNumberOfTasks() >= initial_task_count)
		vTaskDelay(100 / portTICK_PERIOD_MS);

	return _connected ? 1 : 0;
}

size_t SSHClient::write(uint8_t b) {
	return write(&b, 1);
}

size_t SSHClient::write(const uint8_t* buf, size_t size) {
	if (!_connected || !channel)
		return 0;
	if (!ssh_channel_is_open(channel)) {
		_connected = false;
		return 0;
	}
	int written = ssh_channel_write(channel, buf, size);
	return (written > 0) ? written : 0;
}

int SSHClient::available() {
	if (!_connected || !channel)
		return 0;
	if (_peek_byte >= 0)
		return 1;
	if (!ssh_channel_is_open(channel) || ssh_channel_is_eof(channel)) {
		_connected = false;
		return 0;
	}
	int avail = ssh_channel_poll(channel, 0);
	return (avail > 0) ? avail : 0;
}

int SSHClient::read() {
	if (!_connected || !channel)
		return -1;

	if (_peek_byte >= 0) {
		int b = _peek_byte;
		_peek_byte = -1;
		return b;
	}

	if (!ssh_channel_is_open(channel)) {
		_connected = false;
		return -1;
	}

	char buf[1];
	int nbytes = ssh_channel_read_nonblocking(channel, buf, 1, 0);
	return (nbytes > 0) ? (uint8_t)buf[0] : -1;
}

int SSHClient::read(uint8_t* buf, size_t size) {
	if (!_connected || !channel)
		return -1;

	size_t offset = 0;
	if (_peek_byte >= 0 && size > 0) {
		buf[0] = _peek_byte;
		_peek_byte = -1;
		offset = 1;
		size--;
	}

	if (size > 0) {
		int nbytes = ssh_channel_read_nonblocking(channel, buf + offset, size, 0);
		if (nbytes > 0)
			return nbytes + offset;
	}
	return offset > 0 ? offset : -1;
}

int SSHClient::peek() {
	if (_peek_byte >= 0)
		return _peek_byte;
	_peek_byte = read();
	return _peek_byte;
}

void SSHClient::flush() {
}

void SSHClient::stop() {
	if (channel) {
		ssh_channel_send_eof(channel);
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		channel = nullptr;
	}
	if (ssh) {
		ssh_disconnect(ssh);
		ssh_free(ssh);
		ssh = nullptr;
	}
	_connected = false;
	_peek_byte = -1;
}

uint8_t SSHClient::connected() {
	if (!_connected || !channel)
		return 0;
	return ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel);
}

SSHClient::operator bool() {
	return connected();
}
