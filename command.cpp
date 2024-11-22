#include "command.h"
#include "host.h"
#include "utility.h"
#include "terminal.h"
#include "telnet.h"
#include "parser.h"
#include "eeprom.h"

const int MAX_CMD_PARTS = 10;
const String CMD_PROMPT = "hiterm> ";

void debug_settings(String operand, String setting) {
	String find_str = setting;
	operand.toUpperCase();
	setting.toUpperCase();
	if (operand == "VARS") {
		if (setting == "SHOW")
			g_terminal->show_vars();
		else
			Serial.println("ERROR");
	}
	else if (operand == "VT") {
		if (setting == "SHOW")
			g_terminal->show();
		else
			Serial.println("ERROR");
	}
	else if (operand == "TELNET") {
		if (setting == "OFF") {
			g_debug_telnet = false;
			Serial.println("DEBUG TELNET=OFF");
		}
		else if (setting == "ON") {
			g_debug_telnet = true;
			Serial.println("DEBUG TELNET=ON");
		}
		else
			Serial.println("ERROR");
	}
	else if (operand == "ANSI") {
		if (setting == "SHOW")
			show_ansi();
		else
			Serial.println("ERROR");
	}
	else if (operand == "TX") {
		if (setting == "SHOW")
			g_host->show_tx_hist();
		else
			Serial.println("ERROR");
	}
	else if (operand == "RX") {
		if (setting == "SHOW")
			g_host->show_rx_hist();
		else if (setting == "SAVE")
			g_host->save_rx_hist();
		else if (setting == "REPLAY")
			g_host->replay_rx_hist();
		else
			g_host->show_rx_hist(find_str);
	}
	else
		Serial.println("ERROR");
}

int process(String cmd_str) {
	String parts[MAX_CMD_PARTS];
	int num_parts;

	split_str(cmd_str, ':', parts, num_parts, MAX_CMD_PARTS);
	
	if (num_parts < 1) return 1;
	String cmd = parts[0];
	cmd.toUpperCase();

	if (cmd == "WIFI") {
		if (num_parts < 3) return 0;
		do_wifi_config(parts[1], parts[2]);
	}
	else if (cmd == "TCP") {
		if (num_parts < 3) return 0;
		g_host->tcp_connect(parts[1], parts[2].toInt());
	}
	else if (cmd == "TLS") {
		if (num_parts < 3) return 0;
		g_host->tls_connect(parts[1], parts[2].toInt());
	}
	else if (cmd == "BYE") {
		g_host->shutdown();
	}
	else if (cmd == "TERM") {
		if (num_parts < 2) return 0;
		set_term_type(parts[1]);
	}
	else if (cmd == "ANSI") {
		if (num_parts < 2) return 0;
		set_ansi_mode(parts[1]);
	}
	else if (cmd == "ECHO") {
		if (num_parts < 2) return 0;
		g_host->set_local_echo(parts[1]);
	}
	else if (cmd == "CRLF") {
		g_host->toggle_crlf();
	}
	else if (cmd == "PUT") {
		if (num_parts < 3) return 0;
		do_put_user_data(parts[1], parts[2]);
	}
	else if (cmd == "GET") {
		if (num_parts < 2) return 0;
		do_get_user_data(parts[1]);
	}
	else if (cmd == "LISTEEPROM") {
		list_eeprom();
	}
	else if (cmd == "SPEED") {
		if (num_parts < 2) return 0;
		set_serial_speed(parts[1].toInt());
	}
	else if (cmd == "RESTART") {
		Serial.println("RESTARTING");
		ESP.restart();
	}
	else if (cmd == "DEBUG") {
		if (num_parts < 3) return 0;
		debug_settings(parts[1], parts[2]);
	}
	else if (cmd == "HELP") {
		if (num_parts == 1) {
			Serial.println("speed:<baud rate>");
			Serial.println("wifi:<ssid>:<password>");
			Serial.println("term:<TERMINFO_NAME/none>");
			Serial.println("ansi:<on/off>");
			Serial.println("echo:<on/off>");
			Serial.println("crlf");
			Serial.println("tcp:<host>:<port>");
			Serial.println("tls:<host>:<port>");
			Serial.println("bye");
			Serial.println("put:<eeprom_field>:<data>");
			Serial.println("get:<eeprom_field>");
			Serial.println("listeeprom");
			Serial.println("restart");
			Serial.println("debug:<option>:<setting>");
			Serial.println("");
			Serial.println("Escape character is '^]'");
		}
		else {
			String operand = parts[1];
			operand.toUpperCase();
			if (operand == "DEBUG") {
				Serial.println("debug:<vars>:<show>");
				Serial.println("debug:<ansi>:<show>");
				Serial.println("debug:<rx>:<show>");
				Serial.println("debug:<tx>:<show>");
				Serial.println("debug:<telnet>:<on/off>");
			}
			else
				return 0;
		}
	}
	else
		return 0;

	return 1;
}

void command() {
	String cmd_str = "";
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
	if (!process(cmd_str))
		Serial.println("BAD COMMAND");
}
