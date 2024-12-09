#pragma once
#include <Arduino.h>
#include "term_base.h"

class TERM_TELNET : public TERM_BASE {
	public:
		TERM_TELNET(const String& term_type = "", int rows = 0, int cols = 0);

		virtual void print(char c) override;
		virtual void show_term_type() override;

	protected:
		String m_term_type;
		int m_term_rows;
		int m_term_cols;

		bool telnet_char(char c);

	private:
        static const int WRITE_BUFFER_SIZE = 256;
        char m_write_buffer[WRITE_BUFFER_SIZE];
        int m_write_buffer_pos = 0;

		char telnet_read();
		void telnet_write(char c);
		void telnet_write_str(String str);
		void telnet_flush();
		void telnet_verb_SB(char c);
		void telnet_verb_DO(char c);
		void telnet_verb_DONT(char c);
		void telnet_verb_WILL(char c);
		void telnet_verb_WONT(char c);
};

enum telnet_verbs {
	IS = 0,
	SEND = 1,
	SE = 240,
	SB = 250,
	WILL = 251,
	WONT = 252,
	DO = 253,
	DONT = 254,
	IAC = 255
};

enum telnet_options {
	BINARY = 0,
	ECHO = 1,
	SGA = 3,
	TERM_TYPE = 24,
	ENV = 36,
	NEW_ENV = 39,
	NAWS = 31
};
