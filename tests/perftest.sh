#!/bin/bash
ulimit -c unlimited
WORK_PATH=$(cd $(dirname $0); pwd)
LOG_PATH=$WORK_PATH/log
mkdir -p $LOG_PATH
cd $WORK_PATH
# taskset绑定进程到CPU
nohup taskset -c 9 $WORK_PATH/build/EchoServer EchoServer > $LOG_PATH/EchoServer.log 2>&1 &
sleep 1
nohup taskset -c 10 $WORK_PATH/build/EchoClient 10001 EchoServer > $LOG_PATH/EchoClient10001.log 2>&1 &