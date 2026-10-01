#pragma once
#include <Arduino.h>

const int EEPROM_FIELD_MAXLEN = 80;
const int EEPROM_SYS1_ADDR = (0 * EEPROM_FIELD_MAXLEN);		// wifi ssid
const int EEPROM_SYS2_ADDR = (1 * EEPROM_FIELD_MAXLEN);		// wifi password
const int EEPROM_BAUD_ADDR = (2 * EEPROM_FIELD_MAXLEN);		// serial baud rate
const int EEPROM_TERM_ADDR = (3 * EEPROM_FIELD_MAXLEN);		// terminal type
const int EEPROM_USR_COUNT = 10;					// user configurable strings, USR0-USR9
const int EEPROM_USR_BASE = (4 * EEPROM_FIELD_MAXLEN);
#define EEPROM_USR_ADDR(n) (EEPROM_USR_BASE + (n) * EEPROM_FIELD_MAXLEN)
const int EEPROM_FLAG_ADDR = EEPROM_USR_ADDR(EEPROM_USR_COUNT);
const int EEPROM_LEN = EEPROM_FLAG_ADDR + 1;			// EEPROM.begin(EEPROM_LEN)

String read_eeprom(int addr_offset);
void write_eeprom(int addr_offset, const String &str_to_write);
void do_put_user_data(String field_name, String field_data);
void do_get_user_data(String field_name);
void list_eeprom();
