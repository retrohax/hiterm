#pragma once
#include <WiFi.h>
#include <WiFiClientSecure.h>

class SSHClient;  // Forward declaration

enum ConnectionType {
    CONN_NONE,
	CONN_ESTABLISHED,
    CONN_SSH
};

class Host {
	public:
		Host();
		virtual ~Host() = default;

		static const int TX_HIST_MAXLEN = 1000;
		static const int RX_HIST_MAXLEN = 2048;

		void connect(String host, int port, bool use_tls=false);
		void connect_ssh(String host, int port, String user, String password);
		void shutdown();
		bool connected();
		bool available();
		bool is_ssh_connection();

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
		void show_status();
		void toggle_crlf();
		void show_rx_hist();
		void show_tx_hist();

		void keepalive();

		// Connection timing access
		void update_data_received_time() { last_data_received_time = millis(); }
		unsigned long get_last_data_received_time() const { return last_data_received_time; }

		// Connection type access
		ConnectionType get_connection_type() const { return conn_type; }

		Client *client = nullptr;

	private:
		String host_name = "";
		String line_end = "\r\n";
		ConnectionType conn_type = CONN_NONE;
		bool local_echo = false;
		int flow_mode = 1;

		// Connection timing for keepalive
		unsigned long last_data_received_time = 0;

		char g_tx_hist[TX_HIST_MAXLEN];
		char g_rx_hist[RX_HIST_MAXLEN];
		int g_tx_hist_index;
		int g_rx_hist_index;

		void save_rx_char(char c);
		void save_tx_char(char c);
};

extern Host *g_host;
