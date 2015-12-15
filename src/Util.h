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

#include "Http.h"
#include "Source.h"
#include "Encoder.h"
//----------------------------------------------------------------------

class Util {
public:
	static void	vlog(const char * format, ...) throw();

	static int strtollu(std::string data);
	static std::string ultostr(int64_t data);

	static std::string trim(std::string& s, const std::string& drop = " \t\n\v\r");

	static int split(std::string data, const char delimiter, std::vector<std::string>& tokens);
	static bool split_key_value(std::string data, std::string delimiter, std::string &key, std::string &value);

	static void kill_process(int pid);

	static std::string host_addr();

	static std::vector<int> find_process_by_name(std::string name, int mypid);

	static int get_encoder_count();
};
//----------------------------------------------------------------------

typedef struct _thread_params_t {
	Source *source;
	Encoder *encoder;
	HttpHeader *request;
} ThreadParams;
//----------------------------------------------------------------------

#endif /* UTILS_H_ */
