/*
 * Utils.cpp
 *
 *  Created on: 2014. 6. 10.
 *      Author: oskwon
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <sstream>
#include <fstream>

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

int strtollu(std::string data)
{
	long long retval;
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

	if (strncmp(path.c_str(), "/file", 5) == 0) {
		std::vector<std::string> tokens;
		if (split(path.substr(6), '&', tokens) > 0) {
			for (int i = 0; i < tokens.size(); ++i) {
				std::string data = tokens[i];
				std::string key = "", value = "";
				if (!split_key_value(data, "=", key, value)) {
					ERROR("fail to request : %s", data.c_str());
					continue;
				}
				if (key == "file") {
					extension[key] = UriDecoder().decode(value.c_str());;
					continue;
				}
				extension[key] = value;
			}
		}
		type = REQ_TYPE_TRANSCODING_FILE;

//		DEBUG(":: HEADER :: %s", extension["file"].c_str());
//		std::map<std::string, std::string>::iterator iter = extension.begin();
//		for (; iter != extension.end(); ++iter) {
//			std::string key = iter->first;
//			std::string value = iter->second;
//			DEBUG("[%s] -> [%s]", key.c_str(), value.c_str());
//		}
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
					byte_offset = strtollu(range);
				}
			}

			off_t content_length = source->stream_length - byte_offset;
			if (byte_offset > 0) {
				content_length += 1;
				response += HTTP_PARTIAL;
			}
			else {
				response += HTTP_OK;
			}
			response += HTTP_PARAMS;
			response += "Accept-Ranges: bytes\r\n"
					    "Content-Length: " + ultostr(content_length) + "\r\n";
			response += string("Content-Range: bytes ") +
					ultostr(byte_offset) + "-" +
					ultostr(source->stream_length - 1) + "/" +
					ultostr(source->stream_length) + "\r\n";
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

std::string get_host_addr()
{
	std::stringstream ss;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    getpeername(0, (struct sockaddr*)&addr, &addrlen);
    ss << inet_ntoa(addr.sin_addr);

    return ss.str();
}
//-------------------------------------------------------------------------------

std::vector<int> find_process_by_name(std::string name, int mypid)
{
	std::vector<int> pidlist;
	char cmdlinepath[256] = {0};
	DIR* d = opendir("/proc");
	if (d != 0) {
		struct dirent* de;
		while ((de = readdir(d)) != 0) {
			int pid = atoi(de->d_name);
			if (pid > 0) {
				sprintf(cmdlinepath, "/proc/%s/cmdline", de->d_name);

				std::string cmdline;
				std::ifstream cmdlinefile(cmdlinepath);
				std::getline(cmdlinefile, cmdline);
				if (!cmdline.empty()) {
					size_t pos = cmdline.find('\0');
					if (pos != string::npos)
					cmdline = cmdline.substr(0, pos);
					pos = cmdline.rfind('/');
					if (pos != string::npos)
					cmdline = cmdline.substr(pos + 1);
					if ((name == cmdline) && ((mypid != pid) || (mypid == 0))) {
						pidlist.push_back(pid);
					}
				}
			}
		}
		closedir(d);
	}
	return pidlist;
}
//-------------------------------------------------------------------------------

void kill_process(int pid)
{
	int result = kill(pid, SIGINT);
	DEBUG("SEND SIGINT to %d, result : %d", pid, result);
	sleep(1);
}
//----------------------------------------------------------------------
