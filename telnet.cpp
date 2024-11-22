#include "telnet.h"
#include "host.h"
#include "terminal.h"
#include "utility.h"

bool g_debug_telnet = false;

bool g_telnet_reading = false;
bool g_telnet_writing = false;

char telnet_read() {
	char c = g_host->read();
	if (g_debug_telnet) {
		if (g_telnet_writing) {
			Serial.print("\r\n");
			g_telnet_writing = false;
		}
		if (!g_telnet_reading)
			Serial.printf("< %d %d ", (char)telnet_verbs::IAC, c);
		else
			Serial.printf("%d ", c);
		g_telnet_reading = true;
	}
	return c;
}

void telnet_write(char c, char debug='\0') {
	if (debug == '\0')
		g_host->write(c);
	if (g_debug_telnet) {
		if (g_telnet_reading) {
			Serial.print("\r\n");
			g_telnet_reading = false;
		}
		if (!g_telnet_writing)
			Serial.printf("> %d ", c);
		else
			Serial.printf("%d ", c);
		if (debug != '\0')
			Serial.printf("%c ", debug);
		g_telnet_writing = true;
	}
}

void telnet_end() {
	if (g_debug_telnet) {
		if (g_telnet_reading || g_telnet_writing)
			Serial.print("\r\n");
		g_telnet_reading = false;
		g_telnet_writing = false;
	}
}

void telnet_verb_SB(char c) {
	switch (c) {
		case (char)telnet_options::TERM_TYPE: {
			if (g_telnet_term_type == "") break;
			if (!(g_host->available())) break;
			char sb_verb1 = telnet_read();
			if (!(g_host->available())) break;
			char sb_verb2 = telnet_read();
			if (!(g_host->available())) break;
			char sb_verb3 = telnet_read();
			if (sb_verb1 == (char)telnet_verbs::SEND
					&& sb_verb2 == (char)telnet_verbs::IAC
					&& sb_verb3 == (char)telnet_verbs::SE) {
				telnet_write((char)telnet_verbs::IAC);
				telnet_write((char)telnet_verbs::SB);
				telnet_write((char)telnet_options::TERM_TYPE);
				telnet_write((char)telnet_verbs::IS);
				for (int i=0; g_telnet_term_type[i]!='\0'; i++)
					telnet_write(g_telnet_term_type[i]);
				telnet_write((char)telnet_verbs::IAC);
				telnet_write((char)telnet_verbs::SE);
			}
			break;
		}
		default:
			break;
	}	
}

void telnet_verb_DO(char c) {
	switch (c) {
		case (char)telnet_options::SGA: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::WILL);
			telnet_write((char)telnet_options::SGA);
			break;
		}
		case (char)telnet_options::TERM_TYPE: {
			if (g_telnet_term_type == "") {
				telnet_write((char)telnet_verbs::IAC);
				telnet_write((char)telnet_verbs::WONT);
				telnet_write((char)telnet_options::TERM_TYPE);
			} else {
				telnet_write((char)telnet_verbs::IAC);
				telnet_write((char)telnet_verbs::WILL);
				telnet_write((char)telnet_options::TERM_TYPE);
			}
			break;
		}
		case (char)telnet_options::NAWS: {
			uint8_t bytes[2];
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::WILL);
			telnet_write((char)telnet_options::NAWS);
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::SB);
			telnet_write((char)telnet_options::NAWS);
			to_big_endian(g_terminal->get_cols(), bytes);
			telnet_write((char)bytes[0]);
			telnet_write((char)bytes[1]);
			to_big_endian(g_terminal->get_rows(), bytes);
			telnet_write((char)bytes[0]);
			telnet_write((char)bytes[1]);
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::SE);
			break;
		}
		default: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::WONT);
			telnet_write(c);
			break;
		}
	}
}

void telnet_verb_DONT(char c) {
	telnet_write((char)telnet_verbs::IAC);
	telnet_write((char)telnet_verbs::WONT);
	telnet_write(c);
}

void telnet_verb_WILL(char c) {
	switch (c) {
		case (char)telnet_options::ECHO: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::DO);
			telnet_write((char)telnet_options::ECHO);
			g_host->set_local_echo("off", true);
			break;
		}
		case (char)telnet_options::SGA: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::DO);
			telnet_write((char)telnet_options::SGA);
			break;
		}
		default: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::DONT);
			telnet_write(c);
			break;
		}
	}
}

void telnet_verb_WONT(char c) {
	telnet_write((char)telnet_verbs::IAC);
	telnet_write((char)telnet_verbs::DONT);
	telnet_write(c);
}

bool telnet_char(char c) {
	if (c != (char)telnet_verbs::IAC) return false;
	if (!(g_host->available())) return false;
	char verb = telnet_read();
	switch (verb) {
		case (char)telnet_verbs::IAC: {
			// IAC escape, pass IAC to terminal
			telnet_end();
			return false;
		}
		case (char)telnet_verbs::SB: {
			if (!(g_host->available())) break;
			char option = telnet_read();
			telnet_verb_SB(option);
			break;
		}
        case (char)telnet_verbs::DO: {
			if (!(g_host->available())) break;
			char option = telnet_read();
			telnet_verb_DO(option);
			break;
		}
        case (char)telnet_verbs::DONT: {
			if (!(g_host->available())) break;
        	char option = telnet_read();
        	telnet_verb_DONT(option);
			break;
		}
        case (char)telnet_verbs::WILL: {
			if (!(g_host->available())) break;
        	char option = telnet_read();
        	telnet_verb_WILL(option);
			break;
		}
        case (char)telnet_verbs::WONT: {
			if (!(g_host->available())) break;
        	char option = telnet_read();
        	telnet_verb_WONT(option);
			break;
		}
		default: {
			telnet_write(verb, '?');
			break;
		}
	}

	telnet_end();
	return true;
}
