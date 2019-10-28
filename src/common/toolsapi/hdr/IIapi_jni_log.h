/*
** Copyright 2008-2011 Actian Corporation. All rights reserved
**
** Description:
**	Logging API for the IIapi JNI wrapper layer.
**
** History:
**    March 2008 (whiro01)
**	Initial prototype version.
**    07-Dec-2009 (whiro01)
**	Cleaned up; added header comments.
**    06-Jun-2010 (whiro01)
**	Added much more functionality and use non-Ingres
**	functions.
**    23-Jun-2010 (whiro01)
**	Added the EntryExitLogger class.
**    07-Jul-2010 (whiro01)
**	Make this work under OSX.
**    27-Jul-2010 (whiro01)
**	Open/close log file on each output to get output
**	in case of a crash.
**    09-Dec-2010 (whiro01)
**	Put mutex protection around logFile access in the
**	(common) case that two JNI threads are each using
**	the same logger object.
**    18-Mar-2011 (whiro01)
**	Add methods to check a logger's log level.
**    04-Jun-2011 (whiro01)
**	Added defines for Linux.
**    22-Sep-2011 (whiro01)
**	Changed WINDOWS to NT_GENERIC for integration into
**	Ingres codeline; added OSX in addition to DARWIN.
**    28-Sep-2011 (whiro01)
**	Added missing #include of <string.h> for "strlen" et. al.
*/
#ifndef IIAPI_JNI_LOG_H
#define IIAPI_JNI_LOG_H

#if defined(NT_GENERIC) || defined(WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <time.h>
#elif defined(DARWIN) || defined(OSX)
  #include <mach/mach_time.h>
  #include <time.h>
#elif defined(LINUX)
  #include <time.h>
#else
  #error Must implement high-res timer on this platform!
#endif
#include <string>
#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "IIapi_jni_util.h"

typedef enum
{
	LOG_INVALID	= -99,
	LOG_OFF		= -1,
	/* Special level just for timing of code                            */
	/* If the level is < 0, logging is off, if > 0 then check the level */
	/* If the level == 0 we're doing timing                             */
	LOG_TIMER	= 0,
	/* Logging levels as defined by the BSD syslog protocol (RFC3164)   */
	LOG_EMERGENCY	= 1,	/* Emergency: system is unusable            */
	LOG_ALERT	= 2,	/* Alert: action must be taken immediately  */
	LOG_CRITICAL	= 3,	/* Critical: critical conditions            */
	LOG_ERROR	= 4,	/* Error: error conditions                  */
	LOG_WARNING	= 5,	/* Warning: warning conditions              */
	LOG_NOTICE	= 6,	/* Notice: normal but significant condition */
	LOG_INFO	= 7,	/* Informational: informational messages    */
	LOG_DEBUG	= 8,	/* Debug: debug-level messages              */
	LOG_TRACE	= 9	/* Trace: almost everything that happens    */
} LogLevel;


class Logger {
	private:
		std::string logFileName;
		std::string prefix;
		FILE *logFile;
		LogLevel level;
		Mutex sync;
		bool logToFile;
		void init(std::string file, LogLevel lvl);
		bool openLogFile();
		void closeLogFile();
	public:
		explicit Logger();
		explicit Logger(const char *file);
		explicit Logger(std::string file);
		explicit Logger(LogLevel lvl);
		~Logger();
		LogLevel setLevel(LogLevel newLvl);
		LogLevel setLevel(std::string level);
		LogLevel setLevel(const char *level);
		LogLevel convertLevel(const char *level);
		LogLevel getLevel() { return level; }
		bool isLevelEnabled(LogLevel lvl);
		void setPrefix(const char *pfx);
		void output(LogLevel lvl, const char *fmt, va_list args);
		void trace(const char *fmt, ...);
		void debug(const char *fmt, ...);
		void info(const char *fmt, ...);
		void notice(const char *fmt, ...);
		void warn(const char *fmt, ...);
		void error(const char *fmt, ...);
		void critical(const char *fmt, ...);
		void alert(const char *fmt, ...);
		void emergency(const char *fmt, ...);
		void timer(const char *fmt, ...);
};


class LogTimer {
	private:
		bool	_canCount;
		Logger *log;

#if defined(NT_GENERIC) || defined(WIN32)
		LARGE_INTEGER	_freq;
		LARGE_INTEGER	_start;
		LARGE_INTEGER	_stop;
		LARGE_INTEGER	_total;

		void do_replacements(LARGE_INTEGER& value, std::string& str, const char *templ, bool do_raw);
#elif defined(DARWIN) || defined(OSX)
		double	    _freq;
		uint64_t    _start;
		uint64_t    _stop;
		uint64_t    _total;

		void do_replacements(uint64_t value, std::string& str, const char *templ, bool do_raw);
#elif defined(LINUX)
		struct timespec	_freq;
		struct timespec _start;
		struct timespec _stop;
		struct timespec _total;

		void do_replacements(struct timespec value, std::string& str, const char *templ, bool do_raw);
#else
	#error	Implement high-resolution timer on this platform!
#endif

	public:
		LogTimer();
		~LogTimer();

		bool canCount() { return _canCount; }

		void Start(void);
		void Stop(void);

		void LogTiming(const char *szMessage);
};


class EntryExitLogger {
	private:
		std::string funcName;
		Logger *logger;
		void Log(const char *event);
	public:
		EntryExitLogger(Logger *logger, const char *function);
		EntryExitLogger(Logger *logger, std::string& function);
		~EntryExitLogger();
};

#ifdef __cplusplus
extern "C" {
#endif

typedef void *	TimerHandle;

TimerHandle InitTimer();
void TermTimer(TimerHandle);

void StartTimer(TimerHandle);
void StopTimer(TimerHandle);

void ReportTimer(TimerHandle, const char *szMessage);
void ReportSplit(TimerHandle, const char *szMessage);

#ifdef __cplusplus
}
#endif

#endif	/* IIAPI_JNI_LOG_H */
