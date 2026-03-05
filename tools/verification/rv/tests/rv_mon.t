#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
source ../tests/engine.sh
test_begin

set_timeout 30s

RVDIR=/sys/kernel/tracing/rv/

# Help and basic tests
check "verify mon subcommand help" \
	"$RV mon --help" 0 "run a monitor"

# Error handling tests
check "mon without monitor name" \
	"$RV mon" 1 "usage: rv mon"

check "invalid monitor name" \
	"$RV mon invalid" 1 "monitor invalid does not exist"

check "invalid reactor name" \
	"$RV mon wwnr -r invalid" 1 "failed to set invalid reactor, is it available?"

# rv mon runs until terminated
set_expected_timeout 2s

# Run monitors with different configurations
check_if_exists "run the monitor without parameters" \
	"$RV mon wwnr" "$RVDIR/monitors/wwnr" "" "."

check_if_exists "run the monitor as verbose" \
	"$RV mon wwnr -v" "$RVDIR/monitors/wwnr" \
	"my pid is \$pid" "\(event\|error\)"

check_if_exists "run the monitor with a reactor" \
	"$RV mon wwnr -r printk & sleep .5 && cat $RVDIR/monitors/wwnr/reactors && wait" \
	"$RVDIR/monitors/wwnr/reactors" "\[printk\]"

check_if_exists "run a nested monitor with a reactor" \
	"$RV mon snroc -r printk & sleep .5 && cat $RVDIR/monitors/sched/snroc/reactors && wait" \
	"$RVDIR/monitors/wwnr/reactors" "\[printk\]"

check_if_exists "run an explicitly nested monitor with a reactor" \
	"$RV mon sched:sssw -r printk & sleep .5 && cat $RVDIR/monitors/sched/sssw/reactors && wait" \
	"$RVDIR/monitors/wwnr/reactors" "\[printk\]"

# Regexes for the trace
header="^[[:space:]]\+\(\([][A-Z_x<>-]\+\||\)[[:space:]]*\)\+$"
type="\(event\|error\)[[:space:]]\+"
genpid="[0-9]\+[[:space:]]\+"
selfpid="\$pid[[:space:]]\+"
cpu="\[[0-9]\{3\}\][[:space:]]\+"
state="[a-z_]\+ "
trace_task="${genpid}${cpu}${type}${genpid}${state}"
trace_task_self="${genpid}${cpu}${type}${selfpid}${state}"
trace_cpu="${genpid}${cpu}${type}${state}"
trace_cpu_self="${selfpid}${cpu}${type}${state}"

check_if_exists "run per-task monitor with tracing" \
	"$RV mon sssw -t" "$RVDIR/monitors/sched/sssw" \
	"$header" "$trace_task_self" "\($header\|$trace_task\)"

check_if_exists "run per-task monitor tracing also self" \
	"$RV mon sched:sssw -t -s" "$RVDIR/monitors/sched/sssw" \
	"$trace_task_self" "" "\($header\|$trace_task\)"

check_if_exists "run per-cpu monitor with tracing" \
	"$RV mon sched:sco -t" "$RVDIR/monitors/sched/sco" \
	"$header" "$trace_cpu_self" "\($header\|$trace_cpu\)"

check_if_exists "run per-cpu monitor tracing also self" \
	"$RV mon sco -t -s" "$RVDIR/monitors/sched/sco" \
	"$trace_cpu_self" "" "\($header\|$trace_cpu\)"

test_end
