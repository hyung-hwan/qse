#include "MainApp.hpp"

#include <qse/cmn/main.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/time.h>
#include <qse/si/sio.h>
#include <locale.h>

static void print_usage (const qse_cli_t* cli)
{
	qse_fprintf (QSE_STDERR, QSE_T("[USAGE] %s [options]\n"), cli->verb);
	qse_fprintf (QSE_STDERR, QSE_T("Options as follows:\n"));
	qse_fprintf (QSE_STDERR, QSE_T(" --foreground      run the program in the foreground mode\n"));
	qse_fprintf (QSE_STDERR, QSE_T(" --guardian=yes|no run the program under the guardian process\n"));
	qse_fprintf (QSE_STDERR, QSE_T(" --chroot=path     specify the directory to chroot to\n"));
	qse_fprintf (QSE_STDERR, QSE_T(" --sysconfdir=path specify the directory to store system configuration\n"));
	qse_fprintf (QSE_STDERR, QSE_T(" --logfile=path    specify the log file\n"));
	qse_fprintf (QSE_STDERR, QSE_T(" --logtype=string  specify logging types\n"));
	qse_fprintf (QSE_STDERR, QSE_T(" --loglevel=string specify logging levels\n"));
	qse_fprintf (QSE_STDERR, QSE_T(" --gate-addresses=string\n"));
	qse_fprintf (QSE_STDERR, QSE_T("                   specify the control channel address\n"));
	qse_fprintf (QSE_STDERR, QSE_T("\n"));
	qse_fprintf (QSE_STDERR, QSE_T("logging type string: one or more of the followings delimited by a comma:\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  console,  file, syslog\n"));
	qse_fprintf (QSE_STDERR, QSE_T("logging level string: one or more of the followings delimited by a comma:\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  panic, alert, critical, error, warning, notice, info, debug\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  each item may get suffixed with +, -, ! to mean\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  'higher', 'lower', 'not' respectively (e.g. info+,notice!,alert!,debug)\n"));
}


static int handle_cli_error (qse_cli_t* cli, qse_cli_error_code_t code, const qse_char_t* name, const qse_char_t* value)
{
	switch (code)
	{
		case QSE_CLI_ERROR_INVALID_OPTNAME:
			qse_fprintf (QSE_STDERR, QSE_T("[ERROR] unknown option - %s\n"), name);
			break;

		case QSE_CLI_ERROR_MISSING_OPTNAME:
			qse_fprintf (QSE_STDERR, QSE_T("[ERROR] missing option - %s\n"), name);
			break;

		case QSE_CLI_ERROR_REDUNDANT_OPTVAL:
			qse_fprintf (QSE_STDERR, QSE_T("[ERROR] redundant value %s for %s\n"), value, name);
			break;

		case QSE_CLI_ERROR_MISSING_OPTVAL:
			qse_fprintf (QSE_STDERR, QSE_T("[ERROR] missing value for %s\n"), name);
			break;

		case QSE_CLI_ERROR_MEMORY:
			qse_fprintf (QSE_STDERR, QSE_T("[ERROR] memory error in processing %s\n"), name);
			break;

		default:
			qse_fprintf (QSE_STDERR, QSE_T("[ERROR] unknown cli error - %d\n"), code);
			break;
	}

	print_usage (cli);
	return -1;
}

static int parse_cli (int argc, qse_char_t* argv[], qse_cli_t* cli)
{
	static const qse_char_t* optsta[] = 
	{
		QSE_T("--"), QSE_NULL
	};

	static qse_cli_opt_t opts[] = 
	{
		{ QSE_T("foreground"),     QSE_CLI_DISCRETIONARY_OPTVAL },
		{ QSE_T("guardian"),       QSE_CLI_REQUIRE_OPTVAL },
		{ QSE_T("chroot"),         QSE_CLI_REQUIRE_OPTVAL },
		{ QSE_T("gate-addresses"), QSE_CLI_REQUIRE_OPTVAL },
		{ QSE_T("sysconfdir"),     QSE_CLI_REQUIRE_OPTVAL },
		{ QSE_T("logfile"),        QSE_CLI_REQUIRE_OPTVAL },
		{ QSE_T("logtype"),        QSE_CLI_REQUIRE_OPTVAL },
		{ QSE_T("loglevel"),       QSE_CLI_REQUIRE_OPTVAL },
		{ QSE_NULL,                0 }
	};

	static qse_cli_data_t cli_data = 
	{
		handle_cli_error,
		optsta,
		QSE_T("="),
		opts,
		QSE_NULL
	};

	return qse_parsecli(cli, QSE_NULL, argc, argv, &cli_data);
}


static int app_main (int argc, qse_char_t* argv[])
{
	qse_cli_t cli;
	int rc = -1;

	/* parse command line */
	if (parse_cli(argc, argv, &cli) <= -1) return -1;
	if (qse_getncliparams(&cli) > 0) 
	{
		print_usage (&cli);
		qse_fprintf (QSE_STDERR, QSE_T("<error> redundant cli parameters"));
		qse_clearcli (&cli);
		return -1;
	}

	try
	{
		MainApp main_app;
		const qse_char_t* tmp;
		bool foreground = false;

		tmp = qse_getclioptval(&cli, QSE_T("foreground"));
		if (tmp) foreground = (tmp[0] == '\0' || qse_strcasecmp(tmp, APP_YES) == 0);

		tmp = qse_getclioptval(&cli, QSE_T("guardian"));
		if (tmp) main_app.setGuardian(qse_strcasecmp(tmp, APP_YES) == 0);

		tmp = qse_getclioptval(&cli, QSE_T("chroot"));
		if (tmp) main_app.setChroot (tmp);

		tmp = qse_getclioptval(&cli, QSE_T("sysconfdir"));
		if (tmp) main_app.setSysconfDir (tmp);

		tmp = qse_getclioptval(&cli, QSE_T("logfile"));
		if (tmp) main_app.setLogFile (tmp);

		tmp = qse_getclioptval(&cli, QSE_T("logtype"));
		if (tmp) main_app.setLogType (tmp);

		tmp = qse_getclioptval(&cli, QSE_T("loglevel"));
		if (tmp) main_app.setLogLevel (tmp);

		tmp = qse_getclioptval(&cli, QSE_T("gate-addresses"));
		if (tmp) main_app.setGateAddresses (tmp);

		if (!foreground && main_app.daemonize(true) <= -1)
		{
			qse_printf (QSE_T("Error: unable to daemonize the process\n"));
			rc = -1;
		}
		else
		{
			rc = main_app.run(foreground);
		}
	}
	catch (QSE::Exception& e)
	{
		qse_printf (QSE_T("Exception: %js\n"), QSE_EXCEPTION_NAME(e), QSE_EXCEPTION_MSG(e));
	}
	catch (...)
	{
		qse_printf (QSE_T("Exception: unknown exception\n"));
	}

	qse_clearcli (&cli);
	return rc;
}

int main (int argc, char* argv[])
{
	int rc;

	setlocale (LC_ALL, "");
	qse_open_stdsios ();

	rc = qse_run_main(argc, argv, app_main);

	qse_close_stdsios ();
	return rc;
}
