#pragma once
#include <Arduino.h>

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

bool telnet_char(char c);
