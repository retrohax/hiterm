#include <Arduino.h>
#include "host.h"
#include "command.h"
#include "eeprom.h"
#include "term_base.h"

static void send_str(String str);

TERM_BASE::TERM_BASE() {}

bool TERM_BASE::available() { return Serial.available(); }

void TERM_BASE::print(char c) {
	Serial.print(c);
}

char TERM_BASE::read() {
	char c, u;
	c = Serial.read();
	switch (c) {
		case '\021':
			// ^Q (XON)
			g_host->set_flow_mode(1);
			break;
		case '\023':
			// ^S (XOFF)
			g_host->set_flow_mode(0);
			break;
		case '\035':
			// ^]
			command();
			break;
		case '\037':
			// ^_
			while (!Serial.available()) yield();
			u = Serial.read();
			if (u >= '0' && u < '0' + EEPROM_USR_COUNT) {
				// Send USRn string from EEPROM
				send_str(read_eeprom(EEPROM_USR_ADDR(u - '0')));
			}
			break;
		default:
			return c;
			break;
	}
	return '\0';
}

static void send_str(String str) {
	for (int i=0; i<str.length(); i++)
		g_host->send(str[i]);
}
