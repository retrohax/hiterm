#include "command.h"
#include "host.h"
#include "terminal.h"
#include "parser.h"
#include "eeprom.h"

const int MAX_CMD_PARTS = 10;
const String CMD_PROMPT = "hiterm> ";

void help_main() {
	Serial.println("Commands are:");
	Serial.println();
	Serial.println("wifi            configure wifi");
	Serial.println("open            connect to a site");
	Serial.println("opentls         connect to a ssl site");
	Serial.println("close           close current connection");
	Serial.println("set             set operating parameters ('set ?' for more)");
	Serial.println("toggle          toggle operating parameters ('toggle ?' for more)");
	Serial.println("display         display operating parameters");
	Serial.println("status          print status information");
	Serial.println("restart         restart ESP device");
}

void help_set() {
	Serial.println("baud            serial baud rate");
	Serial.println("term            terminal type");
}

void help_toggle() {
	Serial.println("crlf            toggle sending carriage returns as telnet <CR><LF>");
	Serial.println("echo            toggle local echo");
}

String readLineWithEcho(bool echo) {
	String input = "";
	while(true) {
		if (Serial.available()) {
			char c = Serial.read();
			if (c == '\r') break;
			if (echo) Serial.write(c);
			input += c;
		}
	}
	return input;
}

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

void wifi_config() {
	Serial.print("SSID: ");
	String ssid = readLineWithEcho();
	Serial.println();
	ssid = ssid.substring(0, EEPROM_FIELD_MAXLEN-1);
	Serial.print("Passphrase: ");
	String passphrase = readLineWithEcho();
	Serial.println();
	passphrase = passphrase.substring(0, EEPROM_FIELD_MAXLEN-1);
	write_eeprom(EEPROM_SYS1_ADDR, ssid);
	write_eeprom(EEPROM_SYS2_ADDR, passphrase);
}

int set_serial_baud_rate(int baud_rate) {
	switch (baud_rate) {
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
			return 0;
	}
	write_eeprom(EEPROM_SERI_ADDR, String(baud_rate));
	return 1;
}

void show_serial_baud_rate() {
	int baud_rate = read_eeprom(EEPROM_SERI_ADDR).toInt();
	Serial.printf("Serial baud rate is %d.\r\n", baud_rate);
}

int set(String key, String val) {
	key.toUpperCase();
	if (key == "?") {
		help_set();
		return 1;
	}
	if (key == "TERM") {
		if (init_terminal(val)) {
			g_terminal->show_term_type();
		}
		return 1;
	}
	if (key == "BAUD") {
		int baud_rate = val.toInt();
		if (set_serial_baud_rate(baud_rate) == 1) {
			show_serial_baud_rate();
			return 1;
		}
	}
	return 0;
}

int toggle(String key) {
	key.toUpperCase();
	if (key == "?") {
		help_toggle();
		return 1;
	}
	if (key == "CRLF") {
		g_host->toggle_crlf();
		g_host->show_crlf();
		return 1;
	}
	if (key == "ECHO") {
		g_host->toggle_local_echo();
		g_host->show_local_echo();
		return 1;
	}
	return 0;
}

void display() {
	Serial.println("echo            [^E]");
	Serial.println("escape          [^]]");
	Serial.println("close           [^\\]");
	Serial.println("start           [^Q]");
	Serial.println("stop            [^S]");
	Serial.println();
	g_terminal->show_term_type();
	g_host->show_crlf();
	g_host->show_local_echo();
	show_serial_baud_rate();
}

int process(String cmd_str) {
	String parts[MAX_CMD_PARTS];
	int num_parts;

	split_str(cmd_str, ' ', parts, num_parts, MAX_CMD_PARTS);
	
	if (num_parts < 1) return 1;
	String cmd = parts[0];
	cmd.toUpperCase();

	if (cmd == "HELP" || cmd == "?") {
		help_main();
		return 1;
	}

	if (cmd == "WIFI") {
		wifi_config();
		return 1;
	}

	if (cmd == "OPEN") {
		if (num_parts < 2) return 0;
		int port = 23;
		if (num_parts == 3)
			port = parts[2].toInt();
		g_host->connect(parts[1], port);
		return 1;
	}

	if (cmd == "OPENTLS") {
		if (num_parts < 2) return 0;
		int port = 992;
		if (num_parts == 3)
			port = parts[2].toInt();
		g_host->connect(parts[1], port, true);
		return 1;
	}

	if (cmd == "CLOSE") {
		g_host->shutdown();
		return 1;
	}

	if (cmd == "SET") {
		if (num_parts < 2) return 0;
		String key = parts[1];
		String val = (num_parts < 3) ? "" : parts[2];
		return set(key, val);
	}

	if (cmd == "TOGGLE") {
		if (num_parts < 2) return 0;
		return toggle(parts[1]);
	}

	if (cmd == "DISPLAY") {
		display();
		return 1;
	}

	if (cmd == "STATUS") {
		g_host->show_status();
		return 1;
	}

	if (cmd == "RESTART") {
		Serial.println("Restarting");
		ESP.restart();
	}

	return 0;
}

void command() {
	String cmd_str;
	cmd_str.reserve(80);
	while (true) {
		cmd_str = "";
		Serial.printf("\r\n%s", CMD_PROMPT.c_str());
		while (true) {
			if (Serial.available()) {
				char c = Serial.read();
				if (c == '\r')
					break;
				if (c == '\b') {
					if (cmd_str.length() >= 1) {
						Serial.printf("\b \b");
						cmd_str.remove(cmd_str.length()-1);
					}
					continue;
				}
				Serial.print(c);
				cmd_str += c;
			}
			yield();
		}
		Serial.println();
		if (process(cmd_str))
			break;
		else
			Serial.println("?Invalid command");
	}
}
