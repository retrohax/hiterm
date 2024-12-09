#include "command.h"
#include "host.h"
#include "terminal.h"
#include "serial.h"
#include "wifi.h"

const int MAX_CMD_LEN = 80;
const int MAX_CMD_PARTS = 10;

struct Command {
	const char* name;
	void (*handler)(String* parts, int count);
};

struct SetCommand {
	const char* name;
	void (*handler)(const String& val, String* options, int option_count);
};

struct ToggleCommand {
	const char* name;
	void (*handler)();
};

const Command* find_command(const String& input, const Command commands[]);
const SetCommand* find_set_command(const String& input);
const ToggleCommand* find_toggle_command(const String& input);

void cmd_help_main();
void cmd_wifi_config();
void cmd_open_connection(String host, int port);
void cmd_open_tls_connection(String host, int port);
void cmd_close_connection();
void cmd_display();
void cmd_show_status();
void cmd_show_vars();
void cmd_show_rx_hist();
void cmd_show_tx_hist();
void cmd_restart_device();
void cmd_help_toggle();
void cmd_toggle_crlf();
void cmd_toggle_echo();
void cmd_toggle_parameters(String key);
void cmd_help_set(const String& val, String* options, int option_count);
void cmd_set_baud_rate(const String& val, String* options, int option_count);
void cmd_set_term_type(const String& val, String* options, int option_count);
void cmd_set_parameters(String key, String val, String* options, int option_count);

void split_str(String str, char delimiter, String results[], int &count, int max_parts);


/*
	HELP COMMANDS
*/

void cmd_help_main() {
	Serial.println("Commands may be abbreviated.  Commands are:");
	Serial.println();
	Serial.println("wifi            configure wifi");
	Serial.println("open            connect to a site");
	Serial.println("tlsopen         connect to a TLS site");
	Serial.println("close           close current connection");
	Serial.println("set             set operating parameters ('set ?' for more)");
	Serial.println("toggle          toggle operating parameters ('toggle ?' for more)");
	Serial.println("display         display operating parameters");
	Serial.println("status          print status information");
	Serial.println("restart         restart ESP device");
	Serial.println();
	Serial.println("Debug commands:");
	Serial.println("show_vars       show terminal variables");
	Serial.println("show_rx         show received data");
	Serial.println("show_tx         show sent data");
}

void cmd_help_toggle() {
	Serial.println("crlf            toggle sending carriage returns as telnet <CR><LF>");
	Serial.println("echo            toggle local echo");
}

void cmd_help_set(const String& val, String* options, int option_count) {
	Serial.println("baud            serial baud rate");
	Serial.println("term            terminal type (optional: rows cols)");
}

/*
	MAIN COMMANDS
*/

const Command MAIN_COMMANDS[] = {
    {"?", [](String* parts, int count) { 
        cmd_help_main(); 
    }},
    {"HELP", [](String* parts, int count) { 
        cmd_help_main(); 
    }},
    {"WIFI", [](String* parts, int count) { 
        cmd_wifi_config(); 
    }},
	{"OPEN", [](String* parts, int count) {
		cmd_open_connection(count >= 2 ? parts[1] : "", count == 3 ? parts[2].toInt() : 23);
	}},
	{"TLSOPEN", [](String* parts, int count) {
		cmd_open_tls_connection(count >= 2 ? parts[1] : "", count == 3 ? parts[2].toInt() : 992);
	}},
	{"CLOSE", [](String* parts, int count) {
		cmd_close_connection();
	}},
	{"DISPLAY", [](String* parts, int count) {
		cmd_display();
	}},
	{"STATUS", [](String* parts, int count) {
		cmd_show_status();
	}},
	{"SHOW_VARS", [](String* parts, int count) {
		cmd_show_vars();
	}},
	{"SHOW_RX", [](String* parts, int count) {
		cmd_show_rx_hist();
	}},
	{"SHOW_TX", [](String* parts, int count) {
		cmd_show_tx_hist();
	}},
	{"RESTART", [](String* parts, int count) {
		cmd_restart_device();
	}},
	{"SET", [](String* parts, int count) {
		cmd_set_parameters(
			(count >= 2) ? parts[1] : "",
			(count >= 3) ? parts[2] : "",
			parts + 3,
			count > 3 ? count - 3 : 0
		);
	}},
	{"TOGGLE", [](String* parts, int count) {
		cmd_toggle_parameters(count >= 2 ? parts[1] : "");
	}},
	{nullptr, nullptr}  // Terminator
};

const Command* find_command(const String& input, const Command commands[]) {
	const Command* match = nullptr;
	
	for (int i = 0; commands[i].name != nullptr; i++) {
		if (strncasecmp(input.c_str(), commands[i].name, input.length()) == 0) {
			if (match != nullptr) {
				// Found second match - ambiguous!
				Serial.println("?Ambiguous command");
				return nullptr;
			}
			match = &commands[i];
		}
	}

	if (match == nullptr) {
		Serial.println("?Invalid command");
	}

	return match;
}

void cmd_wifi_config() {
	wifi_config();
}

void cmd_open_connection(String host, int port) {
	if (host.isEmpty()) {
		Serial.println("?Missing host");
		return;
	}
	g_host->connect(host, port);
}

void cmd_open_tls_connection(String host, int port) {
	if (host.isEmpty()) {
		Serial.println("?Missing host");
		return;
	}
	g_host->connect(host, port, true);
}

void cmd_close_connection() {
	g_host->shutdown();
}

void cmd_display() {
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

void cmd_show_status() {
	g_host->show_status();
}

void cmd_show_vars() {
	g_terminal->show_vars();
}

void cmd_show_rx_hist() {
	g_host->show_rx_hist();
}

void cmd_show_tx_hist() {
	g_host->show_tx_hist();
}

void cmd_restart_device() {
	Serial.println("Restarting");
	ESP.restart();
}

void cmd_set_parameters(String key, String val, String* options, int option_count) {
    const SetCommand* cmd = find_set_command(key);
    if (!cmd) return;
    cmd->handler(val, options, option_count);
}

void cmd_toggle_parameters(String key) {
    const ToggleCommand* cmd = find_toggle_command(key);
    if (!cmd) return;
    cmd->handler();
}

/*
	SET COMMANDS
*/

const SetCommand SET_COMMANDS[] = {
    {"?", cmd_help_set},
    {"BAUD", cmd_set_baud_rate},
    {"TERM", cmd_set_term_type},
    {nullptr, nullptr}  // Terminator
};

const SetCommand* find_set_command(const String& input) {
    const SetCommand* match = nullptr;
    
    for (int i = 0; SET_COMMANDS[i].name != nullptr; i++) {
        if (strncasecmp(input.c_str(), SET_COMMANDS[i].name, input.length()) == 0) {
            if (match != nullptr) {
                // Found second match - ambiguous!
                Serial.println("?Ambiguous parameter");
                return nullptr;
            }
            match = &SET_COMMANDS[i];
        }
    }

    if (match == nullptr) {
        Serial.println("?Invalid parameter");
    }

    return match;
}

void cmd_set_baud_rate(const String& val, String* options, int option_count) {
    uint32_t baud_rate = strtoul(val.c_str(), NULL, 10);
    set_serial_baud_rate(baud_rate);
    show_serial_baud_rate();
}

void cmd_set_term_type(const String& val, String* options, int option_count) {
    String term_type = val;
	int rows = options[0].toInt();
	int cols = options[1].toInt();
    if (init_terminal(term_type, rows, cols)) {
        g_terminal->show_term_type();
    }
}

/*
	TOGGLE COMMANDS
*/

const ToggleCommand TOGGLE_COMMANDS[] = {
	{"?", cmd_help_toggle},
	{"CRLF", cmd_toggle_crlf},
	{"ECHO", cmd_toggle_echo},
	{nullptr, nullptr}  // Terminator
};

const ToggleCommand* find_toggle_command(const String& input) {
    const ToggleCommand* match = nullptr;
    
    for (int i = 0; TOGGLE_COMMANDS[i].name != nullptr; i++) {
        if (strncasecmp(input.c_str(), TOGGLE_COMMANDS[i].name, input.length()) == 0) {
            if (match != nullptr) {
                // Found second match - ambiguous!
                Serial.println("?Ambiguous parameter");
                return nullptr;
            }
            match = &TOGGLE_COMMANDS[i];
        }
    }

    if (match == nullptr) {
        Serial.println("?Invalid parameter");
    }

    return match;
}

void cmd_toggle_crlf() {
	g_host->toggle_crlf();
	g_host->show_crlf();
}

void cmd_toggle_echo() {
	g_host->toggle_local_echo();
	g_host->show_local_echo();
}

/*
	COMMAND PROCESSING
*/

void process(String cmd_str) {
	String parts[MAX_CMD_PARTS];
	int num_parts;
	split_str(cmd_str, ' ', parts, num_parts, MAX_CMD_PARTS);
	if (num_parts < 1) return;
	const Command* cmd = find_command(parts[0], MAIN_COMMANDS);
	if (!cmd) return;
	cmd->handler(parts, num_parts);
}

void command() {
	String cmd_str;
	cmd_str.reserve(MAX_CMD_LEN);
	cmd_str = "";
	Serial.printf("\r\n%s", CMD_PROMPT.c_str());

	while (true) {
		while (!Serial.available()) yield();
		char c = Serial.read();
		if (c == '\r')
			break;
			
		// Handle escape sequences for cursor keys
		if (c == '\e') {
			// Wait for [ character
			while (!Serial.available()) yield();
			if (Serial.read() != '[') continue;
			
			// Read and discard the cursor key code
			while (!Serial.available()) yield();
			Serial.read();
			continue;
		}
		
		// Handle backspace (both ASCII backspace and delete)
		if (c == '\b' || c == '\x7f') {
			if (cmd_str.length() >= 1) {
				Serial.printf("\b \b");
				cmd_str.remove(cmd_str.length()-1);
			}
			continue;
		}
		
		// Ignore other control characters
		if (c < 32)
			continue;
			
		// Add printable characters to command string
		if (cmd_str.length() < MAX_CMD_LEN) {
			Serial.print(c);
			cmd_str += c;
		}
	}

	Serial.println();
	process(cmd_str);
}

/*
	HELPER FUNCTIONS
*/

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

