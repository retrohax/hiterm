#include "eeprom.h"
#include "terminal.h"
#include "host.h"
#include "term_base.h"
#include "term_telnet.h"
#include "terminals/lsi_adm3a.h"

TERM_BASE *g_terminal = new TERM_BASE();
int g_terminal_rows = 0;
int g_terminal_cols = 0;
String g_terminal_type = "none";

void init_term_type() {
	String term_type = read_eeprom(EEPROM_TERM_ADDR);
	char buf[term_type.length()+1];
	term_type.toCharArray(buf, sizeof(buf));
	char *tt_name = strtok(buf, " ");
	char *tt_s1 = (tt_name != NULL) ? strtok(NULL, " ") : NULL;
	char *tt_s2 = (tt_s1 != NULL) ? strtok(NULL, " ") : NULL;
	g_terminal_type = (tt_name == NULL) ? "none" : tt_name;
	if (g_terminal_type.equalsIgnoreCase("none")) {
		g_terminal_rows = 0;
		g_terminal_cols = 0;
	} else if (g_terminal_type.equalsIgnoreCase("dumb")) {
		g_terminal_rows = 0;
		g_terminal_cols = 80;
		if (tt_s1 != NULL) g_terminal_rows = atoi(tt_s1);
		if (tt_s2 != NULL) g_terminal_cols = atoi(tt_s2);
	} else {
		g_terminal_rows = 24;
		g_terminal_cols = 80;
		if (tt_s1 != NULL) g_terminal_rows = atoi(tt_s1);
		if (tt_s2 != NULL) g_terminal_cols = atoi(tt_s2);
	}
}

void init_terminal(ConnectionType conn_type) {

	// Not yet connected
	if (conn_type == CONN_NONE) {
		TERM_BASE *new_term = new TERM_BASE();
		delete g_terminal;
		g_terminal = new_term;
		return;
	}

	// Raw connection
	if (g_terminal_type.equalsIgnoreCase("none")) {
		TERM_BASE *new_term = new TERM_BASE();
		delete g_terminal;
		g_terminal = new_term;
		return;
	}

	// LSI ADM-3A connected as a VT100 over telnet
	if (g_terminal_type == "adm3a-ansi") {
		TERM_BASE *new_term = new LSI_ADM3A();
		delete g_terminal;
		g_terminal = new_term;
		return;
	}

	// Standard telnet connection
	if (g_terminal_type.equalsIgnoreCase("dumb")) {
		g_terminal_rows = 0;
		if (g_terminal_cols < 1) g_terminal_cols = 80;
	} else {
		if (g_terminal_rows < 1) g_terminal_rows = 24;
		if (g_terminal_cols < 1) g_terminal_cols = 80;
	}
	TERM_BASE *new_term = new TERM_TELNET(g_terminal_type, g_terminal_rows, g_terminal_cols);
	delete g_terminal;
	g_terminal = new_term;

}
