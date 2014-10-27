#!/bin/sh

while [ 1 ]; do
	clear
	ps -ef | grep transtreamproxy | grep -v grep | grep -v tail
	sleep 1
done

