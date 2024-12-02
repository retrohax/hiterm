#pragma once

class NONE : public Terminal {
	public:
		NONE(const char* term_type);

	protected:
		void rt_print(char c);
};
