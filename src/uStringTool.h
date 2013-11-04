/*
 * uStringTool.h
 *
 *  Created on: 2013. 10. 30.
 *      Author: kos
 */

#ifndef USTRINGTOOL_H_
#define USTRINGTOOL_H_

#include <string>
#include <vector>
#include <sstream>

using namespace std;

#define trim_default_delimiter " \t\n\v\r"

namespace uStringTool
{
	inline std::string Trim(std::string& s, const std::string& drop = trim_default_delimiter)
	{
		std::string r = s.erase(s.find_last_not_of(drop) + 1);
		return r.erase(0, r.find_first_not_of(drop));
	}

	inline std::string RTrim(std::string s, const std::string& drop = trim_default_delimiter)
	{
		std::string r = s.erase(s.find_last_not_of(drop) + 1);
		return r;
	}

	inline std::string LTrim(std::string s, const std::string& drop = trim_default_delimiter)
	{
		std::string r = s.erase(0, s.find_first_not_of(drop));
		return r;
	}

	inline std::string ReplaceAll(const std::string &in, const std::string &entity, const std::string &symbol)
	{
		std::string out = in;
		std::string::size_type loc = 0;
		while (( loc = out.find(entity, loc)) != std::string::npos )
			out.replace(loc, entity.length(), symbol);
		return out;
	}

	inline int Split(std::string data, const char delimiter, std::vector<string>& tokens)
	{
			std::stringstream data_stream(data);
			for(std::string token; std::getline(data_stream, token, delimiter); tokens.push_back(Trim(token)));
			return tokens.size();
	}
};

#endif /* USTRINGTOOL_H_ */
