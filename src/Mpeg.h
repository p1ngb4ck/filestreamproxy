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

class Mpeg : public MpegTS
{
public:
	Mpeg(std::string file, bool request_time_seek) throw (trap)
		: MpegTS(file, request_time_seek)
	{}
	virtual ~Mpeg() throw () {}

	void seek(HttpHeader &header);
};
//----------------------------------------------------------------------

#endif /* MPEG_H_ */
