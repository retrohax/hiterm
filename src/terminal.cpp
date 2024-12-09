#include "terminal.h"
#include "host.h"
#include "term_base.h"
#include "term_telnet.h"
#include "terminals/lsi_adm3a.h"

TERM_BASE *g_terminal = new TERM_BASE();
size_t g_free_heap = ESP.getFreeHeap();

bool init_terminal(String term_type, int rows, int cols) {

    // Can't change terminal type while connected
    if (g_host->connected()) {
        Serial.println("Disconnect first.");
        return false;
    }

	// Terminal type is none
    if (term_type.isEmpty() || term_type.equalsIgnoreCase("NONE")) {
		delete g_terminal;
		g_terminal = new TERM_BASE();
		return true;
	}

	// Sanity check on terminal type
	bool has_alpha = false;
	for (char c : term_type) {
		if (isAlpha(c)) {
			has_alpha = true;
			break;
		}
	}
	if (!has_alpha) {
		Serial.println("Invalid terminal type.");
		return false;
	}

	if (term_type == "dumb") {
		rows = 0;
		if (cols < 1) cols = 80;
	} else {
		if (rows < 1) rows = 24;
		if (cols < 1) cols = 80;
	}

	if (term_type.endsWith("-ansi")) {
		// For ansi terminals we need to make sure there's enough memory for both arrays (vt and rt)
		size_t element_size = sizeof(char);
		size_t max_elements = (g_free_heap * 0.9) / (2 * element_size);
		if (rows * cols > max_elements) {
			Serial.printf("Terminal size exceeds available memory.\r\n");
			return false;
		}
		if (term_type == "adm3a-ansi") {
			delete g_terminal;
			g_terminal = new LSI_ADM3A(term_type, rows, cols);
			return true;
		}
		Serial.println("Invalid terminal type.");
		return false;
	}

	delete g_terminal;
	g_terminal = new TERM_TELNET(term_type, rows, cols);

	return true;
}
