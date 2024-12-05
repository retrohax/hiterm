#pragma once

class NATIVE : public Terminal {
	public:
		NATIVE(const char* term_type="", int rows=0, int cols=0);

	protected:
		void rt_print(char c);	
};
