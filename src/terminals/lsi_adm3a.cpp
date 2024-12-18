#include <Arduino.h>
#include "lsi_adm3a.h"
#include "../term_ansi.h"

LSI_ADM3A::LSI_ADM3A(const String& term_type, int rows, int cols) : TERM_ANSI(term_type, rows, cols) {
	rt = new char *[rows];
	rt_rows = rows;
	rt_cols = cols;
	for (int i = 0; i < rows; i++)
		rt[i] = new char[cols];
}

LSI_ADM3A::~LSI_ADM3A() {
	if (rt) {
		for (int i = 0; i < rt_rows; i++)
			delete[] rt[i];
		delete[] rt;
	}
}

void LSI_ADM3A::rt_BEL() {
	Serial.print('\007');
}

void LSI_ADM3A::rt_home_cursor() {
	Serial.printf("\036");
	rt_y = 1;
	rt_x = 1;	
}

// To make things more efficient, this function
// uses relative cursor movement. That only works if the
// physical cursor is where rt_y, rt_x thinks it is.
// If you want to call this when you don't know where
// the physical cursor might be, call rt_home_cursor()
// first.
void LSI_ADM3A::rt_update_cursor(int y, int x) {
	if (y < 1) y = 1;
	if (x < 1) x = 1;
	if (y > rt_rows) y = rt_rows;
	if (x > rt_cols) x = rt_cols;

	// define a 2D array of methods
	int methods[4][2] = {
		{4, 0},                            // load cursor
		{1 + abs(1-y) + abs(1-x), 1},      // home the cursor first
		{1 + abs(rt_y-y) + abs(1-x), 2},   // carriage return first
		{abs(rt_y-y) + abs(rt_x-x), 3}     // one position at a time
	};

	// find method with least number of moves
	int best_method = 0;
	for (int i = 1; i < 4; i++) {
		if (methods[i][0] < methods[best_method][0]) {
			best_method = i;
		}
	}

	// execute the best method
	if (best_method == 0)
		// load cursor
		Serial.printf("\033=%c%c", 32+(y-1), 32+(x-1));
	else {
		if (best_method == 1) {
			// ^^ home the cursor first
			Serial.printf("\036");
			rt_y = 1;
			rt_x = 1;
		} else if (best_method == 2) {
			// ^M carriage return first
			Serial.printf("\015");
			rt_x = 1;
		}
		// use ^J/^K to move to y
		for (int i=0; i<abs(rt_y-y); i++)
			Serial.printf((y < rt_y) ? "\013" : "\012");
		// use ^H/^L to move to x
		for (int i=0; i<abs(rt_x-x); i++)
			Serial.printf((x < rt_x) ? "\010" : "\014");
	}

	rt_y = y;
	rt_x = x;
}

// When this gets called, vt has been updated but rt has not,
// so the idea is to update rt and print the changes to the
// terminal efficiently.
void LSI_ADM3A::rt_update(int fr_y, int fr_x, int to_y, int to_x) {
	int save_y = rt_y;
	int save_x = rt_x;
	rt_update_cursor(fr_y, fr_x);
	int y = rt_y;
	int x = rt_x;
	while (true) {
		if (rt[y-1][x-1] != get_vt_char(y-1, x-1)) {
			rt[y-1][x-1] = get_vt_char(y-1, x-1);
			if (y == rt_rows && x == rt_cols) {
				; // print would cause adm-3a to scroll
			} else {
				rt_update_cursor(y, x);
				Serial.printf("%c", rt[y-1][x-1]);
				if (rt_x < rt_cols)
					rt_x++;
				else {
					rt_y++;
					rt_x = 1;
				}
			}
		}
		if (y == to_y && x == to_x)
			break;
		if (x < rt_cols)
			x++;
		else {
			y++;
			x = 1;
		}
		yield();
	}
	rt_update_cursor(save_y, save_x);
}

// Clears a defined region of the screen.
// Cursor position is unchanged.
void LSI_ADM3A::rt_clear(int fr_y, int fr_x, int to_y, int to_x) {
	if (fr_y == 1 && fr_x == 1 && to_y == rt_rows && to_x == rt_cols) {
		for (int row=1; row<=rt_rows; row++)
			for (int col=1; col<=rt_cols; col++)
				rt[row-1][col-1] = get_vt_char(row-1, col-1);
		Serial.print("\032");
		// at 19200 baud adm-3a drops characters
		// after CLS - this delay seems to fix it.
		delay(100);
		rt_update_cursor(rt_y, rt_x);
	} else {
		rt_update(fr_y, fr_x, to_y, to_x);
	}
}


// Scrolls a defined region of the screen.
// Cursor position is unchanged.
void LSI_ADM3A::rt_scroll(int fr_y, int to_y, int n) {
	if (fr_y == 1 && to_y == rt_rows && n > 0) {
		// update rt
		for (int row=1; row<=rt_rows; row++)
			for (int col=1; col<=rt_cols; col++)
				rt[row-1][col-1] = get_vt_char(row-1, col-1);
		// let the terminal scroll the screen
		int save_y = rt_y;
		rt_update_cursor(rt_rows, 1);
		for (int i=0; i<n; i++)
			Serial.printf("\012");
		rt_update_cursor(save_y, rt_x);
	} else {
		rt_update(fr_y, 1, to_y, rt_cols);
	}
}

// Prints one character.
// Cursor position is updated.
void LSI_ADM3A::rt_print(char c) {

	rt[rt_y-1][rt_x-1] = c;

	if (rt_x == rt_cols && rt_y == rt_rows) {
		// print would cause adm-3a to scroll
		// maybe need to add an option in case dip switch is set to prevent scrolling
	} else {
		Serial.print(c);
		if (rt_x < rt_cols)
			rt_x++;
		else {
			rt_y++;
			rt_x = 1;
			rt_update_cursor(rt_y-1, rt_cols);
		}
	}

}
