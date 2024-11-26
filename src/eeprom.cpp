#include <EEPROM.h>
#include "eeprom.h"

String read_eeprom(int addr_offset) {
	int new_str_len = EEPROM.read(addr_offset);
	char data[new_str_len + 1];
	for (int i = 0; i < new_str_len; i++) {
		data[i] = EEPROM.read(addr_offset + 1 + i);
	}
	data[new_str_len] = '\0';
	return String(data);
}

void write_eeprom(int addr_offset, const String &str_to_write) {
	byte length = str_to_write.length();
	EEPROM.write(addr_offset, length);
	for (int i = 0; i < length; i++) {
		EEPROM.write(addr_offset + 1 + i, str_to_write[i]);
	}
	EEPROM.commit();
}
