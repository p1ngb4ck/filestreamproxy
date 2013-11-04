/*
 * eTransCodingDevice.h
 *
 *  Created on: 2013. 11. 3.
 *      Author: kos
 */

#ifndef ETRANSCODINGDEVICE_H_
#define ETRANSCODINGDEVICE_H_

class eTransCodingDevice
{
private:
	int mDeviceFd;
public:
	eTransCodingDevice();
	~eTransCodingDevice();

	bool Open();
	void Close();

	int GetDeviceFd();

	bool SetStreamPid(int aVideoPid, int aAudioPid);
	bool SetStreamPid(int aVideoPid, int aAudioPid, int aPmtPid);

	bool StartTranscoding();
	void StopTranscoding();
};
//-------------------------------------------------------------------------------

#endif /* ETRANSCODINGDEVICE_H_ */
