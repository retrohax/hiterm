#pragma once
#include <Arduino.h>

const int EEPROM_FIELD_MAXLEN = 80;
const int EEPROM_SERI_ADDR = (0 * EEPROM_FIELD_MAXLEN) + 0;		// serial speed
const int EEPROM_SYS1_ADDR = (1 * EEPROM_FIELD_MAXLEN) + 1;		// wifi ssid
const int EEPROM_SYS2_ADDR = (2 * EEPROM_FIELD_MAXLEN) + 2;		// wifi password
const int EEPROM_SYS3_ADDR = (3 * EEPROM_FIELD_MAXLEN) + 3;		// vterm type
const int EEPROM_SYS4_ADDR = (4 * EEPROM_FIELD_MAXLEN) + 4;		// rterm type
const int EEPROM_USR1_ADDR = (5 * EEPROM_FIELD_MAXLEN) + 5;
const int EEPROM_USR2_ADDR = (6 * EEPROM_FIELD_MAXLEN) + 6;
const int EEPROM_FLAG_ADDR = (7 * EEPROM_FIELD_MAXLEN) + 7;

const int EEPROM_RX_DATA_ADDR = EEPROM_FLAG_ADDR + 1;

String read_eeprom(int addr_offset);
void write_eeprom(int addr_offset, const String &str_to_write);
void do_put_user_data(String field_name, String field_data);
void do_get_user_data(String field_name);
void list_eeprom();
