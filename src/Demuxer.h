/*
 * Demux.h
 *
 *  Created on: 2014. 6. 11.
 *      Author: oskwon
 */

#ifndef DEMUX_H_
#define DEMUX_H_

#include <vector>
#include <string>

#include "3rdparty/trap.h"

#include "Util.h"
#include "Http.h"
#include "Source.h"
#include "Mutex.h"
//----------------------------------------------------------------------

class Demuxer : public Source
{
private:
	int fd;
	int sock;

	int demux_id;
	int pat_pid;
	std::vector<unsigned long> pids;
	std::vector<unsigned long> new_pids;

	Mutex demux_mutex;

protected:
	std::string webif_reauest(std::string request) throw(http_trap);
	bool already_exist(std::vector<unsigned long> &pidlist, int pid);
	void set_filter(std::vector<unsigned long> &new_pids) throw(trap);
	bool parse_webif_response(std::string& response, std::vector<unsigned long> &new_pids);

public:
	Demuxer(HttpHeader *header) throw(http_trap);
	virtual ~Demuxer() throw();
	void open() throw(http_trap);

	int get_fd() const throw();
	bool is_initialized() { return true; }

	void disconnect_webif_socket();
};
//----------------------------------------------------------------------

#endif /* DEMUX_H_ */
