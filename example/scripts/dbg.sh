#!/bin/sh

ehco 5 > /tmp/.debug_on

touch /tmp/transtreamproxy.log

tail -f /tmp/transtreamproxy.log

