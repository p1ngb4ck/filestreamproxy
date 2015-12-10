/*
 * Mpeg.h
 *
 *  Created on: 2014. 6. 18.
 *      Author: oskwon
 */

#ifndef MPEG_H_
#define MPEG_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <map>
#include <string>

#include "Source.h"
#include "Logger.h"
#include "3rdparty/trap.h"
//----------------------------------------------------------------------

class HttpHeader;

typedef long long pts_t;

class Mpeg : public Source
{
private:
	off_t m_splitsize, m_totallength, m_current_offset, m_base_offset, m_last_offset;
	int m_nrfiles, m_current_file;

	pts_t m_pts_begin, m_pts_end;

	off_t m_offset_begin, m_offset_end;
	off_t m_last_filelength;

	int m_begin_valid, m_end_valid, m_futile;

	int m_samples_taken;
	std::map<pts_t, off_t> m_samples;

	int m_duration;

	int fd;
	bool m_is_initialized;

	void scan();
	int switch_offset(off_t off);

	void calc_end();
	void calc_begin();
	int  calc_length();
	int  calc_bitrate();

	int fix_pts(const off_t &offset, pts_t &now);
	int get_pts(off_t &offset, pts_t &pts, int fixed);
	int get_offset(off_t &offset, pts_t &pts, int marg);

	void take_samples();
	int  take_sample(off_t off, pts_t &p);

	ssize_t read_internal(off_t offset, void *buf, size_t count);

	off_t seek_absolute(off_t offset);
	off_t seek_internal(off_t offset, int whence);

	bool read_ts_meta(std::string media_file_name, int &vpid, int &apid);

	int find_pmt();

public:
	off_t stream_length;

	Mpeg(std::string filename, bool request_time_seek) throw (trap);
	virtual ~Mpeg() throw () {}

	void seek(HttpHeader &header);
	bool is_initialized() { return m_is_initialized; }
	int get_fd() const	throw() { return fd; }
};
//----------------------------------------------------------------------

#endif /* MPEG_H_ */
