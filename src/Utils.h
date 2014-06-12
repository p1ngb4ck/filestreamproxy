/*
 * Utils.h
 *
 *  Created on: 2014. 6. 10.
 *      Author: oskwon
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <map>
#include <string>
#include <vector>

#include <stdint.h>

#include "Source.h"
#include "Encoder.h"
//----------------------------------------------------------------------

int strtollu(std::string data);
std::string ultostr(int64_t data);
std::string trim(std::string& s, const std::string& drop = " \t\n\v\r");
int split(std::string data, const char delimiter, std::vector<std::string>& tokens);
bool split_key_value(std::string data, std::string delimiter, std::string &key, std::string &value);

std::string read_request();
//----------------------------------------------------------------------

typedef enum {
	REQ_TYPE_UNKNOWN = 0,
	REQ_TYPE_LIVE,
	REQ_TYPE_TRANSCODING_LIVE,
	REQ_TYPE_FILE,
	REQ_TYPE_TRANSCODING_FILE
} RequestType;
//----------------------------------------------------------------------

class RequestHeader
{
public:
	RequestType type;
	std::string method;
	std::string path;
	std::string decoded_path;
	std::string version;
	std::map<std::string, std::string> params;

public:
	bool parse_header(std::string header);
};
//----------------------------------------------------------------------

#define HTTP_OK      "HTTP/1.1 200 OK\r\n"
#define HTTP_PARTIAL "HTTP/1.1 206 Partial Content\r\n"
#define HTTP_PARAMS	 "Connection: Close\r\n" \
					 "Content-Type: video/mpeg\r\n" \
					 "Server: transtreamproxy\r\n"
#define HTTP_DONE    "\r\n"
//----------------------------------------------------------------------

typedef struct _thread_params_t {
	Source *source;
	Encoder *encoder;
	RequestHeader *request;
} ThreadParams;
//----------------------------------------------------------------------

off_t make_response(ThreadParams *params, std::string& response);
//----------------------------------------------------------------------

class Util
{
public:
	static void	vlog(const char * format, ...) throw();
};
//----------------------------------------------------------------------

void kill_process(int pid);
std::string get_host_addr();
std::vector<int> find_process_by_name(std::string name, int mypid);
//----------------------------------------------------------------------

#endif /* UTILS_H_ */
