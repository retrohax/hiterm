#pragma once
#include <Arduino.h>
#include <limits>
#include "term_telnet.h"

class TERM_ANSI : public TERM_TELNET {
	public:
		TERM_ANSI(const String& term_type, int rows, int cols);
		virtual ~TERM_ANSI() override;

		virtual void reset() override;
		virtual void print(char c) override;
		virtual void show_term_type() override;
		virtual void show_vars() override;

	protected:
		virtual void rt_home_cursor() {}
		virtual void rt_update_cursor(int y, int x) {}
		virtual void rt_clear(int fr_y, int fr_x, int to_y, int to_x) {}
		virtual void rt_scroll(int fr_y, int to_y, int n) {}
		virtual void rt_print(char c) {}
		virtual void rt_BEL() {}

		int get_vt_char(int y, int x);

        bool m_send_ansi_mode;

	private:
		char **vt;
		int vt_rows;
		int vt_cols;
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

		String vt_term_type;

		void vt_print(char c);
		void vt_home_cursor();
		void vt_update_cursor(int y, int x);
		void vt_clear(int fr_y, int fr_x, int to_y, int to_x);
		void vt_scroll_up();
		void vt_scroll_down();

		static const int EVENT_MAX_PARMS = 100;
		static const int TERM_MAX_Y = std::numeric_limits<int>::max();
		static const int TERM_MAX_X = std::numeric_limits<int>::max();

		bool event_esc_mode;
		bool event_csi_mode;
		int event_pvt_mode;
		String event_cmd_str = "";
		String event_int_str;
		String event_parms_str;
		int event_parms_cnt;
		int event_parms[EVENT_MAX_PARMS];

		void event_reset();
		void do_csi_mode(String cmd_str);
		void do_esc_mode(String cmd_str);
		bool ansi_char(char c);
		void parse_parm_str(String str, char delimiter, int results[], int &count, int max_parts);

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
};
