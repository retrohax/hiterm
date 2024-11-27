#include "parser.h"
#include "terminal.h"

const int ANSI_HIST_MAXLEN = 184;
int g_ansi_hist_index;
String g_ansi_hist_strings[ANSI_HIST_MAXLEN];

void do_csi_mode(String cmd_str) {
	char last_char = cmd_str[cmd_str.length()-1];

	if (last_char == '?' && cmd_str == "\033[?") {
		g_terminal->event_pvt_mode = 1;
		return;
	}

	if (isdigit(last_char) || last_char == ';') {
		g_terminal->event_parms_str += last_char;
		return;
	}

	// if we've made it this far and encounter something
	// other than a final character: abort
	if (last_char < '\100') {
		g_terminal->event_reset();
		return;
	}

	// parse parameters
	parse_parm_str(
		g_terminal->event_parms_str,
		';',
		g_terminal->event_parms,
		g_terminal->event_parms_cnt,
		g_terminal->EVENT_MAX_PARMS);

	// all that remains are final characters
	char final_char = last_char;

	switch (final_char) {
		case 'A':
			g_terminal->dec_CUU();
			break;
		case 'B':
			g_terminal->dec_CUD();
			break;
		case 'C':
			g_terminal->dec_CUF();
			break;
		case 'D':
			g_terminal->dec_CUB();
			break;
		case 'H':
			g_terminal->dec_CUP();
			break;
		case 'J':
			g_terminal->dec_ED();
			break;
		case 'K':
			g_terminal->dec_EL();
			break;
		case 'c':
			g_terminal->dec_DA();
			break;
		case 'f':
			g_terminal->dec_CUP();
			break;
		case 'h':
			g_terminal->dec_SM();
			break;
		case 'l':
			g_terminal->dec_RM();
			break;
		case 'm':
			g_terminal->dec_SGR();
			break;
		case 'n':
			g_terminal->dec_DSR();
			break;
		case 'r':
			g_terminal->dec_DECSTBM();
			break;
	}

	save_ansi(g_terminal->event_cmd_str);
	g_terminal->event_reset();
}

void do_esc_mode(String cmd_str) {
	if (g_terminal->event_csi_mode) {
		do_csi_mode(cmd_str);
		return;
	}

	// we're in an escape sequence

	char last_char = cmd_str[cmd_str.length()-1];

	// from vt100 user guide appendix A (ANSI)
	// intermediate characters are bit combination
	// '\040' to '\057' inclusive
	if (last_char >= '\040' and last_char <= '\057') {
		g_terminal->event_int_str += last_char;
		return;
	}

	// '\000' to '\037' control chars previously handled
	// '\040' to '\057' intermediate chars previously handled
	// '\177' DEL previously ignored
	// 8-bit chars (> '\177') previously ignored

	// final characters are bit combinations '\060' to
	// '\176' inclusive in escape sequences

	// all that remains are final characters

	char final_char = last_char;

	if (g_terminal->event_int_str == "") {
		// no intermediate chars
		if (final_char == '[') {
			g_terminal->event_csi_mode = true;
			return;
		}
		switch (final_char) {
			case '7':
				g_terminal->dec_DECSC();
				break;
			case '8':
				g_terminal->dec_DECRC();
				break;
			case 'D':
				g_terminal->dec_IND();
				break;
			case 'E':
				g_terminal->dec_NEL();
				break;
			case 'M':
				g_terminal->dec_RI();
				break;
			case 'c':
				g_terminal->reset();
				break;			
		}
	} else {
		// intermediate chars
	}

	save_ansi(g_terminal->event_cmd_str);
	g_terminal->event_reset();
}

void parse_char(char c) {

	if (!g_ansi_mode) {
		g_terminal->print(c);
		return;
	}

	if (c <= '\037' || c == '\177') {
		switch(c) {
			case '\007':
				g_terminal->dec_BEL();
				break;
			case '\010':
				g_terminal->dec_BS();
				break;
			case '\011':
				g_terminal->dec_HT();
				break;
			case '\012':
			case '\013':
			case '\014':
				g_terminal->dec_LF();
				break;
			case '\015':
				g_terminal->dec_CR();
				break;
			case '\016':
				g_terminal->dec_shift_out(true);
				break;
			case '\017':
				g_terminal->dec_shift_out(false);
				break;
			case '\032':
				g_terminal->dec_SUB();
			case '\030':
				g_terminal->event_reset();
				break;
			case '\033':
				g_terminal->event_reset();
				g_terminal->event_cmd_str = '\033';
				g_terminal->event_esc_mode = true;
				break;
		}
		return;
	}

	if (g_terminal->event_esc_mode) {
		g_terminal->event_cmd_str += c;
		do_esc_mode(g_terminal->event_cmd_str);
		return;
	}

	g_terminal->print(c);
}

void parse_parm_str(String str, char delimiter, int results[], int &count, int max_parts) {
	count = 0;
	int s0 = 0;
	int s1 = 0;
	while ((s1 = str.indexOf(delimiter, s0)) != -1) {
		results[count++] = str.substring(s0, s1).toInt();
		s0 = s1 + 1;
		if (count >= max_parts - 1) break;
	}
	if (s0 < str.length()) {
		results[count++] = str.substring(s0).toInt();
	}
}

void parser_init() {
	g_terminal->event_reset();
	for (int i=0; i<ANSI_HIST_MAXLEN; i++)
		g_ansi_hist_strings[i] = "";
	g_ansi_hist_index = 0;
}

void save_ansi(String ansi_str) {
	g_ansi_hist_strings[g_ansi_hist_index++] = ansi_str.substring(1);
	if (g_ansi_hist_index > ANSI_HIST_MAXLEN-1)
		g_ansi_hist_index = 0;
}

void show_ansi() {
	for (int i=0; i<ANSI_HIST_MAXLEN; i++) {
		String ansi_str = g_ansi_hist_strings[g_ansi_hist_index];
		int str_len = ansi_str.length();
		g_ansi_hist_index++;
		if (g_ansi_hist_index > ANSI_HIST_MAXLEN-1)
			g_ansi_hist_index = 0;
		if (str_len == 0) continue;
		for (int j=0; j<10; j++)
			if (j < str_len)
				if (ansi_str[j] < '\040')
					Serial.print('?');
				else
					Serial.print(ansi_str[j]);
			else
				Serial.print(' ');
		yield();
	}
}
