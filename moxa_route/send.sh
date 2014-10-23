#!/usr/bin/expect

spawn telnet 127.0.0.1 8889
send "SET"
send "\x00\x15"
send "110:192.168.100.100:2"
send "\x9b\x1d\r"
expect "OK"
send "END\r"

send "exit\r"
expect eof
