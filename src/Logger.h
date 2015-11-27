/*
 * Logger.h
 *
 *  Created on: 2014. 2. 7.
 *      Author: oskwon
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
//----------------------------------------------------------------------

#define MAX_PRINT_LEN	2048
#define DEFAULT_TIMESTAMP_FORMAT "%Y%m%d-%H%M%S"

#ifdef _DISABLE_LOGGER
#	define ERROR(fmt,...)   {}
#	define WARNING(fmt,...) {}
#	define INFO(fmt,...)    {}
#	define DEBUG(fmt,...)   {}
#	define LOG(fmt,...)     {}
#	define LINESTAMP(fmt,...)  {}
#	define HEXLOG(fmt,...)  {}
#	define DUMMY(fmt,...)   {}
#else
#	define ERROR(fmt,...)  { Logger::instance()->log(Logger::ERROR,   fmt" (%s, %s:%d)", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); }
#	define WARNING(fmt,...){ Logger::instance()->log(Logger::WARNING, fmt" (%s, %s:%d)", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); }
#	define INFO(fmt,...)   { Logger::instance()->log(Logger::INFO,    fmt" (%s, %s:%d)", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); }
#	define DEBUG(fmt,...)  { Logger::instance()->log(Logger::DEBUG,   fmt" (%s, %s:%d)", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); }
#	define LOG(fmt,...)    { Logger::instance()->log(Logger::LOG,     fmt" (%s, %s:%d)", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); }
#	define LINESTAMP(fmt,...)  { Logger::instance()->log(fmt" (%s, %s:%d)", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); }
#	define HEXLOG(header, buffer, length) { Logger::instance()->hexlog(header, buffer, length, " (%s, %s:%d)", __FILE__, __FUNCTION__, __LINE__); }
#	define DUMMY(fmt,...)  { Logger::instance()->log(fmt, ##__VA_ARGS__); }
#endif /* USE_DEBUG */
//----------------------------------------------------------------------

char* get_timestamp();

class Logger
{
private:
	int mLogLevel, mPid;
	FILE* mLogHandle;

	static Logger* mInstHandle;

private:
	Logger();
	virtual ~Logger();

	static void logger_release()
	{
		if (mInstHandle) {
			if (Logger::instance()->get_level() >= Logger::INFO) {
				DUMMY("Logger Released.");
			}
			delete mInstHandle;
		}
	};

public:
	enum { NONE = 0, ERROR, WARNING, INFO, DEBUG, LOG };

#ifndef _DISABLE_LOGGER
	bool init(const char* aFileName = 0, int aLogLevel = Logger::ERROR, bool aWithTimestamp = false);

	void log(const char* aFormat, ...);
	void log(int aLogLevel, const char* aFormat, ...);
	void hexlog(const char *header, const char *buffer, const int length, const char *aFormat, ...);

	static Logger* instance();
#endif

	void set_pid();
	int get_level() { return mLogLevel; }
};
//----------------------------------------------------------------------

#endif /* ULOGGER_H_ */
