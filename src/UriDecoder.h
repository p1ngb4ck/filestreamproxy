/*
 * UriDecoder.h
 *
 *  Created on: 2014. 6. 10.
 *      Author: oskwon
 */

#ifndef URIDECODER_H_
#define URIDECODER_H_

#include <memory>
#include <string>

#include <wchar.h>
//-------------------------------------------------------------------------------

#define BR_TO_LF	0
#define BR_TO_CRLF	1
#define BR_TO_CR	2
#define BR_TO_UNIX	BR_TO_LF
#define BR_TO_WINDOWS	BR_TO_CRLF
#define BR_TO_MAC	BR_TO_CR
#define BR_DONT_TOUCH	6

#define _UL_(x) L##x
//-------------------------------------------------------------------------------

class UriDecoder
{
protected:
	unsigned char h2i(wchar_t aHexDigit);
	const wchar_t* decode_uri(wchar_t* aData, int aBreakCond);

public:
	UriDecoder(){};
	virtual ~UriDecoder(){};

	std::string decode(const char* aInput);
	std::wstring decode64(const wchar_t* aInput);
};
//-------------------------------------------------------------------------------

#endif /* URIDECODER_H_ */
