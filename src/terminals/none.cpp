#include "../terminal.h"
#include "none.h"

NONE::NONE(const char* term_type) : Terminal(term_type) {}

void NONE::rt_print(char c) {
	Serial.print(c);
}
