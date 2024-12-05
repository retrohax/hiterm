#include "wifi.h"
#include "eeprom.h"

String read_line_with_echo() {
    String input = "";
    while (true) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\r') break;
            input += c;
        }
    }
    return input;
}

void wifi_config() {
    Serial.print("SSID: ");
    String ssid = read_line_with_echo();
    Serial.println();
    if (ssid.isEmpty()) {
        return;
    }
    ssid = ssid.substring(0, EEPROM_FIELD_MAXLEN-1);
    Serial.print("Passphrase: ");
    String passphrase = read_line_with_echo();
    Serial.println();
    passphrase = passphrase.substring(0, EEPROM_FIELD_MAXLEN-1);
    write_eeprom(EEPROM_SYS1_ADDR, ssid);
    write_eeprom(EEPROM_SYS2_ADDR, passphrase);
}
