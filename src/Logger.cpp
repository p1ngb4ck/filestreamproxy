/*
 * logger.cpp
 *
 *  Created on: 2014. 2. 7.
 *      Author: kos
 */

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include "Logger.h"
//----------------------------------------------------------------------

#define USE_COLOR_LOG 1

static char log_data_buffer[MAX_PRINT_LEN] = {0};
#ifdef USE_COLOR_LOG
static const char* LOG_LV_STR[] = {
		"[   NONE]",
		"\e[1;31m[  ERROR]\e[00m",
		"\e[1;33m[WARNING]\e[00m",
		"\e[1;32m[   INFO]\e[00m",
		"\e[1;36m[  DEBUG]\e[00m",
		"[    LOG]"
};
#else
static const char* LOG_LV_STR[] = {
		"[   NONE]",
		"[  ERROR]",
		"[WARNING]",
		"[   INFO]",
		"[  DEBUG]",
		"[    LOG]"
};
#endif
//----------------------------------------------------------------------

char* get_timestamp()
{
	time_t rawtime;
	struct tm *timeinfo;
	static char buffer[80];

	memset(buffer, 0, 80);

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 80, "%Y%m%d-%H%M%S", timeinfo);

	return buffer;
}
//----------------------------------------------------------------------

Logger::Logger()
	: mLogLevel(0), mLogHandle(0)
{
	mPid = getpid();
}
//----------------------------------------------------------------------

Logger::~Logger()
{
	if (mLogHandle) {
		fclose(mLogHandle);
		mLogHandle = 0;
	}
}
//----------------------------------------------------------------------

void Logger::set_pid()
{
	mPid = getpid();
}
//----------------------------------------------------------------------

Logger* Logger::instance()
{
	if (mInstHandle == 0) {
		mInstHandle = new Logger();
		atexit(logger_release);
	}
	return mInstHandle;
}
//----------------------------------------------------------------------

bool Logger::init(const char* aName, int aLogLevel, bool aWithTimestamp)
{
	if (access("/tmp/.debug_on", F_OK) == 0) {
		FILE *fp = fopen("/tmp/.debug_on", "r");

		int lv = 0;
		fscanf(fp, "%d", &lv);
		if (Logger::NONE < lv && lv <= Logger::LOG) {
			mLogLevel = lv;
		}
		else {
			mLogLevel = aLogLevel;
		}
		fclose(fp);
	}
	else {
		mLogLevel = aLogLevel;
	}

	if (aName == NULL) {
		mLogHandle = stdout;
		INFO("logger initialized.");
		return true;
	}
	char path[256] = {0};
	sprintf(path, "%s.log", aName);
	if (!(mLogHandle = fopen(path, "a+"))) {
		mLogHandle = 0;
//		printf("fail to open logger [%s].", path);
		return false;
	}

	if (mLogLevel >= Logger::INFO) {
#if defined(_MAJOR) && defined(_MINOR)
		DUMMY("Logger initialized. (Ver %d.%d)", _MAJOR, _MINOR);
#else
		DUMMY("Logger initialized.");
#endif
	}
	return true;
}
//----------------------------------------------------------------------

void Logger::hexlog(const char *header, const char *buffer, const int length, const char *aFormat, ...)
{
	int offset = 0, i = 0, ll = 0;

	FILE* output = mLogHandle;

	memset(log_data_buffer, 0, MAX_PRINT_LEN);

	va_list args;
	va_start(args, aFormat);
	ll = vsnprintf(log_data_buffer, MAX_PRINT_LEN-1, aFormat, args);
	va_end(args);

	if (ll > MAX_PRINT_LEN - 1) {
		ll = MAX_PRINT_LEN - 1;
	}
	fprintf(output, "%s\n", log_data_buffer);

	fprintf(output, "HEX DUMP : [%s]-[%d]\n", header, length);
	fprintf(output, "-----------------------------------------------------------------------------\n");
	while (offset < length) {
		char *tmp = (char*) (buffer + offset);
		int  tmp_len  = (offset + 16 < length) ? 16 : (length - offset);

		fprintf(output, "%08X:  ", offset);
		for (i = 0; i < tmp_len; i++) {
			if (i == 8) fprintf(output, " ");
			fprintf(output, "%02X ", (unsigned char) tmp[i]);
		}

		for (i = 0; i <= (16 - tmp_len) * 3; i++)
			fprintf(output, " ");
		if (tmp_len < 9) fprintf(output, " ");

		for (i = 0; i < tmp_len; i++)
			fprintf(output, "%c", (tmp[i] >= 0x20 && tmp[i] <= 0x7E) ? tmp[i] : '.');
		offset += 16; fprintf(output, "\n");
	}
	if (offset == 0) fprintf(output, "%08X:  ", offset);
	fprintf(output, "-----------------------------------------------------------------------------\n");
	fflush(output);
}
//----------------------------------------------------------------------

void Logger::log(const char* aFormat, ...)
{
	memset(log_data_buffer, 0, MAX_PRINT_LEN);

	va_list args;
	va_start(args, aFormat);
	vsnprintf(log_data_buffer, MAX_PRINT_LEN-1, aFormat, args);
	va_end(args);

	fprintf(mLogHandle, "%s\n", log_data_buffer);
	fflush(mLogHandle);
}
//----------------------------------------------------------------------

void Logger::log(int aLogLevel, const char* aFormat, ...)
{
#ifndef _DISABLE_LOGGER
	if (aLogLevel > mLogLevel || mLogHandle == 0) {
		//printf("mLogHandle : %p, mLogLevel : %d, aLogLevel : %d\n", mLogHandle, mLogLevel, aLogLevel);
		return;
	}

	memset(log_data_buffer, 0, MAX_PRINT_LEN);
	va_list args;
	va_start(args, aFormat);
	vsnprintf(log_data_buffer, MAX_PRINT_LEN-1, aFormat, args);
	va_end(args);
	fprintf(mLogHandle, "%s[%d] %s\n", LOG_LV_STR[aLogLevel], mPid, log_data_buffer);
	fflush(mLogHandle);
#endif
}
//----------------------------------------------------------------------

Logger* Logger::mInstHandle = 0;
//----------------------------------------------------------------------
