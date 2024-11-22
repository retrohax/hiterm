#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include "eeprom.h"
#include "host.h"

Host *g_host = new Host();

Host :: Host() {
	g_tx_hist_index = 0;
	g_rx_hist_index = 0;
	for (int i=0; i<TX_HIST_MAXLEN; i++)
		g_tx_hist[i] = '\0';
	for (int i=0; i<RX_HIST_MAXLEN; i++)
		g_rx_hist[i] = '\0';	
}

Host::~Host() {
}

void Host::tcp_connect(String host, int port) {
	if (client && client->connected())
		Serial.println("ALREADY CONNECTED");
	else {
		Serial.printf("host=%s, port=%d\r\n", host.c_str(), port);
		delete client;
		client = new WiFiClient();
		if (!client->connect(host.c_str(), port)) {
			Serial.println("ERROR");
			delete client;
			client = nullptr;
		}
	}
}

void Host::tls_connect(String host, int port) {
	if (client && client->connected())
		Serial.println("ALREADY CONNECTED");
	else {
		Serial.printf("host=%s, port=%d\r\n", host.c_str(), port);
		delete client;
		WiFiClientSecure *client_tls = new WiFiClientSecure();
		client_tls->setInsecure();
		client = client_tls;
		if (!client->connect(host.c_str(), port)) {
			Serial.println("ERROR");
			delete client;
			client = nullptr;
		}
	}
}

bool Host::connected() {
	return (client && client->connected());
}

bool Host::available() {
	return (client && client->connected() && client->available() && flow_mode == 1);
}

void Host::shutdown() {
	if (client) {
		if (client->connected())
			client->stop();
		delete client;
		client = nullptr;
	}
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
	return c;
}

void Host::write(char c) {
	client->write(c);
	save_tx_char(c);
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

void Host::set_local_echo(String str, bool quiet) {
	str.toUpperCase();
	if (str == "ON") {
		local_echo = true;
		if (!quiet) Serial.println("ECHO=ON");
	} else if (str == "OFF") {
		local_echo = false;
		if (!quiet) Serial.println("ECHO=OFF");
	} else {
		if (!quiet) Serial.println("ERROR");
	}
}

void Host::set_flow_mode(int n) {
	flow_mode = n;
}

void Host::toggle_crlf() {
	if (line_end == "\r\n") {
		line_end = "\r\0";
		Serial.println("CRLF=<CR><NUL>");
	}
	else {
		line_end = "\r\n";
		Serial.println("CRLF=<CR><LF>");
	}
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

void Host::show_rx_hist(String find_str) {
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

void Host::save_rx_hist() {
	for (int i=0; i<RX_HIST_MAXLEN; i++) {
		g_rx_hist_index++;
		if (g_rx_hist_index > RX_HIST_MAXLEN-1)
			g_rx_hist_index = 0;
		EEPROM.write(EEPROM_RX_DATA_ADDR+i, g_rx_hist[g_rx_hist_index]);
	}
	EEPROM.commit();
}

void Host::replay_rx_hist() {
	char c;
	Serial.printf("\032");
	for (int i=0; i<RX_HIST_MAXLEN; i++) {
		c = EEPROM.read(EEPROM_RX_DATA_ADDR+i);
		if (c != '\0')
			Serial.printf("%c", c);
		yield();
	}
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
