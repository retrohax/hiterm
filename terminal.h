#pragma once
#include <Arduino.h>
#include <limits>

class Terminal {
	public:
		Terminal(int rows=0, int cols=0);
		virtual ~Terminal();

		void reset();
		void event_reset();
		bool available();

		void dec_CUU();
		void dec_CUD();
		void dec_CUF();
		void dec_CUB();
		void dec_CUP();
		void dec_DSR();
		void dec_DECSC();
		void dec_DECRC();
		void dec_IND();
		void dec_RI();
		void dec_NEL();
		void dec_ED();
		void dec_EL();

		void dec_CPR();
		void dec_DECSTBM();
		void dec_DA();
		void dec_SM();
		void dec_RM();
		void dec_SGR();

		void dec_BEL();
		void dec_BS();
		void dec_HT();
		void dec_LF();
		void dec_CR();
		void dec_SUB();

		void dec_shift_out(bool mode);

		char get();
		void print(char c);

		void show();
		void show_vars();
		uint16_t get_rows();
		uint16_t get_cols();

		static const int EVENT_MAX_PARMS = 100;

		bool event_esc_mode;
		bool event_csi_mode;
		int event_pvt_mode;
		String event_cmd_str = "";
		String event_int_str;
		String event_parms_str;
		int event_parms_cnt;
		int event_parms[EVENT_MAX_PARMS];

	protected:
		virtual void rt_home_cursor() {}
		virtual void rt_update_cursor(int y, int x) {}
		virtual void rt_clear(int fr_y, int fr_x, int to_y, int to_x) {}
		virtual void rt_scroll(int fr_y, int to_y, int n) {}
		virtual void rt_print(char c) {}
		virtual void rt_BEL() {}

		int get_vt_char(int y, int x);

	private:
		char **vt;
		uint16_t vt_rows;
		uint16_t vt_cols;
		int vt_y;                     // ACTIVE_POSITION.LINE
		int vt_x;                     // ACTIVE_POSITION.COLUMN
		int vt_margin_top;
		int vt_margin_bot;
		bool vt_origin_mode;
		bool vt_line_mode;
		bool vt_insert_mode;
		bool vt_bold_mode;
		bool vt_send_ansi_mode;
		bool vt_shift_out;
		int vt_save_y;
		int vt_save_x;
		bool vt_save_origin_mode;
		bool vt_save_bold_mode;

		void vt_print(char c);
		void vt_home_cursor();
		void vt_update_cursor(int y, int x);
		void vt_clear(int fr_y, int fr_x, int to_y, int to_x);
		void vt_scroll_up();
		void vt_scroll_down();
};

extern Terminal *g_terminal;
extern const int TERM_MAX_Y;
extern const int TERM_MAX_X;
extern String g_telnet_term_type;
extern String g_term_type;
extern String g_ansi_mode;

void set_term_type(String term_type);
void set_ansi_mode(String ansi_mode);
void init_terminal();
