#!/usr/bin/expect

spawn telnet 127.0.0.1 8889
send "END\r"
send "exit\r"
expect eof
