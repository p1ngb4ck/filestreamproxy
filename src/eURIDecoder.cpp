/*
 * URIDecoder.cpp
 *
 *  Created on: 2013. 10. 21.
 *      Author: kos
 */

#include <stdio.h>
#include <string.h>

#include "eURIDecoder.h"
//-------------------------------------------------------------------------------

eURIDecoder::eURIDecoder()
{
}
//-------------------------------------------------------------------------------

eURIDecoder::~eURIDecoder()
{
}
//-------------------------------------------------------------------------------

unsigned char eURIDecoder::H2I(wchar_t aHexDigit)
{
	switch (aHexDigit) {
	case _UL_('0'):
	case _UL_('1'):
	case _UL_('2'):
	case _UL_('3'):
	case _UL_('4'):
	case _UL_('5'):
	case _UL_('6'):
	case _UL_('7'):
	case _UL_('8'):
	case _UL_('9'):
		return (unsigned char)(9 + aHexDigit - _UL_('9'));
	case _UL_('a'):
	case _UL_('b'):
	case _UL_('c'):
	case _UL_('d'):
	case _UL_('e'):
	case _UL_('f'):
		return (unsigned char)(15 + aHexDigit - _UL_('f'));
	case _UL_('A'):
	case _UL_('B'):
	case _UL_('C'):
	case _UL_('D'):
	case _UL_('E'):
	case _UL_('F'):
		return (unsigned char)(15 + aHexDigit - _UL_('F'));
	default:
		return 0;
	}
}
//-------------------------------------------------------------------------------

const wchar_t* eURIDecoder::DecodeURI(wchar_t* aData, int aBreakCond)
{
	wchar_t* read  = aData;
	wchar_t* write = aData;
	bool prevWasCr = false;

	if (aData == NULL) {
		return NULL;
	}

	for (;;) {
		switch (read[0]) {
		case _UL_('\0'):
			if (read > write) {
				write[0] = _UL_('\0');
			}
			return write;

		case _UL_('%'):
			switch (read[1]) {
			case _UL_('0'):
			case _UL_('1'):
			case _UL_('2'):
			case _UL_('3'):
			case _UL_('4'):
			case _UL_('5'):
			case _UL_('6'):
			case _UL_('7'):
			case _UL_('8'):
			case _UL_('9'):
			case _UL_('a'):
			case _UL_('b'):
			case _UL_('c'):
			case _UL_('d'):
			case _UL_('e'):
			case _UL_('f'):
			case _UL_('A'):
			case _UL_('B'):
			case _UL_('C'):
			case _UL_('D'):
			case _UL_('E'):
			case _UL_('F'):
				switch (read[2]) {
				case _UL_('0'):
				case _UL_('1'):
				case _UL_('2'):
				case _UL_('3'):
				case _UL_('4'):
				case _UL_('5'):
				case _UL_('6'):
				case _UL_('7'):
				case _UL_('8'):
				case _UL_('9'):
				case _UL_('a'):
				case _UL_('b'):
				case _UL_('c'):
				case _UL_('d'):
				case _UL_('e'):
				case _UL_('f'):
				case _UL_('A'):
				case _UL_('B'):
				case _UL_('C'):
				case _UL_('D'):
				case _UL_('E'):
				case _UL_('F'): {
						const unsigned char left = H2I(read[1]);
						const unsigned char right = H2I(read[2]);
						const int code = 16 * left + right;
						switch (code) {
						case 10:
							switch (aBreakCond) {
							case BR_TO_LF:
								if (!prevWasCr) {
									write[0] = (wchar_t)10;
									write++;
								}
								break;

							case BR_TO_CRLF:
								if (!prevWasCr) {
									write[0] = (wchar_t)13;
									write[1] = (wchar_t)10;
									write += 2;
								}
								break;

							case BR_TO_CR:
								if (!prevWasCr) {
									write[0] = (wchar_t)13;
									write++;
								}
								break;

							case BR_DONT_TOUCH:
							default:
								write[0] = (wchar_t)10;
								write++;

							}
							prevWasCr = false;
							break;

						case 13:
							switch (aBreakCond) {
							case BR_TO_LF:
								write[0] = (wchar_t)10;
								write++;
								break;

							case BR_TO_CRLF:
								write[0] = (wchar_t)13;
								write[1] = (wchar_t)10;
								write += 2;
								break;

							case BR_TO_CR:
								write[0] = (wchar_t)13;
								write++;
								break;

							case BR_DONT_TOUCH:
							default:
								write[0] = (wchar_t)13;
								write++;

							}
							prevWasCr = true;
							break;

						default:
							write[0] = (wchar_t)(code);
							write++;

							prevWasCr = false;

						}
						read += 3;
					}
					break;

				default:
					if (read > write) {
						write[0] = read[0];
						write[1] = read[1];
					}
					read += 2;
					write += 2;

					prevWasCr = false;
				}
				break;

			default:
				if (read > write) {
					write[0] = read[0];
				}
				read++;
				write++;

				prevWasCr = false;
			}
			break;

		case _UL_('+'):
			if (read > write) {
				write[0] = _UL_(' ');
			}
			read++;
			write++;

			prevWasCr = false;
			break;

		default:
			if (read > write) {
				write[0] = read[0];
			}
			read++;
			write++;

			prevWasCr = false;
		}
	}
	return NULL;
}
//-------------------------------------------------------------------------------

std::wstring eURIDecoder::Decode(const wchar_t* aInput)
{
	wchar_t working[1024] = {0};

	wcscpy(working, aInput);
	DecodeURI(working, BR_DONT_TOUCH);

	return std::wstring(working);
}
//-------------------------------------------------------------------------------

std::string eURIDecoder::Decode(const char* aInput)
{
	std::string tmp = aInput;
	std::wstring in = L"";
	in.assign(tmp.begin(), tmp.end());

	std::wstring decode = Decode(in.c_str());

	tmp.assign(decode.begin(), decode.end());

	return tmp;
}
//-------------------------------------------------------------------------------

#ifdef UNIT_TEST
#include <iostream>
int main()
{
	std::string in = "/home/kos/work/workspace/tsstreamproxy/test/20130528%201415%20-%20ZDF%20-%20Die%20K%C3%BCchenschlacht.ts";
	std::string out = URIDecoder().Decode(in.c_str());

	cout << out << endl;

	FILE* fp = fopen(out.c_str(), "rb");

	cout << (fp == NULL) ? "OPEN FAIL!!" : "OPEN OK" << endl;
}

#endif
