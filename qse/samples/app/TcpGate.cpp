#include "TcpGate.hpp"
#include "MainApp.hpp"
#include <qse/sttp/Sttp.hpp>
#include <qse/cmn/HashTable.hpp>
#include <qse/cmn/ErrorGrab.hpp>
#include <qse/cmn/fmt.h>

// --------------------------------------------------------------------------

#define CMD_OK   "OK"
#define CMD_ERR  "ERR"
#define CMD_WARN "WARN"

#define CMD_DISCON "DISCON"
#define CMD_SHTDWN "SHTDWN"
#define CMD_ENVGET "ENVGET"
#define CMD_ENVSET "ENVSET"
#define CMD_ENVLST "ENVLST"
#define CMD_ENVITM "ENVITM"

#define PROC_CHECK_ARG_COUNT(cmd,n) do { \
	if (cmd.getArgCount() != (n)) { \
		this->sendErrCmd (E_ENARGS, (const qse_mchar_t*)QSE_NULL); \
		return 0; \
	} \
} while(0)

#define PROC_CHECK_ARG_COUNT2(cmd,n1,n2) do { \
	if (cmd.getArgCount() != (n1) && cmd.getArgCount() != (n2)) { \
		this->sendErrCmd (E_ENARGS, (const qse_mchar_t*)QSE_NULL); \
		return 0; \
	} \
} while(0)

#define PROC_CHECK_ARG_COUNT_RANGE(cmd,n1,n2)  do { \
	if (cmd.getArgCount() < (n1) || cmd.getArgCount() > (n2)) { \
		this->sendErrCmd (E_ENARGS, (const qse_mchar_t*)QSE_NULL); \
		return 0; \
	} \
} while(0)


class Proto: public QSE::Sttp
{
public:
	typedef int (Proto::*CmdProc) (const QSE::SttpCmd& cmd);

	Proto (MainApp* app, QSE::Socket* sck): QSE::Sttp(), app(app), sck(sck)
	{
		this->cmd_dict.insert (QSE_T(CMD_DISCON), &Proto::proc_discon);
		this->cmd_dict.insert (QSE_T(CMD_SHTDWN), &Proto::proc_shtdwn);
		this->cmd_dict.insert (QSE_T(CMD_ENVLST), &Proto::proc_envlst);
		this->cmd_dict.insert (QSE_T(CMD_ENVGET), &Proto::proc_envget);
		this->cmd_dict.insert (QSE_T(CMD_ENVSET), &Proto::proc_envset);
	}

	int sendCmd (const qse_mchar_t* cmd, qse_size_t nargs, ...)
	{
		int x;
		va_list ap;

		va_start (ap, nargs);
		if (QSE_APP_LOG_ENABLED(this->app, QSE_LOG_INFO))
		{
			va_list xap;
			int h = (int)this->sck->getHandle();

			va_copy (xap, ap);

			// ugly - any better way to call QSE_APP_LOGX in a single call? better to store arguments to a buffer first and call QSE_APP_LOGX with it?
			switch (nargs)
			{
				case 0:
					QSE_APP_LOG2 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %hs\n"), h, cmd);
					break;

				case 1:
					QSE_APP_LOG3 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %hs %hs\n"), h, cmd, va_arg(xap, qse_mchar_t*));
					break;

				case 2:
				{
					qse_mchar_t* x[] = { va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*) };
					QSE_APP_LOG4 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %hs %hs %hs\n"), h, cmd, x[0], x[1]);
					break;
				}

				case 3:
				{
					qse_mchar_t* x[] = { va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*) };
					QSE_APP_LOG5 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %hs %hs %hs %hs\n"), h, cmd, x[0], x[1], x[2]);
					break;
				}

				case 4:
				{
					qse_mchar_t* x[] = { va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*) };
					QSE_APP_LOG6 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %hs %hs %hs %hs %hs\n"), h, cmd, x[0], x[1], x[2], x[3]);
					break;
				}

				case 5:
				{
					qse_mchar_t* x[] = { va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*) };
					QSE_APP_LOG7 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %hs %hs %hs %hs %hs %hs\n"), h, cmd, x[0], x[1], x[2], x[3], x[4]);
					break;
				}

				case 6:
				{
					qse_mchar_t* x[] = { va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*) };
					QSE_APP_LOG8 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %hs %hs %hs %hs %hs %hs %hs\n"), h, cmd, x[0], x[1], x[2], x[3], x[4], x[5]);
					break;
				}

				case 7:
				{
					qse_mchar_t* x[] = { va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*) };
					QSE_APP_LOG9 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %hs %hs %hs %hs %hs %hs %hs %hs\n"), h, cmd, x[0], x[1], x[2], x[3], x[4], x[5], x[6]);
					break;
				}

				default:
				{
					qse_mchar_t* x[] = { va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*), va_arg(xap, qse_mchar_t*) };
					QSE_APP_LOG9 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %hs %hs %hs %hs %hs %hs %hs %hs %hs ...\n"), h, cmd, x[0], x[1], x[2], x[3], x[4], x[5], x[6]);
					break;
				}
			}

			va_end (xap);
		}

		x = QSE::Sttp::sendCmdV(cmd, nargs, ap);
		va_end (ap);

		return x;
	}

	int sendCmd (const qse_wchar_t* cmd, qse_size_t nargs, ...)
	{
		int x;
		va_list ap;

		va_start (ap, nargs);

		if (QSE_APP_LOG_ENABLED(this->app, QSE_LOG_INFO))
		{
			va_list xap;
			int h = (int)this->sck->getHandle();

			va_copy (xap, ap);

			// ugly - any better way to call QSE_APP_LOGX in a single call? better to store arguments to a buffer first and call QSE_APP_LOGX with it?
			switch (nargs)
			{
				case 0:
					QSE_APP_LOG2 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %ls\n"), h, cmd);
					break;

				case 1:
					QSE_APP_LOG3 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %ls %ls\n"), h, cmd, va_arg(xap, qse_wchar_t*));
					break;

				case 2:
				{
					qse_wchar_t* x[] = { va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*) };
					QSE_APP_LOG4 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %ls %ls %ls\n"), h, cmd, x[0], x[1]);
					break;
				}

				case 3:
				{
					qse_wchar_t* x[] = { va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*) };
					QSE_APP_LOG5 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %ls %ls %ls %ls\n"), h, cmd, x[0], x[1], x[2]);
					break;
				}

				case 4:
				{
					qse_wchar_t* x[] = { va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*) };
					QSE_APP_LOG6 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %ls %ls %ls %ls %ls\n"), h, cmd, x[0], x[1], x[2], x[3]);
					break;
				}

				case 5:
				{
					qse_wchar_t* x[] = { va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*) };
					QSE_APP_LOG7 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %ls %ls %ls %ls %ls %ls\n"), h, cmd, x[0], x[1], x[2], x[3], x[4]);
					break;
				}

				case 6:
				{
					qse_wchar_t* x[] = { va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*) };
					QSE_APP_LOG8 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %ls %ls %ls %ls %ls %ls %ls\n"), h, cmd, x[0], x[1], x[2], x[3], x[4], x[5]);
					break;
				}

				case 7:
				{
					qse_wchar_t* x[] = { va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*) };
					QSE_APP_LOG9 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %ls %ls %ls %ls %ls %ls %ls %ls\n"), h, cmd, x[0], x[1], x[2], x[3], x[4], x[5], x[6]);
					break;
				}

				default:
				{
					qse_wchar_t* x[] = { va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*), va_arg(xap, qse_wchar_t*) };
					QSE_APP_LOG9 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,R] %ls %ls %ls %ls %ls %ls %ls %ls %ls ...\n"), h, cmd, x[0], x[1], x[2], x[3], x[4], x[5], x[6]);
					break;
				}
			}

			va_end (xap);
		}

		x = QSE::Sttp::sendCmdV(cmd, nargs, ap);
		va_end (ap);

		return x;
	}

	int sendErrCmd (ErrorNumber err, const qse_mchar_t* msg)
	{
		qse_mchar_t buf[32];
		qse_fmtintmaxtombs(buf, QSE_COUNTOF(buf), err, 10, -1, '\0', QSE_NULL);
		QSE::TypesErrorNumberToMbstr x;
		return this->sendCmd(CMD_ERR, 2, buf, (msg? msg: x(err)));
	}

	int sendErrCmd (ErrorNumber err, const qse_wchar_t* msg)
	{
		qse_wchar_t buf[32];
		qse_fmtintmaxtowcs(buf, QSE_COUNTOF(buf), err, 10, -1, '\0', QSE_NULL);
		QSE::TypesErrorNumberToWcstr x;
		return this->sendCmd(QSE_WT(CMD_ERR), 2, buf, (msg? msg: x(err)));
	}

	int sendWarnCmd (ErrorNumber err, const qse_mchar_t* msg)
	{
		qse_mchar_t buf[32];
		qse_fmtintmaxtombs(buf, QSE_COUNTOF(buf), err, 10, -1, '\0', QSE_NULL);
		QSE::TypesErrorNumberToMbstr x;
		return this->sendCmd(CMD_WARN, 2, buf, (msg? msg: x(err)));
	}

	int sendWarnCmd (ErrorNumber err, const qse_wchar_t* msg)
	{
		qse_wchar_t buf[32];
		qse_fmtintmaxtowcs(buf, QSE_COUNTOF(buf), err, 10, -1, '\0', QSE_NULL);
		QSE::TypesErrorNumberToWcstr x;
		return this->sendCmd(QSE_WT(CMD_WARN), 2, buf, (msg? msg: x(err)));
	}

	int sendOkCmd ()
	{
		return this->sendCmd(CMD_OK, 0);
	}
	int sendOkCmd (const qse_mchar_t* msg1)
	{
		return this->sendCmd(CMD_OK, 1, msg1);
	}
	int sendOkCmd (const qse_mchar_t* msg1, const qse_mchar_t* msg2)
	{
		return this->sendCmd(CMD_OK, 2, msg1, msg2);
	}
	int sendOkCmd (const qse_wchar_t* msg1)
	{
		return this->sendCmd(QSE_WT(CMD_OK), 1, msg1);
	}
	int sendOkCmd (const qse_wchar_t* msg1, const qse_wchar_t* msg2)
	{
		return this->sendCmd(QSE_WT(CMD_OK), 2, msg1, msg2);
	}

protected:
	MainApp* app;
	QSE::Socket* sck;

	typedef QSE::HashTable<QSE::String, CmdProc> CmdDict;
	CmdDict cmd_dict;

	int handle_command (const QSE::SttpCmd& cmd)
	{
		if (QSE_APP_LOG_ENABLED(this->app, QSE_LOG_INFO))
		{
			int h = this->sck->getHandle();
			const qse_char_t* name = cmd.name.getData();

			// ugly - any better way to call QSE_APP_LOGX in a single call? better to store arguments to a buffer first and call QSE_APP_LOGX with it?
			switch (cmd.getArgCount())
			{
				case 0:
					QSE_APP_LOG2 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,Q] %js\n"), h, name);
					break;

				case 1:
					QSE_APP_LOG3 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,Q] %js %js\n"), h, name, cmd.getArgAt(0));
					break;

				case 2:
					QSE_APP_LOG4 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,Q] %js %js %js\n"), h, name, cmd.getArgAt(0), cmd.getArgAt(1));
					break;

				case 3:
					QSE_APP_LOG5 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,Q] %js %js %js %js\n"), h, name, cmd.getArgAt(0), cmd.getArgAt(1), cmd.getArgAt(2));
					break;

				case 4:
					QSE_APP_LOG6 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,Q] %js %js %js %js %js\n"), h, name, cmd.getArgAt(0), cmd.getArgAt(1), cmd.getArgAt(2), cmd.getArgAt(3));
					break;

				case 5:
					QSE_APP_LOG7 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,Q] %js %js %js %js %js %js\n"), h, name,  cmd.getArgAt(0), cmd.getArgAt(1), cmd.getArgAt(2), cmd.getArgAt(3), cmd.getArgAt(4));
					break;

				case 6:
					QSE_APP_LOG8 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,Q] %js %js %js %js %js %js %js\n"), h, name,  cmd.getArgAt(0), cmd.getArgAt(1), cmd.getArgAt(2), cmd.getArgAt(3), cmd.getArgAt(4), cmd.getArgAt(5));
					break;

				case 7:
					QSE_APP_LOG9 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,Q] %js %js %js %js %js %js %js %js\n"), h, name, cmd.getArgAt(0), cmd.getArgAt(1), cmd.getArgAt(2), cmd.getArgAt(3), cmd.getArgAt(4), cmd.getArgAt(5), cmd.getArgAt(6));
					break;

				default:
					QSE_APP_LOG9 (this->app, QSE_LOG_INFO, QSE_T("[h=%d,Q] %js %js %js %js %js %js %js %js ...\n"), h, name, cmd.getArgAt(0), cmd.getArgAt(1), cmd.getArgAt(2), cmd.getArgAt(3), cmd.getArgAt(4), cmd.getArgAt(5), cmd.getArgAt(6));
					break;
			}
		}

		CmdDict::Pair* pair = this->cmd_dict.search(cmd.name);
		if (!pair) return this->sendErrCmd(E_ENOENT, "unknown command");

		return (this->*(pair->value))(cmd);
	}

	int write_bytes (const qse_uint8_t* data, qse_size_t len)
	{
		int n = this->sck->sendx(data, len, QSE_NULL);
		if (QSE_UNLIKELY(n <= -1)) this->setErrorFmt(E_ESYSERR, QSE_T("%js"), sck->getErrorMsg());
		return n;
	}

	int proc_discon (const QSE::SttpCmd& cmd)
	{
		PROC_CHECK_ARG_COUNT (cmd, 0);

		this->sendOkCmd (); // success and failure doesn't matter
		sck->shutdown ();
		return 0;
	}

	int proc_shtdwn (const QSE::SttpCmd& cmd)
	{
		PROC_CHECK_ARG_COUNT_RANGE(cmd, 0, 1);
		int ret = 0;

		if (cmd.getArgCount() >= 1) ret = qse_strtoi(cmd.getArgAt(0), 0, QSE_NULL);

		this->sendOkCmd (); // success and failure doesn't matter
		this->app->stop (ret);
		return 0;
	}

	int proc_envlst (const QSE::SttpCmd& cmd)
	{
		PROC_CHECK_ARG_COUNT (cmd, 0);

		{
			QSE::ScopedMutexLocker sml (this->app->getEnvMutex());
			const MainApp::Env::ItemList& item_list = this->app->getEnv().getItemList();
			for (const MainApp::Env::ItemList::Node* np = item_list.getHeadNode(); np; np = np->getNextNode())
			{
				const MainApp::Env::Item& item = np->value;
				const qse_char_t* val = this->app->getEnv().getValue(item.name);
				QSE_ASSERT (val != QSE_NULL);
				if (this->sendCmd(QSE_T(CMD_ENVITM), 2, item.name, val) <= -1) return -1;
			 }
		}

		return this->sendOkCmd();
	}

	int proc_envget (const QSE::SttpCmd& cmd)
	{
		PROC_CHECK_ARG_COUNT (cmd, 1);

		{
			QSE::ScopedMutexLocker sml (this->app->getEnvMutex());
			const qse_char_t* val = this->app->getEnv().getValue(cmd.getArgAt(0));
			if (!val) goto noent;
			return this->sendOkCmd(val);
		}

	noent:
		return this->sendErrCmd(E_ENOENT, (const qse_mchar_t*)QSE_NULL);
	}

	int proc_envset (const QSE::SttpCmd& cmd)
	{
		PROC_CHECK_ARG_COUNT (cmd, 2);

		const qse_char_t* name = cmd.getArgAt(0);
		const qse_char_t* val  = cmd.getArgAt(1);

		// TODO: session acl?
		//if (!this->_login.isRootUser()) return this->sendErrCmd (ERR_EPERM);

		MainApp::Env& env = this->app->getEnv();

		QSE::ScopedMutexLocker sml (this->app->getEnvMutex());
		if (this->app->getEnv().setValue(name, val) <= -1)
		{
			return this->sendErrCmd(E_EOTHER, "unable to set value");
		}

		const qse_char_t* v = env.getValue(name);
		QSE_ASSERT (v != QSE_NULL);

		if (env.store(this->app->getConfFile().getData()) <= -1) // TODO: full path
		{
			if (this->sendWarnCmd(E_EOTHER, "unable to store") <= -1) return -1;
		}

		return this->sendOkCmd(v);
	}
};


// --------------------------------------------------------------------------


int TcpGate::main () 
{
	this->setStackSize (1024u * 1024u * 2); // 2MB
	return this->execute(this->bind_address);
}

int TcpGate::stop () QSE_CPP_NOEXCEPT
{
	this->halt ();
	return 0;
}

int TcpGate::handle_connection (Connection* connection)
{
	qse_char_t addrbuf[128];
	qse_uint8_t bb[256];
	qse_ssize_t n;

	connection->address.toStrBuf(addrbuf, QSE_COUNTOF(addrbuf));

	Proto proto (this->app, &connection->socket);

	while (!this->isHaltRequested())
	{
		if ((n = connection->socket.receive(bb, QSE_COUNTOF(bb))) <= 0)
		{
			if (n <= -1)
				QSE_APP_LOG2 (this->app, QSE_LOG_INFO, QSE_T("receive failure from %js - %js\n"), addrbuf, connection->socket.getErrorMsg());
			break;
		}

#if 0
		for (int i = 0; i < n; i++)
		{
			if (proto.feed(&bb[i], 1, QSE_NULL) <= -1)
			{
				QSE_APP_LOG1 (this->app, QSE_LOG_ERROR, QSE_T("protocol error - %js\n"), proto.getErrorMsg());
				goto done;
			}
		}
#else
		if (proto.feed(bb, n, QSE_NULL) <= -1)
		{
			QSE_APP_LOG1 (this->app, QSE_LOG_ERROR, QSE_T("protocol error - %js\n"), proto.getErrorMsg());
			break;
		}
#endif
	}

done:
	//QSE_APP_LOG2 (this->app, QSE_LOG_INFO, QSE_T("byte to %s -> wid %zu\n"), addrbuf, connection->getWid());
	return 0;
}

void TcpGate::logfmt (qse_log_priority_flag_t pri, const qse_char_t* fmt, ...)
{
	if (QSE_APP_LOG_ENABLED(this->app, pri))
	{
		va_list ap;
		va_start (ap, fmt);
		this->app->logfmtv (pri, fmt, ap);
		va_end (ap);
	}
}
