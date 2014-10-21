#!/usr/bin/expect

spawn telnet 127.0.0.1 8888
send "END\r"
send "exit\r"
expect eof
