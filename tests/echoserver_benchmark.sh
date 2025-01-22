#!/bin/bash
ulimit -c unlimited
ulimit -s 16384
WORK_PATH=$(cd $(dirname $0); pwd)
LOG_PATH=$WORK_PATH/log
mkdir -p $LOG_PATH
cd $WORK_PATH
nohup $WORK_PATH/build/EchoServer EchoServer 10 > $LOG_PATH/EchoServer.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/EchoClient 10001 EchoServer 11 12 > $LOG_PATH/EchoClient10001.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/EchoClient 10002 EchoServer 13 14 > $LOG_PATH/EchoClient10002.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/EchoClient 10003 EchoServer 15 16 > $LOG_PATH/EchoClient10003.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/EchoClient 10004 EchoServer 17 18 > $LOG_PATH/EchoClient10004.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/EchoClient 10005 EchoServer 19 20 > $LOG_PATH/EchoClient10005.log 2>&1 &