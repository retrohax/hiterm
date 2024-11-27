#include "terminal.h"
#include "terminals/none.h"
#include "terminals/lsi_adm3a.h"
#include "host.h"
#include "command.h"
#include "eeprom.h"

const int TERM_MAX_Y = std::numeric_limits<int>::max();
const int TERM_MAX_X = std::numeric_limits<int>::max();
const int SAVE_YX_MAXLEN = 100;

// Each terminal listed here needs a corresponding entry
// in init_terminal() or bad things will happen.
String ansi_terminals = "|adm3a|";

char dec_special_graphics(char c);

String g_telnet_term_type = "";
String g_term_type = "";
bool g_ansi_mode = false;

Terminal *g_terminal = nullptr;

Terminal::Terminal(int rows, int cols) {
	vt_rows = rows;
	vt_cols = cols;
	vt = new char *[vt_rows];
	for (int i=0; i<vt_rows; i++)
		vt[i] = new char[vt_cols];
}

Terminal::~Terminal() {
	for (int i=0; i<vt_rows; i++)
		delete[] vt[i];
	delete[] vt;
}

void Terminal::reset() {
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

void Terminal::event_reset() {
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

bool Terminal::available() {
	return (Serial.available()) ? true : false;
}

/*
	DEC VT100 METHODS
*/

void Terminal::dec_CUU() {
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

void Terminal::dec_CUD() {
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

void Terminal::dec_CUF() {
	int n;
	if (event_parms[0] == 0) n = 1;
	else n = event_parms[0];
	if (vt_x + n <= vt_cols)
		vt_x = vt_x + n;
	else
		vt_x = vt_cols;
	vt_update_cursor(vt_y, vt_x);
}

void Terminal::dec_CUB() {
	int n;
	if (event_parms[0] == 0) n = 1;
	else n = event_parms[0];
	if (vt_x - n >= 1)
		vt_x = vt_x - n;
	else
		vt_x = 1;
	vt_update_cursor(vt_y, vt_x);
}

void Terminal::dec_CUP() {
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

void Terminal::dec_CPR() {
	int y;
	if (vt_origin_mode)
		y = vt_y - vt_margin_top + 1;
	else
		y = vt_y;
	g_host->writef("\033[%d;%dR", y, vt_x);
}

void Terminal::dec_DSR() {
	switch (event_parms[0]) {
		case 5:
			g_host->writef("\033[0n");
		case 6:
			dec_CPR();
	}
}

void Terminal::dec_DECSC() {
	vt_save_y = vt_y;
	vt_save_x = vt_x;
	vt_save_bold_mode = vt_bold_mode;
	vt_save_origin_mode = vt_origin_mode;
}

void Terminal::dec_DECRC() {
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

void Terminal::dec_IND() {
	if (vt_y == vt_margin_bot)
		vt_scroll_up();
	else if (vt_y < vt_rows)
		vt_y++;
	vt_update_cursor(vt_y, vt_x);
}

void Terminal::dec_RI() {
	if (vt_y == vt_margin_top)
		vt_scroll_down();
	else if (vt_y > 1)
		vt_y--;
	vt_update_cursor(vt_y, vt_x);
}

void Terminal::dec_NEL() {
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

void Terminal::dec_EL() {
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

void Terminal::dec_ED() {
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

void Terminal::dec_DECSTBM() {
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

void Terminal::dec_DA() {
	// "no options"
	if (event_parms[0] == 0)
		g_host->writef("\033[?1;0c");
}

void Terminal::dec_SM() {
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

void Terminal::dec_RM() {
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

void Terminal::dec_SGR() {
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

void Terminal::dec_BEL() {
	rt_BEL();
}

void Terminal::dec_BS() {
	if (vt_x > 1) vt_x--;
	vt_update_cursor(vt_y, vt_x);
}

void Terminal::dec_HT() {
}

void Terminal::dec_LF() {
	if (vt_line_mode)
		vt_x = 1;
	if (vt_y == vt_margin_bot)
		vt_scroll_up();
	else if (vt_y < vt_rows)
		vt_y++;
	vt_update_cursor(vt_y, vt_x);
}

void Terminal::dec_CR() {
	vt_x = 1;
	vt_update_cursor(vt_y, vt_x);
}

void Terminal::dec_SUB() {
	vt_print('?');
}

void Terminal::dec_shift_out(bool mode) {
	vt_shift_out = mode;
}


/*
	I/O METHODS
*/

char Terminal::get() {
	char c = Serial.read();
	switch (c) {
		case '\005':
			// ^E (ECHO)
			g_host->toggle_local_echo();
			break;
		case '\021':
			// ^Q (XON)
			g_host->set_flow_mode(1);
			break;
		case '\023':
			// ^S (XOFF)
			g_host->set_flow_mode(0);
			break;
		case '\030':
			// ^X
			if (vt_send_ansi_mode)
				vt_send_ansi_mode = false;
			else
				vt_send_ansi_mode = true;
			break;
		case '\034':
			/* ^\ */
			g_host->shutdown();
		case '\035':
			// ^]
			command();
			break;
		default:
			return c;
			break;
	}
	return '\0';
}

void Terminal::print(char c) {
	if (g_ansi_mode)
		vt_print(c);
	else
		rt_print(c);
}


/*
	VIRTUAL TERMINAL METHODS
*/

void Terminal::vt_print(char c) {

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

void Terminal::vt_home_cursor() {
	vt_y = 1;
	vt_x = 1;
	rt_home_cursor();
}

void Terminal::vt_update_cursor(int y, int x) {
	rt_update_cursor(y, x);
}

void Terminal::vt_clear(int fr_y, int fr_x, int to_y, int to_x) {
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

void Terminal::vt_scroll_up() {
	for (int i=vt_margin_top; i<=vt_margin_bot; i++)
		for (int j=1; j<=vt_cols; j++)
			vt[i-1][j-1] = (i<vt_margin_bot) ? vt[(i+1)-1][j-1] : ' ';

	rt_scroll(vt_margin_top, vt_margin_bot, 1);
}

void Terminal::vt_scroll_down() {
	for (int i=vt_margin_bot; i>=vt_margin_top; i--)
		for (int j=1; j<=vt_cols; j++)
			vt[i-1][j-1] = (i>vt_margin_top) ? vt[(i-1)-1][j-1] : ' ';

	rt_scroll(vt_margin_top, vt_margin_bot, -1);
}

int Terminal::get_vt_char(int y, int x) {
	return vt[y][x];
}


/*
	UTILITY METHODS
*/

uint16_t Terminal::get_rows() {
	return vt_rows;
}

uint16_t Terminal::get_cols() {
	return vt_cols;
}

void Terminal::show() {
	Serial.printf("\032");
	delay(100);
	for (int row=0; row<vt_rows; row++)
		for (int col=0; col<vt_cols; col++) {
			if (row == vt_rows && col == vt_cols)
				break;
			Serial.print(vt[row][col]);
			yield();
		}
	Serial.printf("\033=%c%c", 32+vt_y, 32+vt_x);
}

void Terminal::show_vars() {
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

bool Terminal::set_term_type(String term_type) {
	if (g_host->connected()) {
		Serial.println("Close the connection first.");
		return false;
	}
	g_term_type = term_type;
	write_eeprom(EEPROM_SYS3_ADDR, g_term_type);
	g_ansi_mode = (ansi_terminals.indexOf("|"+g_term_type+"|") >= 0);
	write_eeprom(EEPROM_SYS4_ADDR, g_ansi_mode ? "ON" : "OFF");
	return true;
}

void Terminal::show_term_type() {
	Serial.printf("Terminal type is %s.\r\n", (g_term_type == "") ? "none" : g_term_type);
}

bool Terminal::toggle_ansi_mode() {
	if (g_host->connected()) {
		Serial.println("Close the connection first.");
		return false;
	}
	if (!g_ansi_mode && ansi_terminals.indexOf("|"+g_term_type+"|") < 0) {
		Serial.println("Invalid terminal type.");
		return false;
	}
	g_ansi_mode = !g_ansi_mode;
	write_eeprom(EEPROM_SYS4_ADDR, g_ansi_mode ? "ON" : "OFF");
	return true;
}

void Terminal::show_ansi_mode() {
	if (g_ansi_mode)
		Serial.println("ANSI sequences will be handled by the terminal driver.");
	else
		Serial.println("ANSI sequences will be passed through to your terminal.");
}

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

void init_terminal() {
	if (g_terminal)
		delete g_terminal;
	if (g_ansi_mode && ansi_terminals.indexOf("|"+g_term_type+"|") >= 0) {
		g_telnet_term_type = "vt100";
		if (g_term_type == "adm3a") g_terminal = new LSI_ADM3A();
		return;
	}
	g_telnet_term_type = g_term_type;
	g_terminal = new NONE();
}
