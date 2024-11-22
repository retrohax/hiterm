#include <Arduino.h>
#include "utility.h"
#include "eeprom.h"

void split_str(String str, char delimiter, String results[], int &count, int max_parts) {
	count = 0;
	int s0 = 0;
	int s1 = 0;
	while ((s1 = str.indexOf(delimiter, s0)) != -1) {
		results[count++] = str.substring(s0, s1);
		s0 = s1 + 1;
		if (count >= max_parts - 1) break;
	}
	if (s0 < str.length()) {
		results[count++] = str.substring(s0);
	}
}

void set_serial_speed(int speed) {
	switch (speed) {
		case 75: break;
		case 110: break;
		case 150: break;
		case 300: break;
		case 600: break;
		case 1200: break;
		case 1800: break;
		case 2400: break;
		case 4800: break;
		case 9600: break;
		case 19200: break;
		default:
			Serial.println("ERROR");
	}
	write_eeprom(EEPROM_SERI_ADDR, String(speed));
	Serial.printf("SPEED=%d\r\n", speed);
}

void do_wifi_config(String ssid, String passphrase) {
	if (ssid.length() > EEPROM_FIELD_MAXLEN) {
		Serial.println("ERROR");
	} else if (passphrase.length() > EEPROM_FIELD_MAXLEN) {
		Serial.println("ERROR");
	} else {
		write_eeprom(EEPROM_SYS1_ADDR, ssid);
		write_eeprom(EEPROM_SYS2_ADDR, passphrase);
		Serial.printf("SSID=%s\r\n", ssid.c_str());
		Serial.printf("PASSWORD=%s\r\n", passphrase.c_str());
	}
}

void to_big_endian(uint16_t value, uint8_t* bytes) {
	bytes[0] = (value >> 8) & 0xFF;
	bytes[1] = value & 0xFF;
}
