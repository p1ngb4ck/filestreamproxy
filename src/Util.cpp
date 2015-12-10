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
#include <dirent.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <sstream>
#include <fstream>

#include "Util.h"
#include "Logger.h"

using namespace std;
//----------------------------------------------------------------------

std::string Util::ultostr(int64_t data)
{
	std::stringstream ss;
	ss << data;
	return ss.str();
}
//----------------------------------------------------------------------

int Util::strtollu(std::string data)
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

std::string Util::trim(std::string& s, const std::string& drop)
{
	std::string r = s.erase(s.find_last_not_of(drop) + 1);
	return r.erase(0, r.find_first_not_of(drop));
}
//----------------------------------------------------------------------

int Util::split(std::string data, const char delimiter, std::vector<string>& tokens)
{
	std::stringstream data_stream(data);
	for(std::string token; std::getline(data_stream, token, delimiter); tokens.push_back(trim(token)));
	return tokens.size();
}
//----------------------------------------------------------------------

bool Util::split_key_value(std::string data, std::string delimiter, std::string &key, std::string &value)
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

std::string Util::host_addr()
{
	std::stringstream ss;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    getpeername(0, (struct sockaddr*)&addr, &addrlen);
    ss << inet_ntoa(addr.sin_addr);

    return ss.str();
}
//----------------------------------------------------------------------

std::vector<int> Util::find_process_by_name(std::string name, int mypid)
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
//----------------------------------------------------------------------


void Util::kill_process(int pid)
{
	int result = 0;

	result = kill(pid, SIGINT);
	DEBUG("SEND SIGINT to %d, result : %d", pid, result);
}
//----------------------------------------------------------------------

int Util::get_encoder_count()
{
	int max_encodr_count = 0;
	DIR* d = opendir("/dev");
	if (d != 0) {
		struct dirent* de;
		while ((de = readdir(d)) != 0) {
			if (strncmp("bcm_enc", de->d_name, 7) == 0) {
				max_encodr_count++;
			}
		}
		closedir(d);
	}
	return max_encodr_count;
}
//----------------------------------------------------------------------

