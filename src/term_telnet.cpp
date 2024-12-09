#include <Arduino.h>
#include <limits>
#include "host.h"
#include "command.h"
#include "term_base.h"
#include "term_telnet.h"

void to_big_endian(uint16_t value, uint8_t* bytes);

TERM_TELNET::TERM_TELNET(const String& term_type, int rows, int cols) : TERM_BASE() {
	m_term_type = term_type;
	m_term_rows = rows;
	m_term_cols = cols;
}

void TERM_TELNET::show_term_type() {
	if (m_term_rows == 0) {
		Serial.printf(
			"Terminal type is %s (COLS=%d).\r\n",
			m_term_type.c_str(),
			m_term_cols
		);
	} else {
		Serial.printf(
			"Terminal type is %s (ROWS=%d, COLS=%d).\r\n",
			m_term_type.c_str(),
			m_term_rows,
			m_term_cols
		);
	}
}

void TERM_TELNET::print(char c) {
	if (!telnet_char(c))
		TERM_BASE::print(c);
}

char TERM_TELNET::telnet_read() {
	return g_host->read();
}

void TERM_TELNET::telnet_write(char c) {
	if (m_write_buffer_pos < WRITE_BUFFER_SIZE) {
		m_write_buffer[m_write_buffer_pos++] = c;
	}
}

void TERM_TELNET::telnet_write_str(String str) {
	for (int i=0; i < str.length(); i++)
		telnet_write(str[i]);
}

void TERM_TELNET::telnet_flush() {
	if (m_write_buffer_pos > 0) {
		g_host->write_buffer(m_write_buffer, m_write_buffer_pos);
		m_write_buffer_pos = 0;
	}
}

void TERM_TELNET::telnet_verb_SB(char c) {
	switch (c) {
		case (char)telnet_options::TERM_TYPE: {
			if (m_term_type == "") break;
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
				telnet_write_str(m_term_type);
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

void TERM_TELNET::telnet_verb_DO(char c) {
	switch (c) {
		case (char)telnet_options::SGA: {
			telnet_write((char)telnet_verbs::IAC);
			telnet_write((char)telnet_verbs::WILL);
			telnet_write((char)telnet_options::SGA);
			telnet_flush();
			break;
		}
		case (char)telnet_options::TERM_TYPE: {
			if (m_term_type == "") {
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
			to_big_endian((uint16_t)m_term_cols, bytes);
			telnet_write((char)bytes[0]);
			telnet_write((char)bytes[1]);
			to_big_endian((uint16_t)m_term_rows, bytes);
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

void TERM_TELNET::telnet_verb_DONT(char c) {
	telnet_write((char)telnet_verbs::IAC);
	telnet_write((char)telnet_verbs::WONT);
	telnet_write(c);
	telnet_flush();
}

void TERM_TELNET::telnet_verb_WILL(char c) {
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

void TERM_TELNET::telnet_verb_WONT(char c) {
	telnet_write((char)telnet_verbs::IAC);
	telnet_write((char)telnet_verbs::DONT);
	telnet_write(c);
	telnet_flush();
}

bool TERM_TELNET::telnet_char(char c) {
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

/*
	UTILITY FUNCTIONS
*/

void to_big_endian(uint16_t value, uint8_t* bytes) {
	bytes[0] = (value >> 8) & 0xFF;
	bytes[1] = value & 0xFF;
}

