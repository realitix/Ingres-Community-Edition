/*
** Copyright 2008-2011 Actian Corporation. All rights reserved
**
** Description:
**	Logging facility for the IIapi JNI wrapper
**	The first version is mostly to do timing.
**
** History:
**    March 2008 (whiro01)
**	First prototype version.
**    07-Dec-2009 (whiro01)
**	Added header comments.
**    06-Jun-2010 (whiro01)
**	Added much more functionality and use non-Ingres
**	functions.
**    16-Jun-2010 (whiro01)
**	Separate out Windows from non-Windows C library
**	calls.
**    23-Jun-2010 (whiro01)
**	Added the EntryExitLogger class.
**    28-Jun-2010 (whiro01)
**	Filled out the code to initialize from the environment;
**	added methods to set level from a string value.
**    28-Jun-2010 (whiro01)
**	Extra conditions on using the new "secure" Windows
**	CRT functions (VS2008 or higher).
**    07-Jul-2010 (whiro01)
**	Make this work under OSX.
**    27-Jul-2010 (whiro01)
**	Open/close log file on each output to get output
**	in case of a crash.
**    27-Jul-2010 (whiro01)
**	Change logging environment variable to be consistent
**	with Tools API variable.
**    09-Dec-2010 (whiro01)
**	Doh!  Have to mutex protect the file object against
**	concurrent access by multiple JNI threads.
**    10-Dec-2010 (whiro01)
**	One more fix: enter and leave the mutex for all cases
**	not just for file output (otherwise console output could
**	get garbled by multiple threads also); handle possible
**	exceptions from "new".
**    15-Dec-2010 (dansa01)
**	Removed unreferenced variable warnings (from catch clauses).
**    18-Mar-2011 (whiro01)
**	Fix Timer routines to deal gracefully with NULL timer;
**	don't init timer if II_JNI_LOGGING is not set in the environment;
**	add method to check logger's logging level.
**    04-Jun-2011 (whiro01)
**	Adding implementation of high-res timer on Linux.
**    22-Aug-2011 (whiro01)
**	Using NT_GENERIC instead of WINDOWS for the platform tests; add
**	OSX in addition to DARWIN.
**    26-Sep-2011 (whiro01)
**	Change the header file include paths for compilation in Ingres codelines.
**    29-Sep-2011 (whiro01)
**	Moved #include of time.h to IIapi_jni_log.h
*/
#include "toolsapi.h"
#include "IIapi_jni_log.h"

using namespace std;

extern const char *API;
static const LogLevel defaultLevel = LOG_NOTICE;
static const char *levelNames[] = {
	"TIMER",
	"EMERGENCY",
	"ALERT",
	"CRITICAL",
	"ERROR",
	"WARNING",
	"NOTICE",
	"INFO",
	"DEBUG",
	"TRACE"
};
#define NUM_LEVELS	(sizeof(levelNames) / sizeof(levelNames[0]))


void Logger::init(string file, LogLevel lvl)
{
	if (file.empty() || lvl == LOG_INVALID)
	{
		// Get environment setting to set file
#if defined(NT_GENERIC) && (_MSC_VER >= 1500)
		char *settings;
		size_t num;
		if (_dupenv_s(&settings, &num, "II_JNI_LOGGING") == 0 &&
		    settings != NULL && *settings != '\0')
#else
		const char *settings = getenv("II_JNI_LOGGING");
		if (settings != NULL && *settings != '\0')
#endif
		{
			// Format should be:
			// level,file name
			string setting = string(settings);
			size_t idx;
			if ((idx = setting.find(',')) != string::npos)
			{
				string level = setting.substr(0, idx);
				file = setting.substr(idx + 1);
				lvl = convertLevel(level.c_str());
			}
		}
		else
		{
			// Override even the specified level (usually LOG_TIMER)
			// if the user's desired level cannot be determined from
			// the environment -- we don't want timer messages coming
			// out in a production environment unless specifically
			// enabled.
			lvl = defaultLevel;
		}
#if defined(NT_GENERIC) && (_MSC_VER >= 1500)
		free(settings);
#endif
	}
	logFile = NULL;
	logFileName = file;
	logToFile = !logFileName.empty();
	level = lvl;
	prefix = string("");
}

void Logger::setPrefix(const char *pfx)
{
	if (pfx != NULL && *pfx != '\0')
		prefix = string(pfx);
	else
		prefix = string("");
}

bool Logger::openLogFile()
{
	sync.enter();
	if (logToFile)
	{
#if defined(NT_GENERIC) && (_MSC_VER >= 1500)
		fopen_s(&logFile, logFileName.c_str(), "a");
#else
		logFile = fopen(logFileName.c_str(), "a");
#endif
	}
	else
	{
		logFile = NULL;
	}
	return (logFile != NULL);
}

void Logger::closeLogFile()
{
	if (logFile != NULL)
	{
		fclose(logFile);
		logFile = NULL;
	}
	sync.leave();
}

Logger::Logger()
{
	init(string(""), LOG_INVALID);
}

Logger::Logger(const char *file)
{
	init(string(file), LOG_INVALID);
}

Logger::Logger(std::string file)
{
	init(file, LOG_INVALID);
}

Logger::Logger(LogLevel lvl)
{
	init(string(""), lvl);
}

Logger::~Logger()
{
	closeLogFile();
}

LogLevel Logger::setLevel(LogLevel newLvl)
{
	LogLevel prevLvl = this->level;
	this->level = newLvl;
	return prevLvl;
}

LogLevel Logger::setLevel(std::string level)
{
	return setLevel(level.c_str());
}

LogLevel Logger::setLevel(const char *level)
{
	LogLevel lvl = convertLevel(level);
	if (lvl != LOG_INVALID)
		return setLevel(lvl);
	return lvl;
}

LogLevel Logger::convertLevel(const char *level)
{
	// Try to match a number
	unsigned long val;
	char *ptr;
	val = strtoul(level, &ptr, 0);
	if (*ptr == '\0') {
		if (val >= LOG_OFF && val <= LOG_TRACE) {
			return (LogLevel)val;
		}
	}
	else {
		// Try to match a level name
		for (int i = 0; i < NUM_LEVELS; i++) {
#if defined(NT_GENERIC) || defined(WIN32)
			if (_stricmp(level, levelNames[i]) == 0) {
#else
			if (strcasecmp(level, levelNames[i]) == 0) {
#endif
				return (LogLevel)i;
			}
		}
#if defined(NT_GENERIC) || defined(WIN32)
		if (_stricmp(level, "OFF") == 0) {
#else
		if (strcasecmp(level, "OFF") == 0) {
#endif
			return LOG_OFF;
		}
	}
	return LOG_INVALID;
}

bool Logger::isLevelEnabled(LogLevel lvl)
{
	if (lvl == LOG_TIMER && level == LOG_TIMER)
		return true;
	else if (level >= lvl && level != LOG_TIMER)
		return true;
	return false;
}

void Logger::output(const LogLevel lvl, const char *fmt, va_list args)
{
	char timebuf[40];
	struct tm *now;
#if defined(NT_GENERIC) && (_MSC_VER >= 1500)
	__int64 ltime;
	struct tm nowgm;

	_time64( &ltime );
	// Obtain coordinated universal time:
	now = &nowgm;
	_gmtime64_s( now, &ltime );
#else
	time_t	ltime;

	time( &ltime );
	// Obtain coordinated universal time:
	now = gmtime( &ltime );
#endif

	strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", now);

	if (openLogFile())
	{
		fprintf(logFile, "%s %9s %s: ", timebuf, levelNames[lvl], prefix.c_str());
		vfprintf(logFile, fmt, args);
		fprintf(logFile, "\n");
	}
	else
	{
		if (lvl > LOG_WARNING)
		{
			fprintf(stderr, "%s %9s %s: ", timebuf, levelNames[lvl], prefix.c_str());
			vfprintf(stderr, fmt, args);
			fprintf(stderr, "\n");
		}
		else
		{
			printf("%s %9s %s: ", timebuf, levelNames[lvl], prefix.c_str());
			vprintf(fmt, args);
			printf("\n");
		}
	}
	closeLogFile();
}

void Logger::trace(const char *fmt, ...)
{
	if (level >= LOG_TRACE && level != LOG_TIMER) {
		va_list args;
		va_start(args, fmt);
		output(LOG_TRACE, fmt, args);
		va_end(args);
	}
}

void Logger::debug(const char *fmt, ...)
{
	if (level >= LOG_DEBUG && level != LOG_TIMER) {
		va_list args;
		va_start(args, fmt);
		output(LOG_DEBUG, fmt, args);
		va_end(args);
	}
}

void Logger::info(const char *fmt, ...)
{
	if (level >= LOG_INFO && level != LOG_TIMER) {
		va_list args;
		va_start(args, fmt);
		output(LOG_INFO, fmt, args);
		va_end(args);
	}
}

void Logger::notice(const char *fmt, ...)
{
	if (level >= LOG_NOTICE && level != LOG_TIMER) {
		va_list args;
		va_start(args, fmt);
		output(LOG_NOTICE, fmt, args);
		va_end(args);
	}
}

void Logger::warn(const char *fmt, ...)
{
	if (level >= LOG_WARNING && level != LOG_TIMER) {
		va_list args;
		va_start(args, fmt);
		output(LOG_WARNING, fmt, args);
		va_end(args);
	}
}

void Logger::error(const char *fmt, ...)
{
	if (level >= LOG_ERROR) {
		va_list args;
		va_start(args, fmt);
		output(LOG_ERROR, fmt, args);
		va_end(args);
	}
}

void Logger::critical(const char *fmt, ...)
{
	if (level >= LOG_CRITICAL) {
		va_list args;
		va_start(args, fmt);
		output(LOG_CRITICAL, fmt, args);
		va_end(args);
	}
}

void Logger::alert(const char *fmt, ...)
{
	if (level >= LOG_ALERT) {
		va_list args;
		va_start(args, fmt);
		output(LOG_ALERT, fmt, args);
		va_end(args);
	}
}

void Logger::emergency(const char *fmt, ...)
{
	if (level >= LOG_EMERGENCY) {
		va_list args;
		va_start(args, fmt);
		output(LOG_EMERGENCY, fmt, args);
		va_end(args);
	}
}

void Logger::timer(const char *fmt, ...)
{
	if (level == LOG_TIMER) {
		va_list args;
		va_start(args, fmt);
		output(LOG_TIMER, fmt, args);
		va_end(args);
	}
}


LogTimer::LogTimer()
	:_canCount(false),
	 log(NULL)
{
#if defined(NT_GENERIC) || defined(WIN32)
	_canCount = QueryPerformanceFrequency(&_freq) ? true : false;
#elif defined(DARWIN) || defined(OSX)
	_canCount = true;
	mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	_freq = 1.0e9 * (double)info.numer / (double)info.denom;
#elif defined(LINUX)
	_canCount = true;
	clock_getres(CLOCK_MONOTONIC, &_freq);
#endif
	try
	{
		log = new Logger(LOG_TIMER);
		// Disable the timer if level doesn't get set to LOG_TIMER
		if (!log->isLevelEnabled(LOG_TIMER))
		{
			delete log;
			log = NULL;
			_canCount = false;
		}
	}
	catch (bad_alloc&)
	{
		// Catch so we don't crash, but there is nothing much to do
		// except say we can't do this and leave "log" at NULL
		_canCount = false;
		log = NULL;
	}
}

LogTimer::~LogTimer()
{
	delete log;
	log = NULL;
}

void LogTimer::Start(void)
{
	if (_canCount) {
#if defined(NT_GENERIC) || defined(WIN32)
		QueryPerformanceCounter(&_start);
#elif defined(DARWIN) || defined(OSX)
		_start = mach_absolute_time();
#elif defined(LINUX)
		clock_gettime(CLOCK_MONOTONIC, &_start);
#endif
	}
}

void LogTimer::Stop(void)
{
	if (_canCount) {
#if defined(NT_GENERIC) || defined(WIN32)
		QueryPerformanceCounter(&_stop);
		_total.QuadPart = _stop.QuadPart - _start.QuadPart;
#elif defined(DARWIN) || defined(OSX)
		_stop = mach_absolute_time();
		_total = _stop - _start;
#elif defined(LINUX)
		clock_gettime(CLOCK_MONOTONIC, &_stop);
		_total.tv_nsec = _stop.tv_nsec - _start.tv_nsec;
		_total.tv_sec = _stop.tv_sec - _start.tv_sec;
		// Borrow from seconds if nanosecond flipped
		if (_start.tv_nsec > _stop.tv_nsec)
			_total.tv_sec--;
#endif
	}
}



#if defined(NT_GENERIC) || defined(WIN32)
void LogTimer::do_replacements(LARGE_INTEGER& value, string& str, const char *templ, bool do_raw)
#elif defined(DARWIN) || defined(OSX)
void LogTimer::do_replacements(uint64_t value, std::string& str, const char *templ, bool do_raw)
#elif defined(LINUX)
void LogTimer::do_replacements(struct timespec value, std::string& str, const char *templ, bool do_raw)
#endif
{
	string::size_type ix = str.find(templ);
	if (ix != string::npos) {
		// Format the value appropriately
		ostringstream number;
		double dval;
		if (do_raw) {
#if defined(NT_GENERIC) || defined(WIN32)
			dval = (double)value.QuadPart;
#elif defined(DARWIN) || defined(OSX)
			dval = (double)value;
#elif defined(LINUX)
			dval = (double)value.tv_sec * 1.0e9 + (double)value.tv_nsec;
#endif
		}
		else {
#if defined(NT_GENERIC) || defined(WIN32)
			dval = (double)value.QuadPart / (double)_freq.QuadPart;
#elif defined(DARWIN) || defined(OSX)
			dval = (double)value / _freq;
#elif defined(LINUX)
			dval = ((double)value.tv_sec * 1.0e9 + (double)value.tv_nsec) /
			       ((double)_freq.tv_sec * 1.0e9 + (double)_freq.tv_nsec);
#endif
		}
		number << dval << ends;
		string numval = number.str();
		int len = strlen(templ);
		do {
			str.replace(ix, len, numval.c_str());
			ix = str.find(templ, ix);
		} while (ix != string::npos);
	}
}

//
// Recognizes the following substitutions:
//   {$r} = total time in raw form (counts)
//   {$t} = total time in seconds (counts / freq)
//   {$1} = start time in raw form (counts)
//   {$S} = start time in seconds
//   {$2} = stop time in raw form (counts)
//   {$s} = stop time in seconds
void LogTimer::LogTiming(const char *szMessage)
{
	if (log != NULL)
	{
		string message(szMessage);

		do_replacements(_total, message, "{$r}", true);
		do_replacements(_total, message, "{$t}", false);
		do_replacements(_start, message, "{$1}", true);
		do_replacements(_start, message, "{$S}", false);
		do_replacements(_stop, message, "{$2}", true);
		do_replacements(_stop, message, "{$s}", false);

		log->timer("%s", message.c_str());
	}
}


//
// "C" style wrappers -- basically defer to C++ code to do the work
//
TimerHandle InitTimer()
{
	LogTimer *timer = NULL;
	try
	{
		timer = new LogTimer();
	}
	catch (bad_alloc&)
	{
		timer = NULL;
	}
	if (timer != NULL)
	{
		if (!timer->canCount())
		{
			delete timer;
			timer = NULL;
		}
	}
	return static_cast<TimerHandle>(timer);
}

void TermTimer(TimerHandle handle)
{
	LogTimer *timer = static_cast<LogTimer*>(handle);
	delete timer;
}

void StartTimer(TimerHandle handle)
{
	LogTimer *timer = static_cast<LogTimer*>(handle);
	if (timer != NULL)
		timer->Start();
}

void StopTimer(TimerHandle handle)
{
	LogTimer *timer = static_cast<LogTimer*>(handle);
	if (timer != NULL)
		timer->Stop();
}

void ReportTimer(TimerHandle handle, const char *szMessage)
{
	LogTimer *timer = static_cast<LogTimer*>(handle);
	if (timer != NULL)
		timer->LogTiming(szMessage);
}

void ReportSplit(TimerHandle handle, const char *szMessage)
{
	LogTimer *timer = static_cast<LogTimer*>(handle);
	if (timer != NULL)
	{
		// Stop doesn't really stop anything, just sets the "_stop" and "_total" values
		timer->Stop();
		timer->LogTiming(szMessage);
	}
}


void EntryExitLogger::Log(const char *event)
{
	logger->trace("Function '%s' %s", funcName.c_str(), event);
}


EntryExitLogger::EntryExitLogger(Logger *logger, const char *function)
{
	this->logger = logger;
	funcName = string(function);
	Log("entry");
}


EntryExitLogger::EntryExitLogger(Logger *logger, string& function)
{
	this->logger = logger;
	funcName = function;
	Log("entry");
}


EntryExitLogger::~EntryExitLogger()
{
	Log("exit");
}

