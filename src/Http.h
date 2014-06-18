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
		M3U
	};

	int type;
	std::string method;
	std::string path;
	std::string version;
	std::map<std::string, std::string> params;

	std::string page;
	std::map<std::string, std::string> page_params;

private:

public:
	HttpHeader() : type(UNKNOWN) {}
	virtual ~HttpHeader() {}

	bool parse_request(std::string header);
	std::string build_response(Mpeg *source);

	static std::string read_request();
};
//----------------------------------------------------------------------

#endif /* HTTP_H_ */
