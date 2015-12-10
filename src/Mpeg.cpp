/*
 * Mpeg.cpp
 *
 *  Created on: 2014. 6. 18.
 *      Author: oskwon
 */

#include "Mpeg.h"
#include "Http.h"
#include "Util.h"
#include "Logger.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <vector>
#include <string>
#include <fstream>
#include <iterator>

using namespace std;
//----------------------------------------------------------------------

void Mpeg::seek(HttpHeader &header)
{
	try {
		off_t byte_offset = 0;
		std::string position = header.page_params["position"];
		std::string relative = header.page_params["relative"];
		if (position.empty() && relative.empty()) {
			std::string range = header.params["Range"];
			DEBUG("Range : %s", range.c_str());
			if((range.length() > 7) && (range.substr(0, 6) == "bytes=")) {
				range = range.substr(6);
				if(range.find('-') == (range.length() - 1)) {
					byte_offset = Util::strtollu(range);
					DEBUG("Range To : %s -> %llu", range.c_str(), byte_offset);
				}
			}
		}
		else {
			pts_t position_offset;
			if (!relative.empty()) {
				int dur = calc_length();
				DEBUG("duration : %d", dur);
				position_offset = (dur * Util::strtollu(relative)) / 100;
			}
			else {
				position_offset = Util::strtollu(position);
			}
			position_offset *= 90000;
			get_offset(byte_offset, position_offset, -1);
		}

		DEBUG("seek to byte_offset %llu", byte_offset);
		if (byte_offset > 0) {
			seek_absolute(byte_offset);
			DEBUG("seek ok");
		}
	}
	catch (...) {
		WARNING("seek fail.");
	}
}
//----------------------------------------------------------------------

off_t Mpeg::seek_internal(off_t offset, int whence)
{
	if (m_nrfiles < 2)
		return ::lseek(get_fd(), offset, whence);

	switch (whence) {
	case SEEK_SET: m_current_offset  = offset; break;
	case SEEK_CUR: m_current_offset += offset; break;
	case SEEK_END: m_current_offset  = m_totallength + offset; break;
	}

	if (m_current_offset < 0)
		m_current_offset = 0;
	return m_current_offset;
}
//----------------------------------------------------------------------

ssize_t Mpeg::read_internal(off_t offset, void *buf, size_t count)
{
	if (offset != m_current_offset) {
		m_current_offset = seek_internal(offset, SEEK_SET);
		if (m_current_offset < 0) {
			return m_current_offset;
		}
	}
	switch_offset(m_current_offset);

	if (m_nrfiles >= 2) {
		if ((m_current_offset + count) > m_totallength)
			count = m_totallength - m_current_offset;
		if (count < 0) {
			return 0;
		}
	}

	int ret = ::read(get_fd(), buf, count);
	if (ret > 0)
		m_current_offset = m_last_offset += ret;
	return ret;
}
//----------------------------------------------------------------------

int Mpeg::switch_offset(off_t off)
{
	if (m_splitsize) {
		int filenr = off / m_splitsize;
		if (filenr >= m_nrfiles)
			filenr = m_nrfiles - 1;
#if 0
		if (filenr != m_current_file) {
			close();
			m_fd = open(filenr);
			m_last_offset = m_base_offset = m_splitsize * filenr;
			m_current_file = filenr;
		}
#endif
	}
	else m_base_offset = 0;

	return (off != m_last_offset) ? (m_last_offset = ::lseek(get_fd(), off - m_base_offset, SEEK_SET) + m_base_offset) : m_last_offset;
}
//----------------------------------------------------------------------

void Mpeg::calc_begin()
{
	if (!(m_begin_valid || m_futile)) {
		m_offset_begin = 0;
		if (!get_pts(m_offset_begin, m_pts_begin, 0))
			m_begin_valid = 1;
		else m_futile = 1;
	}
	if (m_begin_valid) {
		m_end_valid = 0;
	}
}
//----------------------------------------------------------------------

void Mpeg::calc_end()
{
	off_t end = seek_internal(0, SEEK_END);

	if (llabs(end - m_last_filelength) > 1*1024*1024) {
		m_last_filelength = end;
		m_end_valid = 0;

		m_futile = 0;
	}

	int maxiter = 10;

	m_offset_end = m_last_filelength;

	while (!(m_end_valid || m_futile)) {
		if (!--maxiter) {
			m_futile = 1;
			return;
		}

		m_offset_end -= 256*1024;
		if (m_offset_end < 0)
			m_offset_end = 0;

		off_t off = m_offset_end;

		if (!get_pts(m_offset_end, m_pts_end, 0))
			m_end_valid = 1;
		else m_offset_end = off;

		if (!m_offset_end) {
			m_futile = 1;
			break;
		}
	}
}
//----------------------------------------------------------------------

int Mpeg::fix_pts(const off_t &offset, pts_t &now)
{
	/* for the simple case, we assume one epoch, with up to one wrap around in the middle. */
	calc_begin();
	if (!m_begin_valid) {
		return -1;
	}

	pts_t pos = m_pts_begin;
	if ((now < pos) && ((pos - now) < 90000 * 10)) {
		pos = 0;
		return 0;
	}

	if (now < pos) /* wrap around */
		now = now + 0x200000000LL - pos;
	else now -= pos;

	return 0;
}
//----------------------------------------------------------------------

void Mpeg::take_samples()
{
	m_samples_taken = 1;
	m_samples.clear();
	int retries=2;
	pts_t dummy = calc_length();

	if (dummy <= 0)
		return;

	int nr_samples = 30;
	off_t bytes_per_sample = (m_offset_end - m_offset_begin) / (long long)nr_samples;
	if (bytes_per_sample < 40*1024*1024)
		bytes_per_sample = 40*1024*1024;

	bytes_per_sample -= bytes_per_sample % 188;

	DEBUG("samples step %lld, pts begin %llx, pts end %llx, offs begin %lld, offs end %lld:",
			bytes_per_sample, m_pts_begin, m_pts_end, m_offset_begin, m_offset_end);

	for (off_t offset = m_offset_begin; offset < m_offset_end;) {
		pts_t p;
		if (take_sample(offset, p) && retries--)
			continue;
		retries = 2;
		offset += bytes_per_sample;
	}
	m_samples[0] = m_offset_begin;
	m_samples[m_pts_end - m_pts_begin] = m_offset_end;
}
//----------------------------------------------------------------------

/* returns 0 when a sample was taken. */
int Mpeg::take_sample(off_t off, pts_t &p)
{
	off_t offset_org = off;

	if (!get_pts(off, p, 1)) {
		/* as we are happily mixing PTS and PCR values (no comment, please), we might
		   end up with some "negative" segments.
		   so check if this new sample is between the previous and the next field*/
		std::map<pts_t, off_t>::const_iterator l = m_samples.lower_bound(p);
		std::map<pts_t, off_t>::const_iterator u = l;

		if (l != m_samples.begin()) {
			--l;
			if (u != m_samples.end()) {
				if ((l->second > off) || (u->second < off)) {
					DEBUG("ignoring sample %lld %lld %lld (%llx %llx %llx)", l->second, off, u->second, l->first, p, u->first);
					return 1;
				}
			}
		}

		DEBUG("adding sample %lld: pts 0x%llx -> pos %lld (diff %lld bytes)", offset_org, p, off, off-offset_org);
		m_samples[p] = off;
		return 0;
	}
	return -1;
}
//----------------------------------------------------------------------

int Mpeg::calc_bitrate()
{
	calc_length();
	if (!m_begin_valid || !m_end_valid) {
		return -1;
	}

	pts_t len_in_pts = m_pts_end - m_pts_begin;

	/* wrap around? */
	if (len_in_pts < 0) {
		len_in_pts += 0x200000000LL;
	}
	off_t len_in_bytes = m_offset_end - m_offset_begin;
	if (!len_in_pts) return -1;

	unsigned long long bitrate = len_in_bytes * 90000 * 8 / len_in_pts;
	if ((bitrate < 10000) || (bitrate > 100000000)) {
		return -1;
	}
	return bitrate;
}
//----------------------------------------------------------------------

int Mpeg::find_pmt()
{
	off_t position=0;

	int left = 5*1024*1024;

	while (left >= 188) {
		unsigned char packet[188];
		int ret = read_internal(position, packet, 188);
		if (ret != 188) {
			DEBUG("read error");
			break;
		}
		left -= 188;
		position += 188;

		if (packet[0] != 0x47) {
			int i = 0;
			while (i < 188) {
				if (packet[i] == 0x47)
					break;
				--position; ++i;
			}
			continue;
		}
		int pid = ((packet[1] << 8) | packet[2]) & 0x1FFF;
		int pusi = !!(packet[1] & 0x40);

		if (!pusi) continue;

		/* ok, now we have a PES header or section header*/
		unsigned char *sec;

		/* check for adaption field */
		if (packet[3] & 0x20) {
			if (packet[4] >= 183)
				continue;
			sec = packet + packet[4] + 4 + 1;
		}
		else {
			sec = packet + 4;
		}
		if (sec[0])	continue; /* table pointer, assumed to be 0 */
		if (sec[1] == 0x02) { /* program map section */
			pmt_pid = pid;
			return 0;
		}
	}
	return -1;
}
//----------------------------------------------------------------------

int Mpeg::get_offset(off_t &offset, pts_t &pts, int marg)
{
	calc_length();
	if (!m_begin_valid || !m_end_valid) {
		return -1;
	}

	if (!m_samples_taken) {
		take_samples();
	}

	if (!m_samples.empty()) {
		int maxtries = 5;
		pts_t p = -1;

		while (maxtries--) {
			/* search entry before and after */
			std::map<pts_t, off_t>::const_iterator l = m_samples.lower_bound(pts);
			std::map<pts_t, off_t>::const_iterator u = l;

			if (l != m_samples.begin())
				--l;

			/* we could have seeked beyond the end */
			if (u == m_samples.end()) {
				/* use last segment for interpolation. */
				if (l != m_samples.begin()) {
					--u;
					--l;
				}
			}

			/* if we don't have enough points */
			if (u == m_samples.end())
				break;

			pts_t pts_diff = u->first - l->first;
			off_t offset_diff = u->second - l->second;

			if (offset_diff < 0) {
				DEBUG("something went wrong when taking samples.");
				m_samples.clear();
				take_samples();
				continue;
			}
			DEBUG("using: %llx:%llx -> %llx:%llx", l->first, u->first, l->second, u->second);

			int bitrate = (pts_diff) ? (offset_diff * 90000 * 8 / pts_diff) : 0;
			offset = l->second;
			offset += ((pts - l->first) * (pts_t)bitrate) / 8ULL / 90000ULL;
			offset -= offset % 188;
			p = pts;

			if (!take_sample(offset, p)) {
				int diff = (p - pts) / 90;
				DEBUG("calculated diff %d ms", diff);

				if (::abs(diff) > 300) {
					DEBUG("diff to big, refining");
					continue;
				}
			}
			else DEBUG("no sample taken, refinement not possible.");
			break;
		}

		if (p != -1) {
			pts = p;
			DEBUG("aborting. Taking %llx as offset for %lld", offset, pts);
			return 0;
		}
	}

	int bitrate = calc_bitrate();
	offset = pts * (pts_t)bitrate / 8ULL / 90000ULL;
	DEBUG("fallback, bitrate=%d, results in %016llx", bitrate, offset);
	offset -= offset % 188;

	return 0;
}
//----------------------------------------------------------------------

int Mpeg::get_pts(off_t &offset, pts_t &pts, int fixed)
{
	int left = 256*1024;

	offset -= offset % 188;

	while (left >= 188) {
		unsigned char packet[188];
		if (read_internal(offset, packet, 188) != 188) {
			//break;
			return -1;
		}
		left   -= 188;
		offset += 188;

		if (packet[0] != 0x47) {
			int i = 0;
			while (i < 188) {
				if (packet[i] == 0x47)
					break;
				--offset; ++i;
			}
			continue;
		}

		unsigned char *payload;
		int pusi = !!(packet[1] & 0x40);

		/* check for adaption field */
		if (packet[3] & 0x20) {
			if (packet[4] >= 183)
				continue;
			if (packet[4]) {
				if (packet[5] & 0x10) { /* PCR present */
					pts  = ((unsigned long long)(packet[ 6]&0xFF)) << 25;
					pts |= ((unsigned long long)(packet[ 7]&0xFF)) << 17;
					pts |= ((unsigned long long)(packet[ 8]&0xFE)) << 9;
					pts |= ((unsigned long long)(packet[ 9]&0xFF)) << 1;
					pts |= ((unsigned long long)(packet[10]&0x80)) >> 7;
					offset -= 188;
					if (fixed && fix_pts(offset, pts))
						return -1;
					return 0;
				}
			}
			payload = packet + packet[4] + 4 + 1;
		}
		else payload = packet + 4;

		if (!pusi) continue;

		/* somehow not a startcode. (this is invalid, since pusi was set.) ignore it. */
		if (payload[0] || payload[1] || (payload[2] != 1))
			continue;

		if (payload[3] == 0xFD) { // stream use extension mechanism defined in ISO 13818-1 Amendment 2
			if (payload[7] & 1) { // PES extension flag
				int offs = 0;
				if (payload[7] & 0x80) // pts avail
					offs += 5;
				if (payload[7] & 0x40) // dts avail
					offs += 5;
				if (payload[7] & 0x20) // escr avail
					offs += 6;
				if (payload[7] & 0x10) // es rate
					offs += 3;
				if (payload[7] & 0x8)  // dsm trickmode
					offs += 1;
				if (payload[7] & 0x4)  // additional copy info
					offs += 1;
				if (payload[7] & 0x2)  // crc
					offs += 2;
				if (payload[8] < offs)
					continue;
				uint8_t pef = payload[9+offs++]; // pes extension field
				if (pef & 1) { // pes extension flag 2
					if (pef & 0x80) // private data flag
						offs += 16;
					if (pef & 0x40) // pack header field flag
						offs += 1;
					if (pef & 0x20) // program packet sequence counter flag
						offs += 2;
					if (pef & 0x10) // P-STD buffer flag
						offs += 2;
					if (payload[8] < offs)
						continue;
					uint8_t stream_id_extension_len = payload[9+offs++] & 0x7F;
					if (stream_id_extension_len >= 1) {
						if (payload[8] < (offs + stream_id_extension_len) )
							continue;
						if (payload[9+offs] & 0x80) // stream_id_extension_bit (should not set)
							continue;
						switch (payload[9+offs]) {
						case 0x55 ... 0x5f: // VC-1
							break;
						case 0x71: // AC3 / DTS
							break;
						case 0x72: // DTS - HD
							break;
						default:
							continue;
						}
					}
					else continue;
				}
				else continue;
			}
			else continue;
		}
		/* drop non-audio, non-video packets because other streams can be non-compliant.*/
		else if (((payload[3] & 0xE0) != 0xC0) &&  // audio
			     ((payload[3] & 0xF0) != 0xE0)) {  // video
			continue;
		}

		if (payload[7] & 0x80) { /* PTS */
			pts  = ((unsigned long long)(payload[ 9]&0xE))  << 29;
			pts |= ((unsigned long long)(payload[10]&0xFF)) << 22;
			pts |= ((unsigned long long)(payload[11]&0xFE)) << 14;
			pts |= ((unsigned long long)(payload[12]&0xFF)) << 7;
			pts |= ((unsigned long long)(payload[13]&0xFE)) >> 1;
			offset -= 188;

			return (fixed && fix_pts(offset, pts)) ? -1 : 0;
		}
	}
	return -1;
}
//----------------------------------------------------------------------

int Mpeg::calc_length()
{
	if (m_duration <= 0) {
		calc_begin(); calc_end();
		if (!(m_begin_valid && m_end_valid))
			return -1;
		pts_t len = m_pts_end - m_pts_begin;

		if (len < 0)
			len += 0x200000000LL;

		len = len / 90000;

		m_duration = int(len);
	}
	return m_duration;
}
//----------------------------------------------------------------------

namespace eCacheID {
	enum {
		cVPID = 0,
		cAPID,
		cTPID,
		cPCRPID,
		cAC3PID,
		cVTYPE,
		cACHANNEL,
		cAC3DELAY,
		cPCMDELAY,
		cSUBTITLE,
		cacheMax
	};
};

/* f:40,c:00007b,c:01008f,c:03007b */
bool Mpeg::read_ts_meta(std::string media_file_name, int &vpid, int &apid)
{
	std::string metafilename = media_file_name;
	metafilename += ".meta";

	std::ifstream ifs(metafilename.c_str());

	if (!ifs.is_open()) {
		DEBUG("metadata is not exists..");
		return false;
	}

	size_t rc = 0, i = 0;
	char buffer[1024] = {0};
	while (!ifs.eof()) {
		ifs.getline(buffer, 1024);
		if (i++ == 7) {
			DEBUG("%d [%s]", i, buffer);
			std::vector<string> tokens;
			Util::split(buffer, ',', tokens);
			if(tokens.size() < 3) {
				DEBUG("pid count size error : %d", tokens.size());
				return false;
			}

			int setting_done = false;
			for (int ii = 0; ii < tokens.size(); ++ii) {
				std::string token = tokens[ii];
				if(token.length() <= 0) continue;
				if(token.at(0) != 'c') continue;

				int cache_id = atoi(token.substr(2,2).c_str());
				DEBUG("token : %d [%s], chcke_id : [%d]", ii, token.c_str(), cache_id);
				switch(cache_id) {
					case(eCacheID::cVPID):
						vpid = strtol(token.substr(4,4).c_str(), NULL, 16);
						DEBUG("video pid : %d", vpid);
						setting_done = (vpid != -1 && apid != -1) ? true : false;
						break;
					case(eCacheID::cAC3PID):
						apid = strtol(token.substr(4,4).c_str(), NULL, 16);
						DEBUG("audio pid : %d", apid);
						break;
					case(eCacheID::cAPID):
						apid = strtol(token.substr(4,4).c_str(), NULL, 16);
						DEBUG("audio pid : %d", apid);
						setting_done = (vpid != -1 && apid != -1) ? true : false;
						break;
				}
				if(setting_done) break;
			}
			break;
		}
	}
	ifs.close();
	return true;
}
//-------------------------------------------------------------------------------

Mpeg::Mpeg(std::string filename, bool request_time_seek) throw (trap)
//	: MpegTS(filename)
{
	source_type = Source::SOURCE_TYPE_FILE;

	m_current_offset = m_base_offset = m_last_offset = 0;
	m_splitsize = m_nrfiles = m_current_file = m_totallength = 0;

	m_pts_begin = m_pts_end = m_offset_begin = m_offset_end = 0;
	m_last_filelength = m_begin_valid = m_end_valid = m_futile =0;

	m_duration = m_samples_taken = 0;
	m_is_initialized = false;

	pmt_pid = video_pid = audio_pid = -1;

	fd = ::open(filename.c_str(), O_RDONLY | O_LARGEFILE, 0);
	if (fd < 0) {
		throw(trap("cannot open file"));
	}

	struct stat filestat;
	if (fstat(fd, &filestat)) {
		throw(trap("MpegTS::init: cannot stat"));
	}
	INFO("file length: %lld Mb", filestat.st_size  >> 20);

	stream_length = filestat.st_size;

	int apid = -1, vpid = -1;
	if (read_ts_meta(filename, vpid, apid)) {
		if (vpid != -1 && apid != -1) {
			video_pid = vpid;
			audio_pid = apid;
			m_is_initialized = true;

			/*find_pmt();*/
			return;
		}
	}
}
//----------------------------------------------------------------------

off_t Mpeg::seek_absolute(off_t offset2)
{
	off_t result = seek_internal(offset2, SEEK_SET);
	return result;
}
//----------------------------------------------------------------------
