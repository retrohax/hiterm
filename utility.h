#pragma once

void split_str(String str, char delimiter, String results[], int &count, int max_parts);
void set_serial_speed(int speed);
void to_big_endian(uint16_t value, uint8_t* bytes);
void do_wifi_config(String ssid, String passphrase);
