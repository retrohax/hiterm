#include "../../terminal.h"
#include "none.h"

NONE::NONE() : Terminal() {}

void NONE::rt_print(char c) {
	Serial.print(c);
}
