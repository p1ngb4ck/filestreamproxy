/*
 * Demux.h
 *
 *  Created on: 2014. 6. 11.
 *      Author: oskwon
 */

#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/version.h>

#include "Utils.h"
#include "Logger.h"
#include "Demuxer.h"

using namespace std;
//-------------------------------------------------------------------------------

std::string Demuxer::webif_reauest(std::string service, std::string auth) throw(trap)
{
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		throw(trap("webif create socket fail."));

	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(80);
	sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(sock, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in)))
		throw(trap("webif connect fail."));

	std::string request = string("GET /web/stream?StreamService=") + service + " HTTP/1.0\r\n";
	if (auth != "")
		request += "Authorization: " + auth + "\r\n";
	request += "\r\n";

	if (write(sock, request.c_str(), request.length()) != request.length())
		throw(trap("webif send(request) fail."));
	DEBUG("webif request :\n", request.c_str());

	std::string response = "";
	struct pollfd pollevt[2];
	pollevt[0].fd      = sock;
	pollevt[0].events  = POLLIN;
	for (;;) {
		char buffer[1024] = {0};

		pollevt[0].revents = 0;
		int poll_state = poll(pollevt, 1, 1000);
		if (poll_state == 0)
			break;
		else if (poll_state < 0) {
			ERROR("webif receive poll error : %s (%d)", strerror(errno), errno);
			throw(trap("webif receive response error."));
		}
		if (pollevt[0].revents & POLLIN) {
			if (read(sock, buffer, 1024) <= 0) {
				break;
			}
			response += buffer;
		}
	}
	return response;
}
//-------------------------------------------------------------------------------

bool Demuxer::already_exist(std::vector<unsigned long> &pidlist, int pid)
{
	for(int i = 0; i < pidlist.size(); ++i) {
		if(pidlist[i] == pid)
			return true;
	}
	return false;
}
//-------------------------------------------------------------------------------

void Demuxer::set_filter(std::vector<unsigned long> &new_pids) throw(trap)
{
	struct dmx_pes_filter_params filter;
	ioctl(fd, DMX_SET_BUFFER_SIZE, 1024*1024);

	filter.pid = new_pids[0];
	filter.input = DMX_IN_FRONTEND;
#if DVB_API_VERSION > 3
	filter.output = DMX_OUT_TSDEMUX_TAP;
	filter.pes_type = DMX_PES_OTHER;
#else
	filter.output = DMX_OUT_TAP;
	filter.pes_type = DMX_TAP_TS;
#endif
	filter.flags = DMX_IMMEDIATE_START;

	if (::ioctl(fd, DMX_SET_PES_FILTER, &filter) < 0)
		throw(trap("demux filter setting failed."));
	DEBUG("demux filter setting ok.");

	for(int i = 0; i < new_pids.size(); ++i) {
		uint16_t pid = new_pids[i];

		if(already_exist(pids, pid))
			continue;
		LOG("demux add pid (%x).", pid);

#if DVB_API_VERSION > 3
		if (::ioctl(fd, DMX_ADD_PID, &pid) < 0)
			throw(trap("demux add pid failed."));
#else
		if (::ioctl(fd, DMX_ADD_PID, pid) < 0)
			throw(trap("demux add pid failed."));
#endif
	}

	for(int i = 0; i < pids.size(); ++i) {
		uint16_t pid = pids[i];
		if(already_exist(new_pids, pid))
			continue;
		if(i == 4) break;

		LOG("demux remove pid (%x).", pid);
#if DVB_API_VERSION > 3
		::ioctl(fd, DMX_REMOVE_PID, &pid);
#else
		::ioctl(fd, DMX_REMOVE_PID, pid);
#endif
	}
	DEBUG("demux setting PID ok.");
	pids = new_pids;
}
//-------------------------------------------------------------------------------

bool Demuxer::parse_webif_response(std::string& response, std::vector<unsigned long> &new_pids)
{
	int start_idx, end_idx;
	if ((start_idx = response.rfind('+')) == string::npos)
		return false;
	if ((end_idx = response.find('\n', start_idx)) == string::npos)
		return false;

	std::string line = response.substr(start_idx, end_idx - start_idx);
	if (line.length() < 3 || line.at(0) != '+')
		return false;

	/*+0:0:pat,17d4:pmt,17de:video,17e8:audio,17e9:audio,17eb:audio,17ea:audio,17f3:subtitle,17de:pcr,17f2:text*/
	demux_id = atoi(line.substr(1,1).c_str());

	std::vector<std::string> pidtokens;
	if (split(line.c_str() + 3, ',', pidtokens)) {
		for (int i = 0; i < pidtokens.size(); ++i) {
			std::string pidstr, pidtype;
			std::string toekn = pidtokens[i];
			if (!split_key_value(toekn, ":", pidstr, pidtype))
				continue;

			unsigned long pid = strtoul(pidstr.c_str(), 0, 0x10);
			if (pid == -1) continue;

			if (!video_pid || !audio_pid || !pmt_pid) {
				if (pidtype == "pat") {
					pat_pid = pid;
				}
				else if (pidtype == "pmt") {
					pmt_pid = pid;
				}
				else if (pidtype == "video") {
					video_pid = pid;
				}
				else if (pidtype == "audio") {
					audio_pid = pid;
				}
			}
			if (!already_exist(new_pids, pid)) {
				new_pids.push_back(pid);
			}
			DEBUG("find pid : %s - %04X", toekn.c_str(), pid);
		}
	}
	return true;
}
//-------------------------------------------------------------------------------

Demuxer::Demuxer(RequestHeader *header) throw(trap)
{
	demux_id = pat_pid = fd = sock = -1;
	pmt_pid = audio_pid = video_pid = 0;

	std::string auth = header->params["WWW-Authenticate"];
	std::string service = header->path.substr(1);
	std::string webif_response = webif_reauest(service, auth);
	DEBUG("webif response :\n%s", webif_response.c_str());

	std::vector<unsigned long> new_pids;
	if (!parse_webif_response(webif_response, new_pids))
		throw(trap("webif response parsing fail."));

	std::string demuxpath = "/dev/dvb/adapter0/demux" + ultostr(demux_id);
	if ((fd = open(demuxpath.c_str(), O_RDWR | O_NONBLOCK)) < 0) {
		throw(trap(std::string("demux open fail : ") + demuxpath));
	}
	INFO("demux open success : %s", demuxpath.c_str());

	set_filter(new_pids);
}
//-------------------------------------------------------------------------------

Demuxer::~Demuxer() throw()
{
	if (fd != -1) close(fd);
	if (sock != -1) close(sock);

	fd = sock = -1;
}
//-------------------------------------------------------------------------------

int Demuxer::get_fd() const throw()
{
	return fd;
}
//-------------------------------------------------------------------------------
