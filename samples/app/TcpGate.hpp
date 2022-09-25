#ifndef _TCPGATE_CLASS_
#define _TCPGATE_CLASS_

#include <qse/si/TcpServer.hpp>
#include <qse/cmn/String.hpp>
#include <qse/cmn/HashTable.hpp>
#include <qse/sttp/Sttp.hpp>

class MainApp;

class TcpGateProto: public QSE::Sttp
{
public:
	typedef int (TcpGateProto::*CmdProc) (const QSE::SttpCmd& cmd);

	TcpGateProto (MainApp* app, QSE::Socket* sck);

	int sendCmd (const qse_mchar_t* cmd, qse_size_t nargs, ...);
	int sendCmd (const qse_wchar_t* cmd, qse_size_t nargs, ...);
	int sendErrCmd (ErrorNumber err);
	int sendErrCmd (ErrorNumber err, const qse_mchar_t* msg);
	int sendErrCmd (ErrorNumber err, const qse_wchar_t* msg);
	int sendWarnCmd (ErrorNumber err);
	int sendWarnCmd (ErrorNumber err, const qse_mchar_t* msg);
	int sendWarnCmd (ErrorNumber err, const qse_wchar_t* msg);
	int sendOkCmd ();
	int sendOkCmd (const qse_mchar_t* msg1);
	int sendOkCmd (const qse_mchar_t* msg1, const qse_mchar_t* msg2);
	int sendOkCmd (const qse_wchar_t* msg1);
	int sendOkCmd (const qse_wchar_t* msg1, const qse_wchar_t* msg2);

protected:
	MainApp* app;
	QSE::Socket* sck;

	typedef QSE::HashTable<QSE::String, CmdProc> CmdDict;
	CmdDict cmd_dict;

	int opt_autosave;

	int handle_command (const QSE::SttpCmd& cmd);
	int write_bytes (const qse_uint8_t* data, qse_size_t len);

	int proc_discon (const QSE::SttpCmd& cmd);
	int proc_shtdwn (const QSE::SttpCmd& cmd);
	int proc_envlst (const QSE::SttpCmd& cmd);
	int proc_envget (const QSE::SttpCmd& cmd);
	int proc_envset (const QSE::SttpCmd& cmd);
	int proc_envsav (const QSE::SttpCmd& cmd);
	int proc_optget (const QSE::SttpCmd& cmd);
	int proc_optset (const QSE::SttpCmd& cmd);
};

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
