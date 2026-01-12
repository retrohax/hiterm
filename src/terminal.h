#pragma once
#include <Arduino.h>
#include "term_telnet.h"
#include "host.h"

extern TERM_BASE *g_terminal;
extern String g_terminal_type;
extern int g_terminal_rows;
extern int g_terminal_cols;
extern void init_terminal(ConnectionType conn_type);
