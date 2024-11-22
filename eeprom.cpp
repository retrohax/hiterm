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

void do_put_user_data(String field_name, String field_data) {
	if (field_data.length() > EEPROM_FIELD_MAXLEN) {
		Serial.println("ERROR");
	} else {
		field_name.toUpperCase();
		if (field_name == "USR1") {
			write_eeprom(EEPROM_USR1_ADDR, field_data);
			Serial.printf("%s=%s\r\n", field_name.c_str(), field_data.c_str());
		} else if (field_name == "USR2") {
			write_eeprom(EEPROM_USR2_ADDR, field_data);
			Serial.printf("%s=%s\r\n", field_name.c_str(), field_data.c_str());
		} else {
			Serial.println("ERROR");
		}
	}
}

void do_get_user_data(String field_name) {
	field_name.toUpperCase();
	if (field_name == "USR1")
		Serial.println(read_eeprom(EEPROM_USR1_ADDR));
	else if (field_name == "USR2")
		Serial.println(read_eeprom(EEPROM_USR2_ADDR));
	else
		Serial.println("ERROR");
}

void list_eeprom() {
	Serial.printf("SERI: [%s]\r\n", read_eeprom(EEPROM_SERI_ADDR).c_str());
	Serial.printf("SYS1: [%s]\r\n", read_eeprom(EEPROM_SYS1_ADDR).c_str());
	Serial.printf("SYS2: [%s]\r\n", read_eeprom(EEPROM_SYS2_ADDR).c_str());
	Serial.printf("SYS3: [%s]\r\n", read_eeprom(EEPROM_SYS3_ADDR).c_str());
	Serial.printf("SYS4: [%s]\r\n", read_eeprom(EEPROM_SYS4_ADDR).c_str());
	Serial.printf("USR1: [%s]\r\n", read_eeprom(EEPROM_USR1_ADDR).c_str());
	Serial.printf("USR2: [%s]\r\n", read_eeprom(EEPROM_USR2_ADDR).c_str());
}

