/*
 * Http.h
 *
 *  Created on: 2014. 6. 18.
 *      Author: oskwon
 */

#ifndef HTTP_H_
#define HTTP_H_

#include <map>
#include <string>

#include "Mpeg.h"
//----------------------------------------------------------------------

class HttpHeader
{
public:
	enum {
		UNKNOWN = 0,
		TRANSCODING_LIVE,
		TRANSCODING_FILE,
		M3U,
		TRANSCODING_FILE_CHECK,
		TRANSCODING_LIVE_STOP,
		TRANSCODING_MAX
	};

	int type;
	std::string method;
	std::string path;
	std::string version;
	std::map<std::string, std::string> params;

	std::string page;
	std::map<std::string, std::string> page_params;

	std::string authorization;
public:
	HttpHeader() : type(UNKNOWN) {}
	virtual ~HttpHeader() {}

	bool parse_request(std::string header);
	std::string build_response(Mpeg *source);

	static std::string read_request();
};
//----------------------------------------------------------------------

namespace HttpUtil {
	std::string http_error(int errcode, std::string errmsg);
};
//----------------------------------------------------------------------

#endif /* HTTP_H_ */
