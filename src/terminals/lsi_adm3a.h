#pragma once
#include "../term_ansi.h"

class LSI_ADM3A : public TERM_ANSI {
	public:
		LSI_ADM3A();
		~LSI_ADM3A() override;

	private:
		char **rt;
		static const int rt_rows = 24;
		static const int rt_cols = 80;
		int rt_y;
		int rt_x;

		void rt_BEL();
		void rt_home_cursor();
		void rt_update(int fr_y, int fr_x, int to_y, int to_x);
		void rt_update_cursor(int y, int x);
		void rt_clear(int fr_y, int fr_x, int to_y, int to_x);
		void rt_scroll(int fr_y, int to_y, int n);
		void rt_print(char c);
};
