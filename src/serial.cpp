#include "serial.h"
#include "eeprom.h"
#include <Arduino.h>

void show_serial_baud_rate() {
    int baud_rate = read_eeprom(EEPROM_SERI_ADDR).toInt();
    Serial.printf("Serial baud rate is %d.\r\n", baud_rate);
}

void set_serial_baud_rate(uint32_t baud_rate) {
    if (baud_rate >= 75 && baud_rate <= 115200) {
        write_eeprom(EEPROM_SERI_ADDR, String(baud_rate));
    } else {
        Serial.println("Baud rate must be between 75 and 115200.");
    }
}

void init_serial() {
    String baud_rate = read_eeprom(EEPROM_SERI_ADDR);
    Serial.begin(baud_rate.toInt());
    delay(500);
}
