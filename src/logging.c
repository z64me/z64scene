//
// logging.c <z64.me>
//
// basic logging
//

#include <stdio.h>
#include <stdarg.h>

#include "logging.h"

// default log level
static enum LogLevel gCurrentLogLevel =
#ifdef NDEBUG
	LOG_LEVEL_INFO
#else
	LOG_LEVEL_DEBUG
#endif
;

void LogLevelSet(const enum LogLevel level)
{
	gCurrentLogLevel = level;
}

void LogMessage(const char *unit, const int line, enum LogLevel level, const char *fmt, ...)
{
	va_list args;
	
	if (!fmt || !*fmt)
		return;
	
	if (level < gCurrentLogLevel || level >= LOG_LEVEL_COUNT)
		return;
	
	// matches enum LogLevel
	const char *logLevelString[] = {
		[LOG_LEVEL_DEBUG] = "DEBUG",
		[LOG_LEVEL_INFO] = "INFO",
		[LOG_LEVEL_WARN] = "WARN",
		[LOG_LEVEL_ERROR] = "ERROR",
	};
	
	fprintf(stderr, "[%s] ", logLevelString[level]);
	
	if (unit)
	{
		fprintf(stderr, "[%s", unit);
		
		if (line > 0)
			fprintf(stderr, ":%d", line);
		
		fprintf(stderr, "] ");
	}
	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	fprintf(stderr, "\n");
}
