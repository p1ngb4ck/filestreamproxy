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
//----------------------------------------------------------------------

void Mpeg::seek(HttpHeader &header)
{
#ifdef USE_EXTERNAL_SEEK_TIME
	try {
		std::string position = header.page_params["position"];
		std::string relative = header.page_params["relative"];

		if (position.empty() && relative.empty()) {
			off_t byte_offset = 0;
			std::string range = header.params["Range"];
			if((range.length() > 7) && (range.substr(0, 6) == "bytes=")) {
				range = range.substr(6);
				if(range.find('-') == (range.length() - 1)) {
					byte_offset = Util::strtollu(range);
				}
			}
			if (is_time_seekable && byte_offset > 0) {
				DEBUG("seek to byte_offset %llu", byte_offset);
				seek_absolute(byte_offset);
				DEBUG("seek ok");
			}
		}
		else {
			unsigned int position_offset;
			if (!relative.empty()) {
				int dur = duration();
				DEBUG("duration : %d", dur);
				position_offset = (dur * Util::strtollu(relative)) / 100;
			}
			else {
				position_offset = Util::strtollu(position);
			}

			if (is_time_seekable && position_offset > 0) {
				DEBUG("seek to position_offset %ds", position_offset);
				seek_time((position_offset * 1000) + first_pcr_ms);
				DEBUG("seek ok");
			}
		}
	}
	catch (const trap &e) {
		WARNING("Exception : %s", e.what());
	}
#else
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
			off_t position_offset;
			if (!relative.empty()) {
				int dur = duration();
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
#endif
}
//----------------------------------------------------------------------

off_t Mpeg::lseek(off_t offset, int whence)
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

ssize_t Mpeg::read(off_t offset, void *buf, size_t count)
{
	if (offset != m_current_offset) {
		m_current_offset = this->lseek(offset, SEEK_SET);
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
	off_t end = this->lseek(0, SEEK_END);

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
	pts_t dummy = duration();

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
	calc_begin(); calc_end();
	if (!(m_begin_valid && m_end_valid))
		return -1;

	pts_t len_in_pts = m_pts_end - m_pts_begin;

	/* wrap around? */
	if (len_in_pts < 0)
		len_in_pts += 0x200000000LL;
	off_t len_in_bytes = m_offset_end - m_offset_begin;

	if (!len_in_pts)
		return -1;

	unsigned long long bitrate = len_in_bytes * 90000 * 8 / len_in_pts;
	if ((bitrate < 10000) || (bitrate > 100000000))
		return -1;

	return bitrate;
}
//----------------------------------------------------------------------

int Mpeg::get_offset(off_t &offset, pts_t &pts, int marg)
{
	calc_begin(); calc_end();

	if (!m_begin_valid)
		return -1;
	if (!m_end_valid)
		return -1;

	if (!m_samples_taken)
		take_samples();

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
		if (this->read(offset, packet, 188) != 188) {
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

int Mpeg::duration()
{
	calc_begin(); calc_end();
	if (!(m_begin_valid && m_end_valid))
		return -1;
	pts_t len = m_pts_end - m_pts_begin;

	if (len < 0)
		len += 0x200000000LL;

	len = len / 90000;
	return int(len);
}
//----------------------------------------------------------------------
