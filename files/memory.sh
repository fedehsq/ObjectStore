#!/bin/bash

found=0
# per ogni riga del file
while read line; do
    # per ogni parola nella riga
    for word in $line; do

        #se ho trovato "exit:" controllo che la parola successiva sia "0"
        if [ $found -eq 1 ]; then
            if [ $word -eq 0 ]; then
                echo "OK: $word byte lost" >> memoryleak.log
            else
                echo "KO: $word bytes lost" >> memoryleak.log
            fi
            #azzero il contatore
            found=0
        fi

        #devo controllare la parola successiva a "exit:"
        if [ "$word" = "exit:" ]; then
            found=1
        fi
    done
done < testout.log

#controllo se ho avuto memory leak
leak=0
while read line; do
	if [ "$line" != "OK: 0 byte lost" ]; then
		leak=1
	fi
done < memoryleak.log
if [ $leak -eq 1 ]; then
	echo "----------------- Memory leak -----------------"
else
	echo "----------------- NO memory leak -----------------"
fi