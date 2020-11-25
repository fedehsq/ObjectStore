#!/bin/bash
serverPid=$(pidof myserver)

#ridirigo stdoud e stderr dei client su testout.log
exec &>testout.log
while read line; do 
	./myclient $line 1 &
done < nomi.txt
wait

i=1
while read lline; do 
	if [ $i -le 30 ]; then
  		./myclient $lline 2 &
	else
		./myclient $lline 3 &
	fi
	(( i++ ))
done < nomi.txt
wait

kill -SIGUSR1 $serverPid
wait
