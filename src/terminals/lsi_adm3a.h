#pragma once
#include "../term_ansi.h"

class LSI_ADM3A : public TERM_ANSI {
	public:
		LSI_ADM3A(const String& term_type, int rows, int cols);
		~LSI_ADM3A() override;

	private:
		char **rt;
		int rt_rows;
		int rt_cols;
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
