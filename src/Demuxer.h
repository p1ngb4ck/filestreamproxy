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

#include "trap.h"

#include "Utils.h"
#include "Source.h"
//----------------------------------------------------------------------

class Demuxer : public Source
{
public:
	int	pmt_pid;
	int	video_pid;
	int	audio_pid;

private:
	int fd;
	int sock;

	int demux_id;
	int pat_pid;
	std::vector<unsigned long> pids;

protected:
	std::string webif_reauest(std::string service, std::string auth) throw(trap);
	bool already_exist(std::vector<unsigned long> &pidlist, int pid);
	void set_filter(std::vector<unsigned long> &new_pids) throw(trap);
	bool parse_webif_response(std::string& response, std::vector<unsigned long> &new_pids);

public:
	Demuxer(RequestHeader *header) throw(trap);
	virtual ~Demuxer() throw();
	int get_fd() const throw();
};
//----------------------------------------------------------------------

#endif /* DEMUX_H_ */
