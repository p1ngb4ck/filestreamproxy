/*
 * eUpstreamPumpThread.h
 *
 *  Created on: 2013. 10. 30.
 *      Author: kos
 */

#ifndef EUPSTREAMSOCKET_H_
#define EUPSTREAMSOCKET_H_

#include <arpa/inet.h>
#include <netinet/ip.h>

#include <string>
//-------------------------------------------------------------------------------

class eUpstreamSocket
{
private:
	int mSockFd;
	struct sockaddr_in mSockAddr;

public:
	eUpstreamSocket();
	virtual ~eUpstreamSocket();

	bool Connect();
	int Send(std::string aData);
	int Recv(std::string& aData);

	int Request(std::string aSendData, std::string& aRecvData);
};
//-------------------------------------------------------------------------------

#endif /* EUPSTREAMSOCKET_H_ */
