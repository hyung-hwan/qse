#include "MainApp.hpp"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <qse/si/sio.h>

// ---------------------------------------------------------------------------

MainApp::Env::Env (MainApp* app, QSE::Mmgr* mmgr): SkvEnv(mmgr), app(app), log_type_preset(false), log_level_preset(false)
{
	this->addItem (APP_ENV_LOG_TYPE,
		APP_LOG_TYPE_FILE,
		(ProbeProc)&MainApp::Env::probe_log_type);

	this->addItem (
		APP_ENV_LOG_LEVEL,
		QSE_T("info+"),
		(ProbeProc)&MainApp::Env::probe_log_level);

	this->addItem (
		APP_ENV_CHROOT,
		QSE_T(""),
		(ProbeProc)&MainApp::Env::probe_chroot);

	this->addItem (
		APP_ENV_GATE_ADDRESSES,
		APP_GATE_ADDRESSES,
		(ProbeProc)&MainApp::Env::probe_gate_addresses);

	this->addItem (
		APP_ENV_GATE_MAX_CONNECTIONS,
		QSE_Q(APP_GATE_MAX_CONNECTIONS),
		(ProbeProc)&MainApp::Env::probe_gate_max_connections);

	this->addItem (
		APP_ENV_GATE_TIMEOUT,
		APP_GATE_TIMEOUT,
		(ProbeProc)&MainApp::Env::probe_gate_timeout);
}

int MainApp::Env::probe_log_type (const qse_char_t* v)
{
	int tmask = 0;
	const qse_char_t* p = v;
	qse_cstr_t tok; 

	while (p) 
	{
		p = qse_strtok(p, QSE_T(","), &tok);
		if (qse_strxcmp(tok.ptr, tok.len, APP_LOG_TYPE_FILE) == 0)
			tmask |= QSE_LOG_FILE;
		else if (qse_strxcmp(tok.ptr, tok.len, APP_LOG_TYPE_SYSLOG) == 0)
			tmask |= QSE_LOG_SYSLOG;
		else if (qse_strxcmp(tok.ptr, tok.len, APP_LOG_TYPE_CONSOLE) == 0)
			tmask |= QSE_LOG_CONSOLE;
		else return -1;
	};

	if (!this->log_type_preset)
	{
		qse_log_target_data_t logtype;
		this->app->getLogTarget (logtype);
		this->app->setLogTarget (tmask, logtype);
	}
	return 0;
}

int MainApp::Env::probe_log_level (const qse_char_t* v)
{
	if (v[0] == '\0' || qse_stristype(v, QSE_CTYPE_SPACE)) return -1;
	int prio = qse_get_log_priority_by_name(v, QSE_T(","));
	if (prio == 0) return -1; // unknown name inside

	if (!this->log_level_preset) this->app->setLogPriorityMask(prio);
	return 0;
}

int MainApp::Env::probe_chroot (const qse_char_t* v)
{
	// nothing to inspect
	if (v[0] != '\0' && qse_stristype(v, QSE_CTYPE_SPACE)) return -1;
	this->chroot.update (v);
	return 0;
}

int MainApp::Env::probe_gate_addresses (const qse_char_t* v)
{
	if (v[0] == '\0' || qse_stristype(v, QSE_CTYPE_SPACE)) return -1;
	this->gate_addresses.update (v);
	return 0;
}

int MainApp::Env::probe_gate_max_connections (const qse_char_t* v)
{
	if (v[0] == QSE_T('\0')) return -1;
	if (!qse_stristype(v, QSE_CTYPE_DIGIT)) return -1;

	/* don't care about the overflow */

	unsigned int num = qse_strtoui(v, 10, QSE_NULL);
	if (num < APP_GATE_MAX_CONNECTIONS_MIN || num > APP_GATE_MAX_CONNECTIONS_MAX) return -1;

	this->gate_max_connections = num;
// TODO:
//	this->app->setMaxTcpGateConnections (this->gate_max_connections);
	return 0;
}

int MainApp::Env::probe_gate_timeout (const qse_char_t* v)
{
	if (v[0] == QSE_T('\0')) return -1;

	/* don't care about the overflow */
	qse_ntime_t tmout;
	if (qse_str_to_ntime(v, &tmout) <= -1) return -1;

	this->gate_timeout = tmout;
	return 0;
}

// ---------------------------------------------------------------------------

// TODO: get application name... use it for log files and for other purposes...
MainApp::MainApp (QSE::Mmgr* mmgr): QSE::App(mmgr), exit_code(0), guardian(true), conffile(APP_INI_FILE), env(this), tcp_gate(this)
{
}

MainApp::~MainApp () 
{
}

int MainApp::run (bool foreground)
{
	QSE::App::SignalSet sigs;

	sigs.set (SIGINT);
	sigs.set (SIGHUP);
	sigs.set (SIGTERM);
	sigs.set (SIGUSR1);
	sigs.set (SIGUSR2);

	if (this->guardProcess(sigs, this->guardian) > 0)
	{
		this->acceptSignals (sigs);
		this->setName (APP_NAME);
		this->init_logger (foreground);

		// assuming the above statements outside 'try..catch' throw no exceptions,
		// no log message should be produced before configuration is loaded fully.
		// any exceptions thrown before entering 'run()' is handled in Main.cc.
		// for instance, setSysconfDir() which is supposed to be called in Main.cc
		// may throw an exception. but it's treated as fatal error that prevents
		// the program start-up in Main.cc.

		try
		{
			int x = this->load_config();

			QSE_APP_LOG1 (this, QSE_LOG_INFO, QSE_T("starting application %d\n"), (int)getpid());

			if (x <= -1) QSE_APP_LOG1 (this, QSE_LOG_WARNING, QSE_T("unable to load configuration from %js\n"), this->conffile.getData());
			else QSE_APP_LOG1 (this, QSE_LOG_INFO, QSE_T("loaded configuration from %js\n"), this->conffile.getData());

			this->chroot_if_needed ();

			this->tcp_gate.setBindAddress (this->gate_addresses.isEmpty()? this->env.getGateAddresses(): this->gate_addresses.getData());
			if (this->tcp_gate.start() <= -1)
			{
				QSE_APP_LOG1 (this, QSE_LOG_ERROR, QSE_T("starting %d\n"), (int)getpid());
				this->exit_code = 88;
				goto done;
			}

			// TODO: start other worker threads and joins on them

			this->tcp_gate.join ();

		done:
			QSE_APP_LOG2 (this, QSE_LOG_INFO, QSE_T("exiting application %d with code %d\n"), (int)getpid(),  this->exit_code);
		}
		catch (QSE::Exception& e)
		{
			QSE_APP_LOG2 (this, QSE_LOG_INFO, QSE_T("terminating application for exception - %js - %js\n"), QSE_EXCEPTION_NAME(e), QSE_EXCEPTION_MSG(e));
		}
		catch (...)
		{
			QSE_APP_LOG0 (this, QSE_LOG_INFO, QSE_T("terminating application for unknown exception\n"));
		}

		this->discardSignals (sigs);
		return this->exit_code;
	}

	this->neglectSignals (sigs, true);
	return 0;
}

void MainApp::on_signal (int sig)
{
	if (!this->isGuardian())
	{
		QSE_APP_LOG2 (this, QSE_LOG_INFO, QSE_T("terminating application %d on signal %d\n"), (int)getpid(), sig);
		this->stop ((sig == SIGSEGV)? 99: 0);
	}
}

void MainApp::stop (int code)
{
	this->exit_code = code;
	this->tcp_gate.stop ();
	// TODO: stop all other worker threads... 
}

void MainApp::setSysconfDir (const qse_char_t* v)
{
	this->sysconfdir.update (v);

	if (!this->sysconfdir.isEmpty())
	{
		this->conffile = this->sysconfdir;
		if (this->conffile.getLastChar() != '/') this->conffile.append (QSE_T("/"));
		this->conffile.append (QSE_T(APP_INI_FILE_));
	}
}

void MainApp::init_logger (bool foreground)
{
	qse_log_target_data_t logtgt;

	memset (&logtgt, 0, QSE_SIZEOF(logtgt));

	this->setLogOption (QSE_LOG_INCLUDE_PID | QSE_LOG_KEEP_FILE_OPEN);

	this->setLogPriorityMask (QSE_LOG_ALL_PRIORITIES);
	if (!this->loglevel.isEmpty() && this->env.probe_log_level(this->loglevel.getData()) >= 0) this->env.log_level_preset = true;

	logtgt.file = this->logfile.isEmpty()? APP_LOG_FILE: this->logfile.getData();
	this->setLogTarget (QSE_LOG_FILE, logtgt);

	if (!this->logtype.isEmpty() && this->env.probe_log_type(this->logtype.getData()) >= 0) this->env.log_type_preset = true;
}

int MainApp::load_config ()
{
	int n = this->env.load(this->conffile.getData());

	// reset these to false so that ENVSET over the control channel 
	// take effect. these might be set to true in init_logger()
	// and load_config() doesn't apply the relevant items when they are true
	this->env.log_type_preset = false;
	this->env.log_level_preset = false;

	return n;
}

void MainApp::chroot_if_needed ()
{
	const qse_char_t* root;

	root = this->chroot_path.getData();
	if (root[0] == '\0') root = this->env.getChroot();

	if (root[0] != '\0')
	{
		if (this->chroot(root) <= -1)
		{
			QSE_APP_LOG1 (this, QSE_LOG_WARNING, QSE_T("unable to chroot to %js\n"), root);
		}
		else
		{
			QSE_APP_LOG1 (this, QSE_LOG_INFO, QSE_T("chrooted to %js\n"), root);
		}
	}
}
