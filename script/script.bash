#!/bin/bash

for N in {1..100}

do
	ruby ./client.rb lipsum$((($N % 20) + 1)) $N  &
done



exit 0
