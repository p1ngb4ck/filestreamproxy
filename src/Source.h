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
	Source(){}
	virtual ~Source(){}
	virtual int get_fd() const throw() = 0;
	virtual bool is_initialized() = 0;
};
//----------------------------------------------------------------------

#endif /* SOURCE_H_ */
