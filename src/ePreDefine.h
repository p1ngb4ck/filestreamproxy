/*
 * main.h
 *
 *  Created on: 2013. 9. 12.
 *      Author: kos
 */

#ifndef FILESTREAMPROXY_H_
#define FILESTREAMPROXY_H_

#define BUFFER_SIZE		(188*256)
#define MAX_LINE_LENGTH	(1024)

#define RETURN_ERR_400(FMT,...) { printf("HTTP/1.0 400 Bad Request\r\n"FMT"\r\n\r\n", ##__VA_ARGS__); return 1; }
#define RETURN_ERR_401(FMT,...) { printf("HTTP/1.0 401 Unauthorized\r\n"FMT"\r\n\r\n",##__VA_ARGS__); return 1; }
#define RETURN_ERR_502(FMT,...) { printf("HTTP/1.0 502 Bad Gateway\r\n"FMT"\r\n\r\n", ##__VA_ARGS__); return 1; }
#define RETURN_ERR_503(FMT,...) { printf("HTTP/1.0 503 Service Unavailable\r\n"FMT"\r\n\r\n", ##__VA_ARGS__); return 1; }
//-------------------------------------------------------------------------------

char* ReadRequest(char* aRequest);

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef DEBUG_LOG
	extern int myPid;
	extern FILE* fpLog;
	#define LOG(X,...) { fprintf(fpLog, "[%d]%s:%s(%d) "X"\n",myPid,__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__); fflush(fpLog); }
#endif /*DEBUG_LOG*/

#endif /* FILESTREAMPROXY_H_ */
