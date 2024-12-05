#include "../terminal.h"
#include "native.h"

NATIVE::NATIVE(const char* term_type, int rows, int cols) : Terminal(term_type, false, rows, cols) {}

void NATIVE::rt_print(char c) {
	Serial.print(c);
}
