#!/usr/bin/expect -f
set from_filename [lindex $argv 0]
set to_filename [lindex $argv 1]
set nodeip [lindex $argv 2]
set password [lindex $argv 3]
spawn scp $from_filename root@$nodeip:$to_filename
set timeout 30
expect {
"Are you sure you want to continue connecting*" {
	send "yes\r"
	expect "*password:" {send "$password\r"}
	}
"*password:" {send "$password\r"}
}
expect eof
