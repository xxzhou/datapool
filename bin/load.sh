#!/bin/bash 
NAME=dp
DIR=/data/smartidc/datapool/bin
APP=$DIR/dp
SUPERWISE=superwise.dp


PID=`pidof -o %PPID $APP`  
PYTHON=python

stop()
{
    echo "Stopping $NAME"  
    killall -9 $NAME &> /dev/null
    killall -9 $SUPERWISE &> /dev/null
}
start()
{
	stop
	sleep 1
	echo "Starting $NAME"  
	setsid $PYTHON $SUPERWISE setsid $APP  &> /dev/null &
}
restart()
{
	start
}
cd $DIR
case "$1" in  
	start)  
  		start
		;;
	stop) 
		stop
		;;
	restart)  
		restart
		;;
	*)  
    	echo "usage: $0 {start|stop|restart}"    
esac  
exit 0  
