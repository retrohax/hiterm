#include <Arduino.h>
#include "host.h"
#include "command.h"
#include "eeprom.h"
#include "term_base.h"

static void send_str(String str);

TERM_BASE::TERM_BASE() {}

bool TERM_BASE::available() { return Serial.available(); }
void TERM_BASE::print(char c) { Serial.print(c); }

char TERM_BASE::read() {
	char c = Serial.read();
	switch (c) {
		case '\001':
			// ^A (send USR1 string from EEPROM)
			send_str(read_eeprom(EEPROM_USR1_ADDR));
			break;
		case '\002':
			// ^B (send USR2 string from EEPROM)
			send_str(read_eeprom(EEPROM_USR2_ADDR));
			break;
		case '\005':
			// ^E (ECHO)
			g_host->toggle_local_echo();
			break;
		case '\021':
			// ^Q (XON)
			g_host->set_flow_mode(1);
			break;
		case '\023':
			// ^S (XOFF)
			g_host->set_flow_mode(0);
			break;
		case '\030':
			// ^X
            //m_send_ansi_mode = !m_send_ansi_mode;
			break;
		case '\034':
			/* ^\ */
			g_host->shutdown();
		case '\035':
			// ^]
			command();
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
