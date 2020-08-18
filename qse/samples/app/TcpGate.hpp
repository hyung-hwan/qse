#ifndef _TCPGATE_CLASS_
#define _TCPGATE_CLASS_

#include <qse/si/TcpServer.hpp>
#include <qse/cmn/String.hpp>

class MainApp;

class TcpGate: public QSE::TcpServer, public QSE::Thread
{
public:
	TcpGate (MainApp* app): app(app)
	{
	}

	void setBindAddress (const qse_char_t* bind_address)
	{
		this->bind_address = bind_address;
	}

	int main ();
	int stop () QSE_CPP_NOEXCEPT;

protected:
	MainApp* app;
	bool stop_requested;
	QSE::String bind_address;

	int handle_connection (Connection* connection);
	void logfmt (qse_log_priority_flag_t pri, const qse_char_t* fmt, ...);
};

#endif
