#ifndef _MAINAPP_CLASS_
#define _MAINAPP_CLASS_

#include <qse/si/App.hpp>
#include <qse/si/Mutex.hpp>
#include <qse/xli/SkvEnv.hpp>
#include "TcpGate.hpp"

#define APP_NAME                      QSE_T("app01")
#define APP_VERSION                   QSE_T("1.0.0")
#define APP_INI_FILE_                 "app01.ini"
#define APP_LOG_FILE_                 "app01.log"
#define APP_FILE_PERM                 0600

#if defined(APP_CFG_DIR_)
#	define APP_INI_FILE QSE_T(APP_CFG_DIR_ "/" APP_INI_FILE_)
#	define APP_CFG_DIR QSE_T(APP_CFG_DIR_)
#else
#	define APP_INI_FILE QSE_T("/etc/" APP_INI_FILE_)
#	define APP_CFG_DIR QSE_T("/etc")
#endif

#if defined(APP_LOG_DIR_)
#	define APP_LOG_FILE QSE_T(APP_LOG_DIR_ "/" APP_LOG_FILE_)
#else
#	define APP_LOG_FILE QSE_T("/var/log/" APP_LOG_FILE_)
#endif

#define APP_YES                       QSE_T("yes")
#define APP_NO                        QSE_T("no")
#define APP_GATE_ADDRESSES            QSE_T("[::]:9998,0.0.0.0:9998")
#define APP_GATE_MAX_CONNECTIONS_MIN  0  // 0 means unlimited, however
#define APP_GATE_MAX_CONNECTIONS_MAX  100
#define APP_GATE_MAX_CONNECTIONS      30
#define APP_GATE_TIMEOUT              QSE_T("-1.000000000")

#define APP_LOG_TYPE_FILE         QSE_T("file")
#define APP_LOG_TYPE_SYSLOG       QSE_T("syslog")
#define APP_LOG_TYPE_CONSOLE      QSE_T("console")

#define APP_ENV_LOG_TYPE          QSE_T("MAIN*LOG_TYPE")
#define APP_ENV_LOG_LEVEL         QSE_T("MAIN*LOG_LEVEL")

#define APP_ENV_GATE_ADDRESSES        QSE_T("GATE*ADDRESSES")
#define APP_ENV_GATE_MAX_CONNECTIONS  QSE_T("GATE*MAX_CONNECTIONS")
#define APP_ENV_GATE_TIMEOUT          QSE_T("GATE*TIMEOUT")


class MainApp: public QSE::App
{
public:
	class Env: public QSE::SkvEnv 
	{
	public:
		friend class MainApp;

		Env (MainApp* app, QSE::Mmgr* mmgr = QSE_NULL);

		const qse_char_t* getChroot () const { return this->chroot.getData(); }
		const qse_char_t* getGateAddresses () const { return this->gate_addresses.getData(); }
		unsigned int getGateMaxConnections () const { return this->gate_max_connections; }
		const qse_ntime_t* getGateTimeout () const { return &this->gate_timeout; }

	protected:
		MainApp* app;

		QSE::String chroot;
		QSE::String gate_addresses;
		unsigned int gate_max_connections;
		qse_ntime_t gate_timeout;

	private:
		bool log_type_preset;
		bool log_level_preset;

		int probe_log_type (const qse_char_t* v);
		int probe_log_level (const qse_char_t* v);

		int probe_gate_addresses       (const qse_char_t* v);
		int probe_gate_max_connections (const qse_char_t* v);
		int probe_gate_timeout         (const qse_char_t* v);
	};

	MainApp (QSE::Mmgr* mmgr = QSE_NULL);
	~MainApp ();

	int run (bool foreground);
	void stop (int code);

	// assignment of bool to int, -1 means it's not set.
	void setGuardian (bool guardian) { this->guardian = guardian; }; 

	void setChroot (const qse_char_t* v) { this->chroot_path.update (v); }
	void setSysconfDir (const qse_char_t* v);
	void setLogFile (const qse_char_t* v) { this->logfile.update (v); }
	void setLogType (const qse_char_t* v) { this->logtype.update (v); }
	void setLogLevel (const qse_char_t* v) { this->loglevel.update (v); }
	void setGateAddresses (const qse_char_t* v) { this->gate_addresses.update (v); }

	QSE::Mutex& getEnvMutex() { return this->env_mutex; }
	Env& getEnv() { return this->env; }
	const Env& getEnv() const { return this->env; }

	const QSE::String& getConfFile() const { return this->conffile; }

protected:
	int exit_code;
	bool guardian;
	QSE::String chroot_path;
	QSE::String sysconfdir;
	QSE::String logfile;
	QSE::String logtype;
	QSE::String loglevel;
	QSE::String gate_addresses;
	QSE::String conffile;
	QSE::Mutex env_mutex;

	Env env;
	TcpGate tcp_gate;

	void on_signal (int sig);

	void init_logger (bool foreground);
	int load_config ();
	void chroot_if_needed ();
};

#endif
