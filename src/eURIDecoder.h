/*
 * URIDecoder.h
 *
 *  Created on: 2013. 10. 21.
 *      Author: kos
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

class eURIDecoder
{
protected:
	unsigned char H2I(wchar_t aHexDigit);
	const wchar_t* DecodeURI(wchar_t* aData, int aBreakCond);

public:
	eURIDecoder();
	virtual ~eURIDecoder();

	std::string Decode(const char* aInput);
	std::wstring Decode(const wchar_t* aInput);
};
//-------------------------------------------------------------------------------

#endif /* URIDECODER_H_ */
