#!/usr/bin/expect

spawn telnet 192.168.1.127 8888
send "END\r"
send "exit\r"
expect eof
