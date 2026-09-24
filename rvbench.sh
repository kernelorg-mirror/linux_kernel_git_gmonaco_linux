#! /usr/bin/bash

######################################################################
# @author      : Gabriele Monaco (gmonaco@redhat.com)
# @file        : rvbench
# @created     : Wednesday Sep 09, 2026 13:07:56 CEST
#
# @description : Run RV benchmark
######################################################################

if [ $HOSTNAME == virtme-ng ]; then
	tools/verification/rv/rv mon "$1" &
	pid=$!

	stress-ng --cpu-sched 3 --timer 3 --cpu 3 --cpu-method ackermann --cpu-load 50 --timer 3 -t 10 > /dev/null

	kill $pid
	trace-cmd show | sed '/^$/d;/^# /d'

else

	mons=(tqueue_kern tqueue_bpf csched_kern csched_bpf gsched_kern gsched_bpf cempty_kern cempty_bpf)
	for m in "${mons[@]}" ; do
		echo "Running for $m"
		vng --user root ./rvbench.sh "$m" > "${m}_bench"
	done
	cat ./*_bench > rvbench_all
	rm ./*_bench

fi
