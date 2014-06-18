/*
 * main.cpp
 *
 *  Created on: 2014. 6. 10.
 *      Author: oskwon
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>

#include <string>

#include "Util.h"
#include "Logger.h"

#include "Http.h"
#include "Mpeg.h"

#include "Demuxer.h"
#include "Encoder.h"
#include "UriDecoder.h"

using namespace std;
//----------------------------------------------------------------------

#define BUFFFER_SIZE (188 * 256)

void show_help();

void signal_handler(int sig_no);
void do_exit(const char *message);

void *source_thread_main(void *params);
void *streaming_thread_main(void *params);

int streaming_write(const char *buffer, size_t buffer_len, bool enable_log = false);
//----------------------------------------------------------------------

static bool is_terminated = true;
//----------------------------------------------------------------------

int main(int argc, char **argv)
{
	if (argc > 1) {
		if (strcmp(argv[1], "-h") == 0)
			show_help();
		exit(0);
	}
	Logger::instance()->init("/tmp/transtreamproxy", Logger::WARNING);

	signal(SIGINT, signal_handler);

	HttpHeader header;

	int source_thread_id, stream_thread_id;
	pthread_t source_thread_handle, stream_thread_handle;

	std::string req = HttpHeader::read_request();

	DEBUG("request head :\n%s", req.c_str());
	if (header.parse_request(req)) {
		Encoder encoder;
		Source *source = 0;
		ThreadParams thread_params = { 0, &encoder, &header };

		int video_pid = 0, audio_pid = 0, pmt_pid = 0;

		switch(header.type) {
		case HttpHeader::TRANSCODING_FILE:
			try {
				std::string uri = UriDecoder().decode(header.page_params["file"].c_str());
				Mpeg *ts = new Mpeg(uri, true);
				pmt_pid   = ts->pmt_pid;
				video_pid = ts->video_pid;
				audio_pid = ts->audio_pid;
				source = ts;
			}
			catch (const trap &e) {
				ERROR("fail to create source : %s", e.what());
				exit(-1);
			}
			break;
		case HttpHeader::TRANSCODING_LIVE:
			try {
				Demuxer *dmx = new Demuxer(&header);
				pmt_pid   = dmx->pmt_pid;
				video_pid = dmx->video_pid;
				audio_pid = dmx->audio_pid;
				source = dmx;
			}
			catch (const trap &e) {
				ERROR("fail to create source : %s", e.what());
				exit(-1);
			}
			break;
		case HttpHeader::M3U:
			try {
				std::string response = header.build_response((Mpeg*) source);
				if (response != "") {
					streaming_write(response.c_str(), response.length(), true);
				}
			}
			catch (...) {
			}
			exit(0);
		default:
			ERROR("not support source type (type : %d)", header.type);
			exit(-1);
		}
		thread_params.source = source;

		if (!encoder.retry_open(2, 3)) {
			exit(-1);
		}

		if (encoder.state == Encoder::ENCODER_STAT_OPENED) {
			std::string response = header.build_response((Mpeg*) source);
			if (response == "") {
				do_exit(0);
				return 0;
			}

			streaming_write(response.c_str(), response.length(), true);

			if (header.type == HttpHeader::TRANSCODING_FILE) {
				((Mpeg*) source)->seek(header);
			}

			if (!encoder.ioctl(Encoder::IOCTL_SET_VPID, video_pid)) {
				do_exit("fail to set video pid.");
				exit(-1);
			}
			if (!encoder.ioctl(Encoder::IOCTL_SET_APID, audio_pid)) {
				do_exit("fail to set audio pid.");
				exit(-1);
			}
			if (!encoder.ioctl(Encoder::IOCTL_SET_PMTPID, pmt_pid)) {
				do_exit("fail to set pmtid.");
				exit(-1);
			}
		}

		is_terminated = false;
		source_thread_id = pthread_create(&source_thread_handle, 0, source_thread_main, (void *)&thread_params);
		if (source_thread_id < 0) {
			do_exit("fail to create source thread.");
		}
		else {
			pthread_detach(source_thread_handle);
			sleep(1);
			if (!encoder.ioctl(Encoder::IOCTL_START_TRANSCODING, 0)) {
				do_exit("fail to start transcoding.");
			}
			else {
				stream_thread_id = pthread_create(&stream_thread_handle, 0, streaming_thread_main, (void *)&thread_params);
				if (stream_thread_id < 0) {
					do_exit("fail to create stream thread.");
				}
			}
		}
		pthread_join(stream_thread_handle, 0);

		if (source != 0) {
			delete source;
			source = 0;
		}
	}
	return 0;
}
//----------------------------------------------------------------------

void *streaming_thread_main(void *params)
{
	if (is_terminated) return 0;

	INFO("streaming thread start.");
	Encoder *encoder = ((ThreadParams*) params)->encoder;
	HttpHeader *header = ((ThreadParams*) params)->request;

	try {
		int poll_state, rc, wc;
		struct pollfd poll_fd[2];
		unsigned char buffer[BUFFFER_SIZE];

		poll_fd[0].fd = encoder->get_fd();
		poll_fd[0].events = POLLIN | POLLHUP;

		while(!is_terminated) {
			poll_state = poll(poll_fd, 1, 1000);
			if (poll_state == -1) {
				throw(trap("poll error."));
			}
			else if (poll_state == 0) {
				continue;
			}
			if (poll_fd[0].revents & POLLIN) {
				rc = wc = 0;
				rc = read(encoder->get_fd(), buffer, BUFFFER_SIZE - 1);
				if (rc <= 0) {
					break;
				}
				else if (rc > 0) {
					wc = streaming_write((const char*) buffer, rc);
					if (wc < rc) {
						//DEBUG("need rewrite.. remain (%d)", rc - wc);
						int retry_wc = 0;
						for (int remain_len = rc - wc; rc != wc; remain_len -= retry_wc) {
							poll_fd[0].revents = 0;

							retry_wc = streaming_write((const char*) (buffer + rc - remain_len), remain_len);
							wc += retry_wc;
						}
						LOG("re-write result : %d - %d", wc, rc);
					}
				}
			}
			else if (poll_fd[0].revents & POLLHUP)
			{
				if (encoder->state == Encoder::ENCODER_STAT_STARTED) {
					DEBUG("stop transcoding..");
					encoder->ioctl(Encoder::IOCTL_STOP_TRANSCODING, 0);
				}
				break;
			}
		}
	}
	catch (const trap &e) {
		ERROR("%s %s (%d)", e.what(), strerror(errno), errno);
	}
	do_exit(0);
	INFO("streaming thread stop.");

	if (encoder->state == Encoder::ENCODER_STAT_STARTED) {
		DEBUG("stop transcoding..");
		encoder->ioctl(Encoder::IOCTL_STOP_TRANSCODING, 0);
	}

	pthread_exit(0);

	return 0;
}
//----------------------------------------------------------------------

void *source_thread_main(void *params)
{
	Source *source = ((ThreadParams*) params)->source;
	Encoder *encoder = ((ThreadParams*) params)->encoder;
	HttpHeader *header = ((ThreadParams*) params)->request;

	INFO("source thread start.");

	try {
		int poll_state, rc, wc;
		struct pollfd poll_fd[2];
		unsigned char buffer[BUFFFER_SIZE];

		poll_fd[0].fd = encoder->get_fd();
		poll_fd[0].events = POLLOUT;

		poll_fd[1].fd = source->get_fd();
		poll_fd[1].events = POLLIN;

		while(!is_terminated) {
			poll_state = poll(poll_fd, 2, 1000);
			if (poll_state == -1) {
				throw(trap("poll error."));
			}
			else if (poll_state == 0) {
				continue;
			}

			if (poll_fd[0].revents & POLLOUT) {
				rc = wc = 0;
				if (poll_fd[1].revents & POLLIN) {
					rc = read(source->get_fd(), buffer, BUFFFER_SIZE - 1);
					if (rc == 0) {
						break;
					}
					else if (rc > 0) {
						wc = write(encoder->get_fd(), buffer, rc);
						//DEBUG("write : %d", wc);
						if (wc < rc) {
							//DEBUG("need rewrite.. remain (%d)", rc - wc);
							int retry_wc = 0;
							for (int remain_len = rc - wc; rc != wc; remain_len -= retry_wc) {
								poll_fd[0].revents = 0;

								poll_state = poll(poll_fd, 1, 1000);
								if (poll_fd[0].revents & POLLOUT) {
									retry_wc = write(encoder->get_fd(), (buffer + rc - remain_len), remain_len);
									wc += retry_wc;
								}
							}
							LOG("re-write result : %d - %d", wc, rc);
							usleep(500000);
						}
					}
				}
			}
		}
	}
	catch (const trap &e) {
		ERROR("%s %s (%d)", e.what(), strerror(errno), errno);
	}
	INFO("source thread stop.");

	pthread_exit(0);

	return 0;
}
//----------------------------------------------------------------------

int streaming_write(const char *buffer, size_t buffer_len, bool enable_log)
{
	if (enable_log) {
		DEBUG("response data :\n%s", buffer);
	}
	return write(1, buffer, buffer_len);
}
//----------------------------------------------------------------------

void do_exit(const char *message)
{
	is_terminated = true;
	if (message) {
		ERROR("%s", message);
	}
}
//----------------------------------------------------------------------

void signal_handler(int sig_no)
{
	INFO("signal no : %d", sig_no);
	do_exit("signal detected..");
}
//----------------------------------------------------------------------

void show_help()
{
	printf("usage : transtreamproxy [-h]\n");
	printf("\n");
	printf(" * To active debug mode, input NUMBER on /tmp/debug_on file. (default : warning)\n");
	printf("   NUMBER : error(1), warning(2), info(3), debug(4), log(5)\n");
	printf("\n");
	printf(" ex > echo \"4\" > /tmp/debug_on\n");
}
//----------------------------------------------------------------------
