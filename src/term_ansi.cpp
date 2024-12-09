#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "host.h"
#include "command.h"
#include "eeprom.h"
#include "term_telnet.h"
#include "term_ansi.h"

char dec_special_graphics(char c);

TERM_ANSI::TERM_ANSI(const String& term_type, int rows, int cols) : TERM_TELNET("vt100", rows, cols) {
	vt_term_type = term_type;
	vt_rows = rows;
	vt_cols = cols;
	vt = new char *[vt_rows];
	for (int i=0; i<vt_rows; i++)
		vt[i] = new char[vt_cols];
	m_send_ansi_mode = false;
	event_reset();
}

TERM_ANSI::~TERM_ANSI() {
	if (vt) {
		for (int i = 0; i < vt_rows; i++)
			delete[] vt[i];
		delete[] vt;
	}
}

void TERM_ANSI::show_term_type() {
	Serial.printf(
		"Terminal type is %s (ROWS=%d, COLS=%d).\r\n",
		vt_term_type.c_str(),
		vt_rows,
		vt_cols
	);
}

void TERM_ANSI::reset() {
	vt_home_cursor();
	vt_clear(1, 1, vt_rows, vt_cols);
	vt_save_y = vt_y;
	vt_save_x = vt_x;
	vt_margin_top = 1;
	vt_margin_bot = vt_rows;
	vt_origin_mode = false;
	vt_line_mode = false;
	vt_insert_mode = false;
	vt_bold_mode = false;
	vt_send_ansi_mode = false;
	vt_shift_out = false;
	vt_save_origin_mode = false;
	vt_save_bold_mode = false;
}

void TERM_ANSI::event_reset() {
	event_esc_mode = false;
	event_csi_mode = false;
	event_pvt_mode = 0;
	event_cmd_str = "";
	event_int_str = "";
	event_parms_str = "";
	event_parms_cnt = 0;
	for (int i=0; i<EVENT_MAX_PARMS; i++)
		event_parms[i] = 0;
}

/*
	DEC VT100 METHODS
*/

void TERM_ANSI::dec_CUU() {
	int n;
	if (event_parms[0] == 0) n = 1;
	else n = event_parms[0];
	if (vt_y >= vt_margin_top) {
		if (vt_y - n >= vt_margin_top)
			vt_y = vt_y - n;
		else
			vt_y = vt_margin_top;
	} else {
		if (vt_y - n >= 1)
			vt_y = vt_y - n;
		else
			vt_y = 1;
	}
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_CUD() {
	int n;
	if (event_parms[0] == 0) n = 1;
	else n = event_parms[0];
	if (vt_y <= vt_margin_bot) {
		if (vt_y + n <= vt_margin_bot)
			vt_y = vt_y + n;
		else
			vt_y = vt_margin_bot;
	} else {
		if (vt_y + n <= vt_rows)
			vt_y = vt_y + n;
		else
			vt_y = vt_rows;
	}
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_CUF() {
	int n;
	if (event_parms[0] == 0) n = 1;
	else n = event_parms[0];
	if (vt_x + n <= vt_cols)
		vt_x = vt_x + n;
	else
		vt_x = vt_cols;
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_CUB() {
	int n;
	if (event_parms[0] == 0) n = 1;
	else n = event_parms[0];
	if (vt_x - n >= 1)
		vt_x = vt_x - n;
	else
		vt_x = 1;
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_CUP() {
	int y = event_parms[0];
	int x = event_parms[1];
	if (y == 0) y = 1;
	if (x == 0) x = 1;
	if (vt_origin_mode) {
		if ((vt_margin_top-1) + y <= vt_margin_bot)
			vt_y = (vt_margin_top-1) + y;
		else
			vt_y = vt_margin_bot;
	} else {
		if (y <= vt_rows)
			vt_y = y;
		else
			vt_y = vt_rows;
	}
	if (x <= vt_cols)
		vt_x = x;
	else
		vt_x = vt_cols;
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_CPR() {
	int y;
	if (vt_origin_mode)
		y = vt_y - vt_margin_top + 1;
	else
		y = vt_y;
	g_host->writef("\033[%d;%dR", y, vt_x);
}

void TERM_ANSI::dec_DSR() {
	switch (event_parms[0]) {
		case 5:
			g_host->writef("\033[0n");
		case 6:
			dec_CPR();
	}
}

void TERM_ANSI::dec_DECSC() {
	vt_save_y = vt_y;
	vt_save_x = vt_x;
	vt_save_bold_mode = vt_bold_mode;
	vt_save_origin_mode = vt_origin_mode;
}

void TERM_ANSI::dec_DECRC() {
	if (vt_save_x > vt_cols)
		vt_x = vt_cols;
	else
		vt_x = vt_save_x;
	vt_y = vt_save_y;
	vt_bold_mode = vt_save_bold_mode;
	vt_origin_mode = vt_save_origin_mode;
	if (vt_origin_mode) {
		if (vt_y < vt_margin_top)
			vt_y = vt_margin_top;
		if (vt_y > vt_margin_bot)
			vt_y = vt_margin_bot;

	}
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_IND() {
	if (vt_y == vt_margin_bot)
		vt_scroll_up();
	else if (vt_y < vt_rows)
		vt_y++;
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_RI() {
	if (vt_y == vt_margin_top)
		vt_scroll_down();
	else if (vt_y > 1)
		vt_y--;
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_NEL() {
	if (vt_y == vt_margin_bot)
		vt_scroll_up();
	else if (vt_y < vt_rows) {
		vt_y++;
		vt_x = 1;
	}
	else
		vt_x = 1;
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_EL() {
	switch (event_pvt_mode) {
		case 0:
			if (event_parms_cnt == 0) event_parms_cnt = 1;
			for (int n=0; n<event_parms_cnt; n++)
				switch (event_parms[n]) {
					case 0:
						vt_clear(vt_y, vt_x, vt_y, TERM_MAX_X);
						break;
					case 1:
						vt_clear(vt_y, 1, vt_y, vt_x);
						break;
					case 2:
						vt_clear(vt_y, 1, vt_y, TERM_MAX_X);
						break;
				}
			break;
		case 1:
			break;
	}
}

void TERM_ANSI::dec_ED() {
	switch (event_pvt_mode) {
		case 0:
			if (event_parms_cnt == 0) event_parms_cnt = 1;
			for (int n=0; n<event_parms_cnt; n++)
				switch (event_parms[n]) {
					case 0:
						vt_clear(vt_y, vt_x, TERM_MAX_Y, TERM_MAX_X);
						break;
					case 1:
						vt_clear(1, 1, vt_y, vt_x);
						break;
					case 2:
						vt_clear(1, 1, TERM_MAX_Y, TERM_MAX_X);
						break;
				}
			break;
		case 1:
			break;
	}
}

void TERM_ANSI::dec_DECSTBM() {
	if (event_parms[0] == 0) event_parms[0] = 1;
	if (event_parms[1] == 0) event_parms[0] = vt_rows;
	if (event_parms[0] < event_parms[1] && event_parms[1] <= vt_rows) {
		vt_margin_top = event_parms[0];
		vt_margin_bot = event_parms[1];
		vt_x = 1;
		if (!vt_origin_mode) vt_y = 1;
		else vt_y = vt_margin_top;
	}
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_DA() {
	// "no options"
	if (event_parms[0] == 0)
		g_host->writef("\033[?1;0c");
}

void TERM_ANSI::dec_SM() {
	for (int i=0; i<event_parms_cnt; i++)
		if (event_pvt_mode) {
			switch (event_parms[i]) {
				case 0:
					// ERROR - ignored
					break;
				case 6:
					// DECOM - origin
					vt_origin_mode = true;
					vt_y = vt_margin_top;
					vt_x = 1;
					break;
			}
		} else {
			switch (event_parms[i]) {
				case 0:
					// ERROR - ignored
					break;
				case 20:
					// LNM - line feed new line mode
					vt_line_mode = true;
					break;
			}
		}
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_RM() {
	for (int i=0; i<event_parms_cnt; i++)
		if (event_pvt_mode) {
			switch (event_parms[i]) {
				case 0:
					// ERROR - ignored
					break;
				case 6:
					// DECOM - origin
					vt_origin_mode = false;
					vt_y = 1;
					vt_x = 1;
					break;
			}
		} else {
			switch (event_parms[i]) {
				case 0:
					// ERROR - ignored
					break;
				case 20:
					// LNM - line feed new line mode
					vt_line_mode = false;
					break;
			}
		}
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_SGR() {
	if (event_parms_cnt == 0) {
		// attributes off
		vt_bold_mode = false;
		return;
	}
	for (int i=0; i<event_parms_cnt; i++)
		switch (event_parms[i]) {
			case 0:
				// attributes off
				vt_bold_mode = false;
				break;
			case 1:
				// bold or increased intensity
				vt_bold_mode = true;
				break;
		}
}

void TERM_ANSI::dec_BEL() {
	rt_BEL();
}

void TERM_ANSI::dec_BS() {
	if (vt_x > 1) vt_x--;
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_HT() {
}

void TERM_ANSI::dec_LF() {
	if (vt_line_mode)
		vt_x = 1;
	if (vt_y == vt_margin_bot)
		vt_scroll_up();
	else if (vt_y < vt_rows)
		vt_y++;
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_CR() {
	vt_x = 1;
	vt_update_cursor(vt_y, vt_x);
}

void TERM_ANSI::dec_SUB() {
	vt_print('?');
}

void TERM_ANSI::dec_shift_out(bool mode) {
	vt_shift_out = mode;
}

/*
	I/O METHODS
*/

void TERM_ANSI::print(char c) {
	if (!telnet_char(c) && !ansi_char(c))
		vt_print(c);
}

/*
	VIRTUAL TERMINAL METHODS
*/

void TERM_ANSI::vt_print(char c) {

	// printable characters only
	if (c < '\040' || c == '\177')
		return;

	// can't print extended ascii
	if (c > '\177')
		c = '?';

	if (vt_shift_out)
		c = dec_special_graphics(c);
	else if (vt_bold_mode && (c >= 'a' and c <= 'z'))
		c -= 32;

	vt[vt_y-1][vt_x-1] = c;
	if (vt_x < vt_cols)
		vt_x++;

	rt_print(c);
}

void TERM_ANSI::vt_home_cursor() {
	vt_y = 1;
	vt_x = 1;
	rt_home_cursor();
}

void TERM_ANSI::vt_update_cursor(int y, int x) {
	rt_update_cursor(y, x);
}

void TERM_ANSI::vt_clear(int fr_y, int fr_x, int to_y, int to_x) {
	if (to_y > vt_rows) to_y = vt_rows;
	if (to_x > vt_cols) to_x = vt_cols;
	if (fr_y < 1) fr_y = 1;
	if (fr_x < 1) fr_x = 1;
	if (fr_y > to_y) return;
	if (fr_x > to_x) return;

	int y = fr_y;
	int x = fr_x;

	while (true) {
		vt[y-1][x-1] = ' ';
		if (y == to_y && x == to_x)
			break;
		if (x < vt_cols)
			x++;
		else {
			x = 1;
			y++;
		}
	}

	rt_clear(fr_y, fr_x, to_y, to_x);
}

void TERM_ANSI::vt_scroll_up() {
	for (int i=vt_margin_top; i<=vt_margin_bot; i++)
		for (int j=1; j<=vt_cols; j++)
			vt[i-1][j-1] = (i<vt_margin_bot) ? vt[(i+1)-1][j-1] : ' ';

	rt_scroll(vt_margin_top, vt_margin_bot, 1);
}

void TERM_ANSI::vt_scroll_down() {
	for (int i=vt_margin_bot; i>=vt_margin_top; i--)
		for (int j=1; j<=vt_cols; j++)
			vt[i-1][j-1] = (i>vt_margin_top) ? vt[(i-1)-1][j-1] : ' ';

	rt_scroll(vt_margin_top, vt_margin_bot, -1);
}

int TERM_ANSI::get_vt_char(int y, int x) {
	return vt[y][x];
}

/*
	ANSI PARSING
*/

void TERM_ANSI::do_csi_mode(String cmd_str) {
	char last_char = cmd_str[cmd_str.length()-1];

	if (last_char == '?' && cmd_str == "\033[?") {
		event_pvt_mode = 1;
		return;
	}

	if (isdigit(last_char) || last_char == ';') {
		event_parms_str += last_char;
		return;
	}

	// if we've made it this far and encounter something
	// other than a final character: abort
	if (last_char < '\100') {
		event_reset();
		return;
	}

	// parse parameters
	parse_parm_str(
		event_parms_str,
		';',
		event_parms,
		event_parms_cnt,
		EVENT_MAX_PARMS);

	// all that remains are final characters
	char final_char = last_char;

	switch (final_char) {
		case 'A':
			dec_CUU();
			break;
		case 'B':
			dec_CUD();
			break;
		case 'C':
			dec_CUF();
			break;
		case 'D':
			dec_CUB();
			break;
		case 'H':
			dec_CUP();
			break;
		case 'J':
			dec_ED();
			break;
		case 'K':
			dec_EL();
			break;
		case 'c':
			dec_DA();
			break;
		case 'f':
			dec_CUP();
			break;
		case 'h':
			dec_SM();
			break;
		case 'l':
			dec_RM();
			break;
		case 'm':
			dec_SGR();
			break;
		case 'n':
			dec_DSR();
			break;
		case 'r':
			dec_DECSTBM();
			break;
	}

	event_reset();
}

void TERM_ANSI::do_esc_mode(String cmd_str) {
	if (event_csi_mode) {
		do_csi_mode(cmd_str);
		return;
	}

	// we're in an escape sequence

	char last_char = cmd_str[cmd_str.length()-1];

	// from vt100 user guide appendix A (ANSI)
	// intermediate characters are bit combination
	// '\040' to '\057' inclusive
	if (last_char >= '\040' and last_char <= '\057') {
		event_int_str += last_char;
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

	if (event_int_str == "") {
		// no intermediate chars
		if (final_char == '[') {
			event_csi_mode = true;
			return;
		}
		switch (final_char) {
			case '7':
				dec_DECSC();
				break;
			case '8':
				dec_DECRC();
				break;
			case 'D':
				dec_IND();
				break;
			case 'E':
				dec_NEL();
				break;
			case 'M':
				dec_RI();
				break;
			case 'c':
				reset();
				break;			
		}
	} else {
		// intermediate chars
	}

	event_reset();
}

bool TERM_ANSI::ansi_char(char c) {

	if (c <= '\037' || c == '\177') {
		switch(c) {
			case '\007':
				dec_BEL();
				break;
			case '\010':
				dec_BS();
				break;
			case '\011':
				dec_HT();
				break;
			case '\012':
			case '\013':
			case '\014':
				dec_LF();
				break;
			case '\015':
				dec_CR();
				break;
			case '\016':
				dec_shift_out(true);
				break;
			case '\017':
				dec_shift_out(false);
				break;
			case '\032':
				dec_SUB();
			case '\030':
				event_reset();
				break;
			case '\033':
				event_reset();
				event_cmd_str = '\033';
				event_esc_mode = true;
				break;
		}
		return true;
	}

	if (event_esc_mode) {
		event_cmd_str += c;
		do_esc_mode(event_cmd_str);
		return true;
	}

	return false;
}

void TERM_ANSI::parse_parm_str(String str, char delimiter, int results[], int &count, int max_parts) {
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

/*
	DEBUG METHODS
*/

void TERM_ANSI::show_vars() {
	Serial.printf("vt_rows=%d, vt_cols=%d\r\n", vt_rows, vt_cols);
	Serial.printf("vt_y=%d, vt_x=%d, vt_save_y=%d, vt_save_x=%d\r\n",
		vt_y,
		vt_x,
		vt_save_y,
		vt_save_x);
	Serial.printf("vt_margin_top=%d, vt_margin_bot=%d\r\n",
		vt_margin_top,
		vt_margin_bot);
	Serial.printf("vt_origin_mode=%s\r\n",
		(vt_origin_mode) ? "true" : "false");
	Serial.printf("vt_line_mode=%s\r\n",
		(vt_line_mode) ? "true" : "false");
	Serial.printf("vt_send_ansi_mode=%s\r\n",
		(vt_send_ansi_mode) ? "true" : "false");
	Serial.printf("vt_shift_out=%s\r\n",
		(vt_shift_out) ? "true" : "false");
	Serial.printf("vt_bold_mode=%s\r\n",
		(vt_bold_mode) ? "true" : "false");
}

/*
	UTILITY FUNCTIONS
*/

char dec_special_graphics(char c) {
	switch (c) {
		case '\x6a': ;
		case '\x6b': ;
		case '\x6c': ;
		case '\x6d': ;
		case '\x6e': ;
		case '\x74': ;
		case '\x75': ;
		case '\x76': ;
		case '\x77': return '+';
		case '\x71': return '-';
		case '\x78': return '|';
	}
	return '?';
}
