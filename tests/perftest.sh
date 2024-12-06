#!/bin/bash
ulimit -c unlimited
WORK_PATH=$(cd $(dirname $0); pwd)
LOG_PATH=$WORK_PATH/log
mkdir -p $LOG_PATH
cd $WORK_PATH
nohup $WORK_PATH/build/EchoServer EchoServer 9 10 > $LOG_PATH/EchoServer.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/EchoClient 10001 EchoServer 11 12 > $LOG_PATH/EchoClient10001.log 2>&1 &