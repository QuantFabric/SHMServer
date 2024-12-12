#!/bin/bash
ulimit -c unlimited
WORK_PATH=$(cd $(dirname $0); pwd)
LOG_PATH=$WORK_PATH/log
mkdir -p $LOG_PATH
cd $WORK_PATH
nohup taskset -c 10 $WORK_PATH/build/EchoServer EchoServer > $LOG_PATH/EchoServer.log 2>&1 &
sleep 1
nohup taskset -c 11 $WORK_PATH/build/EchoClient 10001 EchoServer > $LOG_PATH/EchoClient10001.log 2>&1 &
sleep 1
nohup taskset -c 12 $WORK_PATH/build/EchoClient 10002 EchoServer > $LOG_PATH/EchoClient10002.log 2>&1 &
sleep 1
nohup taskset -c 13 $WORK_PATH/build/EchoClient 10003 EchoServer > $LOG_PATH/EchoClient10003.log 2>&1 &
sleep 1
nohup taskset -c 14 $WORK_PATH/build/EchoClient 10004 EchoServer > $LOG_PATH/EchoClient10004.log 2>&1 &
sleep 1
nohup taskset -c 15 $WORK_PATH/build/EchoClient 10005 EchoServer > $LOG_PATH/EchoClient10005.log 2>&1 &