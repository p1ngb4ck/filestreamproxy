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
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
extern FILE* fpLog;
#define LOG(X,...) { fprintf(fpLog, "%s:%s(%d) "X"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__); fflush(fpLog); }
#endif

#endif /* FILESTREAMPROXY_H_ */
