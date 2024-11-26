#pragma once
#include <Arduino.h>

void parse_char(char c);
void parse_parm_str(String str, char delimiter, int results[], int &count, int max_parts);
void parser_init();
void save_ansi(String ansi_str);
void show_ansi();
