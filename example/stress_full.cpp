/*
 * stress.cpp
 *
 *  Created on: 2014. 10. 17.
 *      Author: oskwon
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define MAX_INTERVAL_LENGTH (32)
#define DD_LOG(X,...) { printf(X" (%s:%d)\n", ##__VA_ARGS__, __FUNCTION__, __LINE__); }
//-------------------------------------------------------------------------------

#define PORT 8002
#define IP "192.168.100.240"

int intervals[MAX_INTERVAL_LENGTH+1] = {
		2,4,6,1,4,2,15,9,
		2,1,2,2,2,2,15,3,
		2,2,2,2,2,2,2,2,
		2,2,2,2,2,2,2,2,
		0
};
const char* services[] = {
		"1:0:19:2B66:3F3:1:C00000:0:0:0:",
		"1:0:19:2B7A:3F3:1:C00000:0:0:0:",
		""
};

pid_t child_pid = 0;
//-------------------------------------------------------------------------------

void child_sigint_handler( int signo)
{
	exit(0);
}
//-------------------------------------------------------------------------------

void parent_sigint_handler( int signo)
{
	kill(child_pid, SIGINT);
	exit(0);
}
//-------------------------------------------------------------------------------

void command_execute(char* command)
{
	DD_LOG("excute... [%s]", command);
	system(command);
}
//-------------------------------------------------------------------------------

void child_main(const char* ip, int port, const char* service)
{
	char command[2048];

	signal(SIGUSR2, child_sigint_handler);

	sprintf(command, "curl http://%s/web/zap?sRef=%s", ip, service);
	command_execute(command);

	sprintf(command, "curl http://%s:%d/%s > /dev/null", ip, port, service);
	command_execute(command);
}
//-------------------------------------------------------------------------------

int main(int argc, char** argv)
{
	signal(SIGINT, parent_sigint_handler);

	for (int idx = 0; idx < 500; idx++) {
		DD_LOG("\e[1;32m==========================================>> try : %d\e[00m", idx);

		child_pid = fork();
		if (child_pid < 0) {
			DD_LOG("fork fail");
			return 0;
		}

		if (child_pid == 0) {
			if (argc == 1) {
				child_main(IP, PORT, services[idx%2]);
			}
			else {
				child_main(argv[1], PORT, services[idx%2]);
			}
			return 0;
		}

		usleep(intervals[idx % MAX_INTERVAL_LENGTH] * 1000*1000);
		kill(child_pid, SIGINT);

		if (idx == 499) {
			idx = 0;
		}
	}
	return 0;
}
//-------------------------------------------------------------------------------

