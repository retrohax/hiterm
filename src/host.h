#pragma once
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

class Host {
	public:
		Host();
		virtual ~Host();

		static const int TX_HIST_MAXLEN = 1000;
		static const int RX_HIST_MAXLEN = 2048;

		void connect(String host, int port, bool use_tls=false);
		void shutdown();
		bool connected();
		bool available();

		char get();
		void send(char c, bool send_ansi_sequence=false);
		char read();
		void write(char c);
		void write_buffer(const char* data, int len);
		void writef(const char* format, ...);

		void show_local_echo();
		void set_local_echo(bool echo);
		void toggle_local_echo();
		void set_flow_mode(int flow_mode);
		void show_crlf();
		void toggle_crlf();
		void show_rx_hist(String find_str="");
		void save_rx_hist();
		void replay_rx_hist();
		void show_tx_hist();

		Client *client = nullptr;

	private:
		char g_tx_hist[TX_HIST_MAXLEN];
		char g_rx_hist[RX_HIST_MAXLEN];
		int g_tx_hist_index;
		int g_rx_hist_index;

		String line_end = "\r\n";
		bool local_echo = false;
		int flow_mode = 1;

		void save_rx_char(char c);
		void save_tx_char(char c);
};

extern Host *g_host;
