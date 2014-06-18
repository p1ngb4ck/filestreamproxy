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
	try {
		std::string position = header.page_params["position"];
		if (position == "") {
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
			unsigned int position_offset = Util::strtollu(position);
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
}
//----------------------------------------------------------------------
