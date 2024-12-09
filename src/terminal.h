#pragma once
#include <Arduino.h>
#include "term_telnet.h"

extern TERM_BASE *g_terminal;
bool init_terminal(String term_type="", int rows=0, int cols=0);
