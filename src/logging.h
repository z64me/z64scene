//
// logging.h <z64.me>
//
// basic logging
//

#ifndef LOGGING_H_INCLUDED
#define LOGGING_H_INCLUDED

enum LogLevel
{
	LOG_LEVEL_DEBUG = 0,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_COUNT,
};

void LogLevelSet(const enum LogLevel level);
void LogMessage(const char *unit, const int line, enum LogLevel level, const char *fmt, ...)  __attribute__ ((format (printf, 4, 5)));

#define LogDebug(fmt, ...) LogMessage(__FILE__, __LINE__, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LogInfo(fmt, ...)  LogMessage(__FILE__, __LINE__, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LogWarn(fmt, ...)  LogMessage(__FILE__, __LINE__, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LogError(fmt, ...) LogMessage(__FILE__, __LINE__, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#ifdef NDEBUG
	#undef LogDebug
	#define LogDebug(fmt, ...) do { } while(0)
#endif

#endif // LOGGING_H_INCLUDED
