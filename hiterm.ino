#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include "src/host.h"
#include "src/telnet.h"
#include "src/terminal.h"
#include "src/parser.h"
#include "src/eeprom.h"
#include "src/command.h"
#include "src/serial.h"

const String TITLE = "HITERM 0.1";
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

	init_serial();
	
	Serial.printf("\r\n\r\n%s\r\n", TITLE.c_str());

	String wifi_ssid = read_eeprom(EEPROM_SYS1_ADDR);

	if (wifi_ssid != "") {
		String wifi_passphrase = read_eeprom(EEPROM_SYS2_ADDR);
		Serial.println("CONNECTING");
		Serial.printf("%s\r\n", wifi_ssid.c_str());  
		WiFi.begin(wifi_ssid, wifi_passphrase);
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

	Serial.printf("\r\nType 'help' for commands\r\n");
}

void loop() {
	if (!connected_to_host)
		command();
	if (g_host->connected() && !connected_to_host) {
		connected_to_host = true;
		g_terminal->reset();
		parser_init();
	}
	if (!g_host->connected() && connected_to_host) {
		connected_to_host = false;
		g_host->shutdown();
		Serial.println();
		Serial.printf("\nConnection closed.\n");
	}
	if (g_terminal->available()) {
		g_host->send(g_terminal->get());
	}
	if (g_host->available()) {
		char c = g_host->get();
		if (!telnet_char(c)) parse_char(c);
	}
}
