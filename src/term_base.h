#pragma once
#include <Arduino.h>

class TERM_BASE {
	public:
		TERM_BASE();
		virtual ~TERM_BASE() = default;

		virtual void reset() {};
		virtual void show_vars() {};

		virtual bool available();
		virtual char read();
		virtual void print(char c);
};
