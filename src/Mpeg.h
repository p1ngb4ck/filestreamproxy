/*
 * Mpeg.h
 *
 *  Created on: 2014. 6. 18.
 *      Author: oskwon
 */

#ifndef MPEG_H_
#define MPEG_H_

#include "trap.h"
#include "mpegts.h"
//----------------------------------------------------------------------

class HttpHeader;

typedef long long pts_t;

class Mpeg : public MpegTS
{
private:
	off_t m_splitsize, m_totallength, m_current_offset, m_base_offset, m_last_offset;
	int m_nrfiles, m_current_file;

	pts_t m_pts_begin, m_pts_end;

	off_t m_offset_begin, m_offset_end;
	off_t m_last_filelength;

	int m_begin_valid, m_end_valid, m_futile;

	std::map<pts_t, off_t> m_samples;
	int m_samples_taken;

	void scan();
	int switch_offset(off_t off);

	void calc_end();
	void calc_begin();

	int fix_pts(const off_t &offset, pts_t &now);
	int get_pts(off_t &offset, pts_t &pts, int fixed);

	int duration();
	off_t lseek(off_t offset, int whence);
	ssize_t read(off_t offset, void *buf, size_t count);

	int get_offset(off_t &offset, pts_t &pts, int marg);
	int take_sample(off_t off, pts_t &p);
	void take_samples();

	int calc_bitrate();
public:
	Mpeg(std::string file, bool request_time_seek) throw (trap)
		: MpegTS(file, request_time_seek)
	{
		m_current_offset = m_base_offset = m_last_offset = 0;
		m_splitsize = m_nrfiles = m_current_file = m_totallength = 0;

		m_pts_begin = m_pts_end = m_offset_begin = m_offset_end = 0;
		m_last_filelength = m_begin_valid = m_end_valid = m_futile =0;

		m_samples_taken = 0;
	}

	virtual ~Mpeg() throw () {}

	void seek(HttpHeader &header);
};
//----------------------------------------------------------------------

#endif /* MPEG_H_ */
