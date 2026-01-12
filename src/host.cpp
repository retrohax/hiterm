#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include "eeprom.h"
#include "host.h"
#include "ssh_client.h"

Host *g_host = new Host();

Host :: Host() {
	g_tx_hist_index = 0;
	g_rx_hist_index = 0;
	for (int i=0; i<TX_HIST_MAXLEN; i++)
		g_tx_hist[i] = '\0';
	for (int i=0; i<RX_HIST_MAXLEN; i++)
		g_rx_hist[i] = '\0';	
}

void Host::connect(String host, int port, bool use_tls) {
	if (client && client->connected())
		Serial.printf("Already connected.\r\n");
	else {
		host_name = host;
		IPAddress ip;
		if(WiFi.hostByName(host_name.c_str(), ip)) {
			Serial.printf("Trying %s...\r\n", ip.toString().c_str());
			delete client;
			if (use_tls) {
				WiFiClientSecure *client_tls = new WiFiClientSecure();
				client_tls->setInsecure();
				client = client_tls;
			} else {
				client = new WiFiClient();
			}
			if (client->connect(ip, port)) {
				conn_type = CONN_ESTABLISHED;
				update_data_received_time();  // Initialize timing on connection
				Serial.printf("Connected to %s.\r\n", host_name.c_str());
				Serial.printf("Escape character is '^]'.\r\n");
				delay(1000);
			} else {
				Serial.printf("Could not connect to %s.\r\n", host_name.c_str());
				delete client;
				client = nullptr;
			}
		}
		else {
			Serial.printf("Could not resolve %s\r\n", host_name.c_str());
		}
	}
}

void Host::connect_ssh(String host, int port, String user, String password) {
	if (client && client->connected()) {
		Serial.printf("Already connected.\r\n");
		return;
	}

	host_name = host;
	
	delete client;
	SSHClient* ssh_client = new SSHClient();
	
	// If no password provided, try key authentication
	if (password.isEmpty()) {
		const char* keyPath = "/ssh_key.pem";
		if (!SSHClient::key_file_exists(keyPath)) {
			Serial.println("No SSH key found. Upload ssh_key.pem to SPIFFS.");
			delete ssh_client;
			return;
		}
		Serial.printf("Trying %s:%d (SSH key auth)...\r\n", host.c_str(), port);
		ssh_client->set_credentials(user.c_str(), nullptr);
		ssh_client->set_key_file(keyPath);
	} else {
		Serial.printf("Trying %s:%d (SSH password auth)...\r\n", host.c_str(), port);
		ssh_client->set_credentials(user.c_str(), password.c_str());
	}
	
	client = ssh_client;

	if (client->connect(host.c_str(), port)) {
		conn_type = CONN_SSH;
		update_data_received_time();  // Initialize timing on connection
		Serial.printf("Connected to %s (SSH).\r\n", host_name.c_str());
		Serial.printf("Escape character is '^]'.\r\n");
		// SSH/Unix just needs \r, the PTY handles the conversion
		line_end = "\r";
	} else {
		Serial.printf("Could not connect to %s.\r\n", host_name.c_str());
		delete client;
		client = nullptr;
	}
}

bool Host::connected() {
	return (client && client->connected());
}

bool Host::is_ssh_connection() {
	return (conn_type == CONN_SSH);
}

void Host::keepalive() {
    // Check if connected and it's an SSH connection (not telnet)
    if (connected() && client && is_ssh_connection()) {
        if (millis() - get_last_data_received_time() >= 60000) {  // 60 seconds
            client->write((uint8_t)0x00);
            update_data_received_time();  // Reset timer after sending keepalive
        }
    }
}

bool Host::available() {
	if (!client) return false;
	if (!client->connected()) {
		Serial.println("[Host:client disconnected]");
		return false;
	}
	if (flow_mode != 1) {
		// Don't spam - flow_mode 0 means XOFF (Ctrl+S was pressed)
		return false;
	}
	return client->available();
}

void Host::shutdown() {
	if (client) {
		if (client->connected())
			client->stop();
		delete client;
		client = nullptr;
	}
	conn_type = CONN_NONE;
}

char Host::get() {
	char c = read();
	return c;
}

void Host::send(char c, bool send_ansi_sequence) {
	if (c == '\0') return;
	if (send_ansi_sequence) {
		String ansi_str = "";
		switch (c) {
			case 'h': ansi_str = "\033[D"; break;	// left
			case 'j': ansi_str = "\033[B"; break;	// down
			case 'k': ansi_str = "\033[A"; break;	// up
			case 'l': ansi_str = "\033[C"; break;	// right
		}
		for (int i=0; i<ansi_str.length(); i++)
			write(ansi_str[i]);
	} else {
		if (local_echo) {
			Serial.print(c);
			if (c == '\r') Serial.print('\n');
		}
		if (c == '\r')
			for (int i=0; line_end[i]!='\0'; i++)
				write(line_end[i]);
		else
			write(c);
	}
}

char Host::read() {
	char c = client->read();
	save_rx_char(c);
	update_data_received_time();  // Track when data was received
	return c;
}

void Host::write(char c) {
	client->write(c);
	save_tx_char(c);
}

void Host::write_buffer(const char* data, int len) {
	client->write((const uint8_t*)data, len);
}

void Host::writef(const char* format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	for (int i = 0; buffer[i] != '\0'; i++)
		write(buffer[i]);
}

void Host::show_local_echo() {
	Serial.printf("Local echo is %s.\r\n", local_echo ? "on" : "off");	
}

void Host::set_local_echo(bool echo) {
	local_echo = echo;
}

void Host::toggle_local_echo() {
	local_echo = !local_echo;
}

void Host::set_flow_mode(int n) {
	flow_mode = n;
}


void Host::show_crlf() {
	if (line_end == "\r\n")
		Serial.println("Carriage returns sent as telnet <CR><LF>.");
	else
		Serial.println("Carriage returns sent as telnet <CR><NUL>.");
}

void Host::show_status() {
	if (connected()) {
		Serial.printf("Connected to %s.\r\n", host_name.c_str());
		show_local_echo();
		show_crlf();
		Serial.printf("Escape character is '^]'.\r\n");
	} else {
		Serial.printf("No connection.\r\n");
		Serial.printf("Escape character is '^]'.\r\n");
	}
}

void Host::toggle_crlf() {
	line_end = (line_end == "\r\n") ? "\r\0" : "\r\n";
}

void Host::save_rx_char(char c) {
	g_rx_hist[g_rx_hist_index++] = c;
	if (g_rx_hist_index > RX_HIST_MAXLEN-1)
		g_rx_hist_index = 0;
}

void Host::save_tx_char(char c) {
	g_tx_hist[g_tx_hist_index++] = c;
	if (g_tx_hist_index > TX_HIST_MAXLEN-1)
		g_tx_hist_index = 0;
}

void Host::show_rx_hist() {
	for (int i=0; i<RX_HIST_MAXLEN; i++) {
		g_rx_hist_index++;
		if (g_rx_hist_index > RX_HIST_MAXLEN-1)
			g_rx_hist_index = 0;
		if (i > 0 && (i % 26) == 0)
			Serial.println();
		if (g_rx_hist[g_rx_hist_index] == '\0')
			Serial.print(".. ");
		else
			Serial.printf("%02x ", g_rx_hist[g_rx_hist_index]);
		yield();
	}
	Serial.println();
}

void Host::show_tx_hist() {
	for (int i=0; i<TX_HIST_MAXLEN; i++) {
		g_tx_hist_index++;
		if (g_tx_hist_index > TX_HIST_MAXLEN-1)
			g_tx_hist_index = 0;
		if (i > 0 && (i % 26) == 0)
				Serial.println();
		if (g_tx_hist[g_tx_hist_index] == '\0')
			Serial.print(".. ");
		else
			Serial.printf("%02x ", g_tx_hist[g_tx_hist_index]);
		yield();
	}
	Serial.println();
}
