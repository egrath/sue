#!/bin/sh
#
# SUE smoke tests. Usage: sh tests/smoke.sh [path-to-sue-binary]
# Requires: expect, xxd. Each test drives the editor through a pty
# and compares saved files byte for byte.

SUE=${1:-./build/sue}
T=/tmp/sue_smoke.$$
PASS=0
FAIL=0

hex() { xxd -p "$1" 2>/dev/null | tr -d '\n'; }

rm -rf "$T"
mkdir -p "$T"

# 1: start and quit
TERM=xterm expect -c "
	set timeout 15
	spawn $SUE
	sleep 1
	send \"\\013q\"
	expect { eof {} timeout { exit 1 } }
" > /dev/null 2>&1 && ok=0 || ok=1
[ $ok = 0 ] && { echo "ok   - start and quit"; PASS=$((PASS+1)); } || { echo "FAIL - start and quit"; FAIL=$((FAIL+1)); }

# 2: type and save (default line endings)
TERM=xterm expect -c "
	set timeout 15
	spawn $SUE $T/2.txt
	sleep 1
	send \"hello world\"
	sleep 1
	send \"\\013s\"
	sleep 1
	send \"\\013q\"
	expect { eof {} timeout { exit 1 } }
" > /dev/null 2>&1
[ "$(hex $T/2.txt)" = "68656c6c6f20776f726c640a" ] \
	&& { echo "ok   - type and save (LF)"; PASS=$((PASS+1)); } \
	|| { echo "FAIL - type and save (LF)"; FAIL=$((FAIL+1)); }

# 3: -d saves CRLF
TERM=xterm expect -c "
	set timeout 15
	spawn $SUE -d $T/3.txt
	sleep 1
	send \"hi\"
	sleep 1
	send \"\\013s\"
	sleep 1
	send \"\\013q\"
	expect { eof {} timeout { exit 1 } }
" > /dev/null 2>&1
[ "$(hex $T/3.txt)" = "68690d0a" ] \
	&& { echo "ok   - -d saves CRLF"; PASS=$((PASS+1)); } \
	|| { echo "FAIL - -d saves CRLF"; FAIL=$((FAIL+1)); }

# 4: delete and undo round-trip
TERM=xterm expect -c "
	set timeout 15
	spawn $SUE $T/4.txt
	sleep 1
	send \"hello world\"
	sleep 1
	send \"\\023\"		;# ^S  left
	send \"\\007\"		;# ^G  delete char under cursor
	send \"\\025\"		;# ^U  undo
	sleep 1
	send \"\\013s\"
	sleep 1
	send \"\\013q\"
	expect { eof {} timeout { exit 1 } }
" > /dev/null 2>&1
[ "$(hex $T/4.txt)" = "68656c6c6f20776f726c640a" ] \
	&& { echo "ok   - delete and undo"; PASS=$((PASS+1)); } \
	|| { echo "FAIL - delete and undo"; FAIL=$((FAIL+1)); }

# 5: F1 is ignored inside the filename prompt
TERM=xterm expect -c "
	set timeout 15
	spawn $SUE
	sleep 1
	send \"\\013s\"
	sleep 1
	send \"$T/5.txt\"
	send \"\\033OP\"	;# F1 (xterm kf1) -> KEY_F(1), must be ignored
	sleep 1
	send \"\\r\"
	sleep 1
	send \"\\013q\"
	expect { eof {} timeout { exit 1 } }
" > /dev/null 2>&1
[ -f "$T/5.txt" ] && [ "$(hex $T/5.txt)" = "0a" ] \
	&& { echo "ok   - F1 ignored in filename prompt"; PASS=$((PASS+1)); } \
	|| { echo "FAIL - F1 ignored in filename prompt"; FAIL=$((FAIL+1)); }

rm -rf "$T"
echo "smoke tests: $PASS passed, $FAIL failed"
[ $FAIL = 0 ]
