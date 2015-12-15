/*
 * Source.h
 *
 *  Created on: 2014. 6. 12.
 *      Author: oskwon
 */

#ifndef SOURCE_H_
#define SOURCE_H_

#include "3rdparty/trap.h"
//----------------------------------------------------------------------

class Source
{
public:
	enum source_type_t {
		SOURCE_TYPE_NONE=0,
		SOURCE_TYPE_FILE,
		SOURCE_TYPE_LIVE,
		SOURCE_TYPE_MAX
	};

	Source(){}
	virtual ~Source(){}
	virtual int get_fd() const throw() = 0;
	virtual bool is_initialized() = 0;

	Source::source_type_t get_source_type() { return source_type; }
	
public:
	int pmt_pid;
	int video_pid;
	int audio_pid;

	Source::source_type_t source_type;
};
//----------------------------------------------------------------------

#endif /* SOURCE_H_ */
