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

void *source_thread_main(void *params);
void *streaming_thread_main(void *params);

int streaming_write(const char *buffer, size_t buffer_len, bool enable_log = false);
//----------------------------------------------------------------------

static bool is_terminated = true;
static int source_thread_id, stream_thread_id;
static pthread_t source_thread_handle, stream_thread_handle;
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
	std::string req = HttpHeader::read_request();

	DEBUG("request head :\n%s", req.c_str());

	try {
		if (req.find("\r\n\r\n") == std::string::npos) {
			throw(http_trap("no found request done code.", 400, "Bad Request"));
		}

		if (header.parse_request(req) == false) {
			throw(http_trap("request parse error.", 400, "Bad Request"));
		}

		if (header.method != "GET") {
			throw(http_trap("not support request type.", 400, "Bad Request, not support request"));
		}

		Encoder encoder;
		Source *source = 0;
		ThreadParams thread_params = { 0, &encoder, &header };

		int video_pid = 0, audio_pid = 0, pmt_pid = 0;

		switch(header.type) {
		case HttpHeader::TRANSCODING_FILE:
			try {
				std::string uri = UriDecoder().decode(header.page_params["file"].c_str());
				Mpeg *ts = new Mpeg(uri, false);
				pmt_pid   = ts->pmt_pid;
				video_pid = ts->video_pid;
				audio_pid = ts->audio_pid;
				source = ts;
			}
			catch (const trap &e) {
				throw(http_trap(e.what(), 404, "Not Found"));
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
			catch (const http_trap &e) {
				throw(e);
			}
			break;
		case HttpHeader::TRANSCODING_FILE_CHECK:
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
			throw(http_trap(std::string("not support source type : ") + Util::ultostr(header.type), 400, "Bad Request"));
		}
		thread_params.source = source;

		if (!encoder.retry_open(2, 3)) {
			throw(http_trap("encoder open fail.", 503, "Service Unavailable"));
		}

		if (encoder.state == Encoder::ENCODER_STAT_OPENED) {
			std::string response = header.build_response((Mpeg*) source);
			if (response == "") {
				throw(http_trap("response build fail.", 503, "Service Unavailable"));
			}

			streaming_write(response.c_str(), response.length(), true);

			if (header.type == HttpHeader::TRANSCODING_FILE) {
				((Mpeg*) source)->seek(header);
			}

			if (source->is_initialized()) {
				if (!encoder.ioctl(Encoder::IOCTL_SET_VPID, video_pid)) {
					throw(http_trap("video pid setting fail.", 503, "Service Unavailable"));
				}
				if (!encoder.ioctl(Encoder::IOCTL_SET_APID, audio_pid)) {
					throw(http_trap("audio pid setting fail.", 503, "Service Unavailable"));
				}
				if (pmt_pid != -1) {
					if (!encoder.ioctl(Encoder::IOCTL_SET_PMTPID, pmt_pid)) {
						throw(http_trap("pmt pid setting fail.", 503, "Service Unavailable"));
					}
				}
			}
		}

		is_terminated = false;
		source_thread_id = pthread_create(&source_thread_handle, 0, source_thread_main, (void *)&thread_params);
		if (source_thread_id < 0) {
			is_terminated = true;
			throw(http_trap("souce thread create fail.", 503, "Service Unavailable"));
		}
		else {
			pthread_detach(source_thread_handle);

			if (!source->is_initialized()) {
				sleep(1);
			}

			if (!encoder.ioctl(Encoder::IOCTL_START_TRANSCODING, 0)) {
				is_terminated = true;
				throw(http_trap("start transcoding fail.", 503, "Service Unavailable"));
			}
			else {
				stream_thread_id = pthread_create(&stream_thread_handle, 0, streaming_thread_main, (void *)&thread_params);
				if (stream_thread_id < 0) {
					is_terminated = true;
					throw(http_trap("stream thread create fail.", 503, "Service Unavailable"));
				}
			}
		}
		pthread_join(stream_thread_handle, 0);
		is_terminated = true;

		if (source != 0) {
			delete source;
			source = 0;
		}
	}
	catch (const http_trap &e) {
		ERROR("%s", e.message.c_str());
		std::string error = "";
		if (e.http_error == 401 && header.authorization.length() > 0) {
			error = header.authorization;
		}
		else {
			error = HttpUtil::http_error(e.http_error, e.http_header);
		}
		streaming_write(error.c_str(), error.length(), true);
		exit(-1);
	}
	catch (...) {
		ERROR("unknown exception...");
		std::string error = HttpUtil::http_error(400, "Bad request");
		streaming_write(error.c_str(), error.length(), true);
		exit(-1);
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
	is_terminated = true;
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

void signal_handler(int sig_no)
{
	INFO("signal no : %d", sig_no);
	is_terminated = true;
}
//----------------------------------------------------------------------

void show_help()
{
	printf("usage : transtreamproxy [-h]\n");
	printf("\n");
	printf(" * To active debug mode, input NUMBER on /tmp/debug_on file. (default : warning)\n");
	printf("   NUMBER : error(1), warning(2), info(3), debug(4), log(5)\n");
	printf("\n");
	printf(" ex > echo \"4\" > /tmp/.debug_on\n");
}
//----------------------------------------------------------------------
