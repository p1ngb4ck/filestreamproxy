/*
 * eTransCodingDevice.cpp
 *
 *  Created on: 2013. 11. 3.
 *      Author: kos
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ePreDefine.h"
#include "eTransCodingDevice.h"
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

eTransCodingDevice::eTransCodingDevice()
	: mDeviceFd(0)
{
}
//-------------------------------------------------------------------------------

eTransCodingDevice::~eTransCodingDevice()
{
	Close();
}
//-------------------------------------------------------------------------------

bool eTransCodingDevice::Open()
{
	mDeviceFd = open("/dev/bcm_enc0", O_RDWR);
	if(mDeviceFd <= 0) {
		mDeviceFd = 0;
#ifdef DEBUG_LOG
		LOG("transcoding device open failed.");
#endif
		return false;
	}
#ifdef DEBUG_LOG
	LOG("transcoding device open ok.");
#endif
	return true;
}
//-------------------------------------------------------------------------------

void eTransCodingDevice::Close()
{
	StopTranscoding();
	if(mDeviceFd == 0) {
		close(mDeviceFd);
	}
	mDeviceFd = 0;
}
//-------------------------------------------------------------------------------

int eTransCodingDevice::GetDeviceFd()
{
	return mDeviceFd;
}
//-------------------------------------------------------------------------------

bool eTransCodingDevice::SetStreamPid(int aVideoPid, int aAudioPid)
{
	if(ioctl(mDeviceFd, 1, aVideoPid) < 0) {
#ifdef DEBUG_LOG
	LOG("setting stream video pid failed.");
#endif
		return false;
	}
	if(ioctl(mDeviceFd, 2, aAudioPid) < 0) {
#ifdef DEBUG_LOG
	LOG("setting stream audio pid failed.");
#endif
		return false;
	}
#ifdef DEBUG_LOG
	LOG("setting stream pid ok.");
#endif
	return true;
}
//-------------------------------------------------------------------------------

bool eTransCodingDevice::StartTranscoding()
{
	if(ioctl(mDeviceFd, 100, 0) < 0) {
#ifdef DEBUG_LOG
	LOG("start transcoding failed.");
#endif
		return false;
	}
#ifdef DEBUG_LOG
	LOG("start transcoding ok.");
#endif
	return true;
}
//-------------------------------------------------------------------------------

void eTransCodingDevice::StopTranscoding()
{
	ioctl(mDeviceFd, 200, 0);
}
//-------------------------------------------------------------------------------
