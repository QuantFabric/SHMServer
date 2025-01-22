#!/bin/bash
ulimit -c unlimited
ulimit -s 16384
WORK_PATH=$(cd $(dirname $0); pwd)
LOG_PATH=$WORK_PATH/log
mkdir -p $LOG_PATH
cd $WORK_PATH
nohup $WORK_PATH/build/PubServer PubServer > $LOG_PATH/PubServer.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/SubClient 10000 PubServer > $LOG_PATH/SubClient10000.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/SubClient 10001 PubServer > $LOG_PATH/SubClient10001.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/SubClient 10002 PubServer > $LOG_PATH/SubClient10002.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/SubClient 10003 PubServer > $LOG_PATH/SubClient10003.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/SubClient 10004 PubServer > $LOG_PATH/SubClient10004.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/SubClient 10005 PubServer > $LOG_PATH/SubClient10005.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/SubClient 10006 PubServer > $LOG_PATH/SubClient10006.log 2>&1 &
sleep 1
nohup $WORK_PATH/build/SubClient 10007 PubServer > $LOG_PATH/SubClient10007.log 2>&1 &