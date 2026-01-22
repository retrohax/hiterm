#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include "src/host.h"
#include "src/eeprom.h"
#include "src/command.h"
#include "src/serial.h"
#include "src/term_telnet.h"
#include "src/terminal.h"
#include "src/ssh_client.h"

#define LED_BUILTIN 2

// ESP32 pins for serial communication
// Use UART2, reserve UART0 for USB updates
// UART0 default pins are RX=3, TX=1
// UART2 default pins are RX=16, TX=17
#define SERIAL_RX 3 
#define SERIAL_TX 1

const String TITLE = "HITERM 0.3";
const String CMD_PROMPT = "hiterm> ";
bool connected_to_host = false;

void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	EEPROM.begin(EEPROM_LEN);
	if (EEPROM.read(EEPROM_FLAG_ADDR) != 1) {
		write_eeprom(EEPROM_SERI_ADDR, String(1200));
		write_eeprom(EEPROM_SYS1_ADDR, "");
		write_eeprom(EEPROM_SYS2_ADDR, "");
		write_eeprom(EEPROM_USR1_ADDR, "");
		write_eeprom(EEPROM_USR2_ADDR, "");
		EEPROM.write(EEPROM_FLAG_ADDR, 1);
		EEPROM.commit();
	}

	init_serial(SERIAL_RX, SERIAL_TX);
	
	Serial.printf("\r\n\r\n%s\r\n", TITLE.c_str());

	String wifi_ssid = read_eeprom(EEPROM_SYS1_ADDR);

	if (wifi_ssid != "") {
		String wifi_passphrase = read_eeprom(EEPROM_SYS2_ADDR);
		Serial.println("CONNECTING");
		Serial.printf("%s\r\n", wifi_ssid.c_str());
		WiFi.setSleep(false);  // Disable WiFi power save for stable SSH
		WiFi.begin(wifi_ssid.c_str(), wifi_passphrase.c_str());
		for (int i=0; i<60; i++) {
			if (WiFi.status() == WL_CONNECTED)
				break;
			delay(500);
			Serial.print(".");
		}
		Serial.println();
	}

	if (WiFi.status() == WL_CONNECTED) {
		Serial.print("IP ADDRESS: ");
		Serial.println(WiFi.localIP());
	} else {
		Serial.println("WIFI NOT CONNECTED");
	}

	init_terminal(CONN_NONE);
	Serial.printf("\r\nTerminal type is none (raw)\r\n");
	Serial.printf("\r\nType 'help' for commands\r\n");
}

void loop() {
	if (!connected_to_host)
		command();
	if (g_host->connected() && !connected_to_host) {
		connected_to_host = true;
		init_terminal(g_host->get_connection_type());
		g_terminal->reset();
	}
	if (!g_host->connected() && connected_to_host) {
		connected_to_host = false;
		g_host->shutdown();
		Serial.println();
		Serial.printf("\nConnection closed.\n");
		init_terminal(CONN_NONE);
	}
	g_host->keepalive();
	if (g_terminal->available()) {
		g_host->send(g_terminal->read());
	}
	if (g_host->available()) {
		g_terminal->print(g_host->get());
	}

}
