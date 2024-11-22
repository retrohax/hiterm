#pragma once
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

class Host {
	public:
		Host();
		virtual ~Host();

		static const int TX_HIST_MAXLEN = 1000;
		static const int RX_HIST_MAXLEN = 2048;

		void tcp_connect(String host, int port);
		void tls_connect(String host, int port);
		void shutdown();
		bool connected();
		bool available();

		char get();
		void send(char c, bool send_ansi_sequence=false);
		char read();
		void write(char c);
		void writef(const char* format, ...);

		void set_local_echo(String str, bool quiet=false);
		void set_flow_mode(int flow_mode);
		void toggle_crlf();
		void show_rx_hist(String find_str="");
		void save_rx_hist();
		void replay_rx_hist();
		void show_tx_hist();

		Client *client;

	private:
		char g_tx_hist[TX_HIST_MAXLEN];
		char g_rx_hist[RX_HIST_MAXLEN];
		int g_tx_hist_index;
		int g_rx_hist_index;

		String line_end = "\r\n";
		bool local_echo = true;
		int flow_mode = 1;

		void save_rx_char(char c);
		void save_tx_char(char c);
};

extern Host *g_host;
