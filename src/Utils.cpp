/*
 * Utils.cpp
 *
 *  Created on: 2014. 6. 10.
 *      Author: oskwon
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <sstream>

#include "mpegts.h"

#include "Utils.h"
#include "Logger.h"
#include "UriDecoder.h"

using namespace std;
//----------------------------------------------------------------------

std::string ultostr(int64_t data)
{
	std::stringstream ss;
	ss << data;
	return ss.str();
}
//----------------------------------------------------------------------

int strtoi(std::string data)
{
	int	retval;
	std::stringstream ss;
	try {
		ss.str(data);
		ss >> retval;
	}
	catch(...) {
		return -1;
	}
	return retval;
}
//----------------------------------------------------------------------

std::string trim(std::string& s, const std::string& drop)
{
	std::string r = s.erase(s.find_last_not_of(drop) + 1);
	return r.erase(0, r.find_first_not_of(drop));
}
//----------------------------------------------------------------------

int split(std::string data, const char delimiter, std::vector<string>& tokens)
{
	std::stringstream data_stream(data);
	for(std::string token; std::getline(data_stream, token, delimiter); tokens.push_back(trim(token)));
	return tokens.size();
}
//----------------------------------------------------------------------

bool split_key_value(std::string data, std::string delimiter, std::string &key, std::string &value)
{
	int idx = data.find(delimiter);
	if (idx == string::npos) {
		WARNING("split key & value (data : %s, delimiter : %s)", data.c_str(), delimiter.c_str());
		return false;
	}
	key = data.substr(0, idx);
	value = data.substr(idx+1, data.length()-idx);
	return true;
}
//----------------------------------------------------------------------

std::string read_request()
{
	std::string request = "";
	while (true) {
		char buffer[128] = {0};
		fgets(buffer, 127, stdin);

		request += buffer;
		if(request.find("\r\n\r\n") != string::npos)
			break;
	}
	return request;
}
//----------------------------------------------------------------------

bool RequestHeader::parse_header(std::string header)
{
	std::vector<string> lines;
	split(header, '\n', lines);

	DEBUG("header lines count : %d", lines.size());
	std::vector<string>::iterator iter = lines.begin();
	std::vector<string> infos;
	if (split(*iter, ' ', infos) != 3) {
		ERROR("fail to parse info : %d", infos.size());
		return false;
	}

	type    = REQ_TYPE_TRANSCODING_LIVE;
	method  = infos[0];
	path    = infos[1];
	version = infos[2];
	decoded_path = path;

	if (strncmp(path.c_str(), "/file", 5) == 0) {
		std::string key = "", value = "";
		if (!split_key_value(path, "=", key, value)) {
			ERROR("fail to parse path(file) : %s", path.c_str());
			return false;
		}
		if (key != "/file?file") {
			ERROR("unknown request file path (key : %s, value : %s)", key.c_str(), value.c_str());
			return false;
		}
		type = REQ_TYPE_TRANSCODING_FILE;
		decoded_path = UriDecoder().decode(value.c_str());
	}

	DEBUG("info (%d) -> type : [%s], path : [%s], version : [%s]", infos.size(), method.c_str(), path.c_str(), version.c_str());

	for (++iter; iter != lines.end(); ++iter) {
		std::string key = "", value = "";
		if (!split_key_value(*iter, ":", key, value))
			continue;
		if (key == "")
			continue;
		key = trim(key);
		value = trim(value);

		if (key.length() > 0) {
			params[key] = value;
			DEBUG("add params : %s -> %s", key.c_str(), value.c_str());
		}
	}
	return true;
}
//----------------------------------------------------------------------

off_t make_response(ThreadParams *params, std::string& response)
{
	response = "";

	LINESTAMP();

	off_t byte_offset = 0;
	RequestHeader *header = ((ThreadParams*) params)->request;
	switch(header->type) {
	case REQ_TYPE_TRANSCODING_FILE: {
			MpegTS *source = (MpegTS*)((ThreadParams*) params)->source;

			std::string range = header->params["Range"];
			if((range.length() > 7) && (range.substr(0, 6) == "bytes=")) {
				range = range.substr(6);
				if(range.find('-') == (range.length() - 1)) {
					byte_offset = strtoi(range);
				}
			}

			response += (byte_offset > 0) ? HTTP_PARTIAL : HTTP_OK;
			response += HTTP_PARAMS;
			response += "Accept-Ranges: bytes\r\n"
					    "Content-Length: " + ultostr(source->stream_length) + "\r\n";
			response += HTTP_DONE;
		}
		break;
	case REQ_TYPE_TRANSCODING_LIVE: {
			response += HTTP_OK;
			response += HTTP_PARAMS;
			response += HTTP_DONE;
		}
		break;
	default: return -1;
	}
	return byte_offset;
}
//----------------------------------------------------------------------

void Util::vlog(const char * format, ...) throw()
{
	static char vlog_buffer[MAX_PRINT_LEN];
    memset(vlog_buffer, 0, MAX_PRINT_LEN);

    va_list args;
	va_start(args, format);
	vsnprintf(vlog_buffer, MAX_PRINT_LEN-1, format, args);
	va_end(args);

	WARNING("%s", vlog_buffer);
}
//----------------------------------------------------------------------
