#pragma once
#include <Arduino.h>

const int EEPROM_FIELD_MAXLEN = 80;
const int EEPROM_SERI_ADDR = (0 * EEPROM_FIELD_MAXLEN);		// serial speed
const int EEPROM_SYS1_ADDR = (1 * EEPROM_FIELD_MAXLEN);		// wifi ssid
const int EEPROM_SYS2_ADDR = (2 * EEPROM_FIELD_MAXLEN);		// wifi password
const int EEPROM_USR1_ADDR = (5 * EEPROM_FIELD_MAXLEN);
const int EEPROM_USR2_ADDR = (6 * EEPROM_FIELD_MAXLEN);
const int EEPROM_FLAG_ADDR = (7 * EEPROM_FIELD_MAXLEN);
const int EEPROM_LEN = EEPROM_FLAG_ADDR + 1;				// EEPROM.begin(EEPROM_LEN)

String read_eeprom(int addr_offset);
void write_eeprom(int addr_offset, const String &str_to_write);
void do_put_user_data(String field_name, String field_data);
void do_get_user_data(String field_name);
void list_eeprom();
