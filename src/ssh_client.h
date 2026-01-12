#pragma once
#include <Arduino.h>
#include <Client.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <libssh_esp32.h>
#include <libssh/libssh.h>
#include "terminal.h"

class SSHClient : public Client {
	// Allow the SSH connect task to access private members
	friend void ssh_connect_task(void* pvParameters);
	
public:
	SSHClient();
	virtual ~SSHClient();

	// Set credentials before calling connect()
	// For password auth: set_credentials(user, password)
	// For key auth: set_credentials(user, nullptr) then set_key_file(path)
	void set_credentials(const char* user, const char* password);
	void set_key_file(const char* keyFilePath);
	
	// Check if key file exists
	static bool key_file_exists(const char* path = "/ssh_key.pem");

	// Required Client interface methods
	int connect(IPAddress ip, uint16_t port) override;
	int connect(const char* host, uint16_t port) override;
	size_t write(uint8_t b) override;
	size_t write(const uint8_t* buf, size_t size) override;

	int available() override;
	int read() override;
	int read(uint8_t* buf, size_t size) override;
	int peek() override;
	void flush() override;
	void stop() override;
	uint8_t connected() override;
	operator bool() override;

private:
	ssh_session ssh = nullptr;
	ssh_channel channel = nullptr;
	String _username;
	String _password;
	String _key_file_path;
	bool _use_key_auth = false;
	bool _connected = false;
	int _peek_byte = -1;  // For peek() implementation

	// Connection parameters for task
	String _connect_host;
	uint16_t _connect_port;

	// Synchronous connection method called by task
	void do_connect_sync();
	void cleanup();
	bool auth_with_key();
};

