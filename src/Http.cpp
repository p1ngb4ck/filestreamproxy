/*
 * Http.cpp
 *
 *  Created on: 2014. 6. 18.
 *      Author: oskwon
 */

#include <string.h>
#include <unistd.h>
#include <sstream>

#include "Util.h"
#include "Logger.h"

#include "Http.h"
#include "UriDecoder.h"

using namespace std;
//----------------------------------------------------------------------

bool HttpHeader::parse_request(std::string header)
{
	std::string line, key, value;
	std::istringstream request_stream;
	request_stream.str(header);

	request_stream >> method;
	request_stream >> path;
	request_stream >> version;
	std::getline(request_stream, line);

	while(std::getline(request_stream, line)) {
		if ((line = Util::trim(line)) != "") {
			Util::split_key_value(line, ":", key, value);

			key   = Util::trim(key);
			value = Util::trim(value);

			params[key] = value;
			DEBUG("add param : [%s] - [%s]", key.c_str(), value.c_str());
		}
	}

	int idx = path.find("?");
	// page
	if (idx != std::string::npos) {
		page = path.substr(0,idx);
		std::string page_param = path.substr(idx + 1);

		DEBUG("request url : [%s] - [%s]", page.c_str(), page_param.c_str());
		std::istringstream request_params_stream;
		request_params_stream.str(page_param);
		while(std::getline(request_params_stream, line, '&')) {
			if ((line = Util::trim(line)) != "") {
				Util::split_key_value(line, "=", key, value);

				key   = Util::trim(key);
				value = Util::trim(value);

				page_params[key] = value;
				DEBUG("add page param : [%s] - [%s]", key.c_str(), value.c_str());
			}
		}

		if (page == "/file") {
			if (page_params["check"] == "valid") {
				type = HttpHeader::TRANSCODING_FILE_CHECK;
			}
			else {
				type = HttpHeader::TRANSCODING_FILE;
			}
		}
		else if (page == "/m3u") {
			type = HttpHeader::M3U;
		}
		else if (page == "/live") {
			if (page_params["cmd"] == "stop") {
				type = HttpHeader::TRANSCODING_LIVE_STOP;
			}
		}
	}
	// live
	else {
		type = HttpHeader::TRANSCODING_LIVE;
	}
	return true;
}
//----------------------------------------------------------------------

static const char *http_ok          = "HTTP/1.1 200 OK\r\n";
static const char *http_partial     = "HTTP/1.1 206 Partial Content\r\n";
static const char *http_connection  = "Connection: Close\r\n";
static const char *http_server      = "Server: transtreamproxy\r\n";
static const char *http_done        = "\r\n";
std::string HttpHeader::build_response(Mpeg *source)
{
	std::ostringstream oss;

	switch(type) {
	case HttpHeader::TRANSCODING_FILE_CHECK: {
			oss << http_ok;
			oss << http_connection;
			oss << "Content-Type: text/plain\r\n";
			oss << http_server;
			oss << http_done;
		}
		break;
	case HttpHeader::TRANSCODING_FILE: {
			std::string range = params["Range"];
			off_t seek_offset = 0, content_length = 0;

			if((range.length() > 7) && (range.substr(0, 6) == "bytes=")) {
				range = range.substr(6);
				if(range.find('-') == (range.length() - 1)) {
					seek_offset = Util::strtollu(range);
				}
			}

			content_length = source->stream_length - seek_offset;
			if (seek_offset > 0) {
				content_length += 1;
				oss << http_partial;
			}
			else {
				oss << http_ok;
			}
			oss << http_connection;
			oss << "Content-Type: video/mpeg\r\n";
			oss << http_server;
			oss << "Accept-Ranges: bytes\r\n";
			oss << "Content-Length: " << Util::ultostr(content_length) << "\r\n";
			oss << "Content-Range: bytes " <<
					Util::ultostr(seek_offset) << "-" <<
					Util::ultostr(source->stream_length - 1) << "/" <<
					Util::ultostr(source->stream_length) << "\r\n";
			oss << http_done;
		}
		break;
	case HttpHeader::TRANSCODING_LIVE: {
			oss << http_ok;
			oss << http_connection;
			oss << "Content-Type: video/mpeg\r\n";
			oss << http_server;
			oss << http_done;
		}
		break;
	case HttpHeader::M3U: {
			std::ostringstream m3u_oss;
			m3u_oss << "#EXTM3U\n";
			m3u_oss << "#EXTVLCOPT--http-reconnect=true\n";
			m3u_oss << "http://" << params["Host"] << "/file?file=" << page_params["file"];
			if (page_params["position"] != "") {
				m3u_oss << "&position=" << page_params["position"];
			}
			m3u_oss << "\n";
			m3u_oss << http_done;

			std::string m3u_content = m3u_oss.str();

			oss << http_partial;
			oss << "Content-Type: audio/x-mpegurl\r\n";
			oss << "Accept-Ranges: bytes\r\n";
			oss << http_connection;
			oss << http_server;
			oss << "Content-Length: " << Util::ultostr(m3u_content.length()) << "\r\n";
			oss << "Content-Range: bytes 0-" <<
					Util::ultostr(m3u_content.length() - 1) << "/" <<
					Util::ultostr(m3u_content.length()) << "\r\n";
			oss << http_done;
			oss << m3u_content;
		}
		break;
	default: return "";
	}
	return oss.str();
}
//----------------------------------------------------------------------

bool terminated();
std::string HttpHeader::read_request()
{
	std::string request = "";
	while (true) {
		char buffer[128] = {0};
		if (!::read (0, buffer, 127)) {
			break;
		}
		request += buffer;
		if(request.find("\r\n\r\n") != string::npos)
			break;

		if (terminated()) {
			request = "";
			break;
		}
		::usleep(0);
	}
	return request;
}
//----------------------------------------------------------------------

std::string HttpUtil::http_error(int errcode, std::string errmsg)
{
	std::ostringstream oss;

	oss << "HTTP/1.1 " << Util::ultostr(errcode) << " " << errmsg << "\r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Connection: close\r\n";
	oss << "Accept-Ranges: bytes\r\n";
	oss << "\r\n";

	return oss.str();
}
//----------------------------------------------------------------------
