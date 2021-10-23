#!/bin/bash

for N in {1..100}

do
	./client -f ./socket/l.sock -R /tmp/TESTFILES/lipsum1.txt,/tmp/TESTFILES/lipsum2.txt,/tmp/TESTFILES/lipsum3.txt
done



exit 0
