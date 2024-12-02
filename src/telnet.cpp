#include "telnet.h"
#include "host.h"
#include "terminal.h"

static const int BUFFER_SIZE = 256;
char g_buffer[BUFFER_SIZE];
int g_buffer_pos = 0;

void to_big_endian(uint16_t value, uint8_t* bytes) {
	bytes[0] = (value >> 8) & 0xFF;
	bytes[1] = value & 0xFF;
}

char telnet_read() {
	return g_host->read();
}

void telnet_write(char c) {
	if (g_buffer_pos < BUFFER_SIZE) {
		g_buffer[g_buffer_pos++] = c;
	}
}

void telnet_write_str(String str) {
	for (int i=0; i < str.length(); i++)
		telnet_write(str[i]);
}

void telnet_flush() {
	if (g_buffer_pos > 0) {
		g_host->write_buffer(g_buffer, g_buffer_pos);
		g_buffer_pos = 0;
	}
}

void telnet_verb_SB(char c) {
	switch (c) {
		case (char)telnet_options::TERM_TYPE: {
			if (g_terminal->get_telnet_term_type() == "") break;
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
				telnet_write_str(g_terminal->get_telnet_term_type());
				telnet_write((char)telnet_verbs::IAC);
				telnet_write((char)telnet_verbs::SE);
				telnet_flush();
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
			telnet_flush();
			break;
		}
		case (char)telnet_options::TERM_TYPE: {
			if (g_terminal->get_telnet_term_type() == "") {
				telnet_write((char)telnet_verbs::IAC);
				telnet_write((char)telnet_verbs::WONT);
				telnet_write((char)telnet_options::TERM_TYPE);
				telnet_flush();
			} else {
				telnet_write((char)telnet_verbs::IAC);
				telnet_write((char)telnet_verbs::WILL);
				telnet_write((char)telnet_options::TERM_TYPE);
				telnet_flush();
			}
			break;
		}
		case (char)telnet_options::NAWS: {
			uint8_t bytes[2];
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::WILL);
			telnet_write((char)telnet_options::NAWS);
			telnet_flush();
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
			telnet_flush();
			break;
		}
		default: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::WONT);
			telnet_write(c);
			telnet_flush();
			break;
		}
	}
}

void telnet_verb_DONT(char c) {
	telnet_write((char)telnet_verbs::IAC);
	telnet_write((char)telnet_verbs::WONT);
	telnet_write(c);
	telnet_flush();
}

void telnet_verb_WILL(char c) {
	switch (c) {
		case (char)telnet_options::ECHO: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::DO);
			telnet_write((char)telnet_options::ECHO);
			telnet_flush();
			g_host->set_local_echo(false);
			break;
		}
		case (char)telnet_options::SGA: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::DO);
			telnet_write((char)telnet_options::SGA);
			telnet_flush();
			break;
		}
		default: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::DONT);
			telnet_write(c);
			telnet_flush();
			break;
		}
	}
}

void telnet_verb_WONT(char c) {
	telnet_write((char)telnet_verbs::IAC);
	telnet_write((char)telnet_verbs::DONT);
	telnet_write(c);
	telnet_flush();
}

bool telnet_char(char c) {
	if (c != (char)telnet_verbs::IAC) return false;
	if (!(g_host->available())) return false;
	char verb = telnet_read();
	switch (verb) {
		case (char)telnet_verbs::IAC: {
			// IAC escape, pass IAC to terminal
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
	}
	return true;
}
