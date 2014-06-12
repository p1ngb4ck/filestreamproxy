ftp -n 192.168.$1 << +
user root a
prompt off
cd /usr/bin
put transtreamproxy
bye
+

