#!/bin/bash

for N in {1..100}

do
	./client -f ./socket/l.sock -R /tmp/LIPSUM/randfile001.txt,/tmp/LIPSUM/randfile003.txt,/tmp/LIPSUM/randfile005.txt &
done



exit 0
