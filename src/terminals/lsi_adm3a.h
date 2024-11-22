#pragma once

class LSI_ADM3A : public Terminal {
	public:
		LSI_ADM3A();
		virtual ~LSI_ADM3A();

	protected:
		static const int ROWS = 24;
		static const int COLS = 80;

		void rt_BEL();
		void rt_home_cursor();
		void rt_update_cursor(int y, int x);
		void rt_clear(int fr_y, int fr_x, int to_y, int to_x);
		void rt_scroll(int fr_y, int to_y, int n);
		void rt_print(char c);

	private:
		char **rt;
		int rt_y;
		int rt_x;

		void rt_update(int fr_y, int fr_x, int to_y, int to_x);
};
