#!/bin/bash

for N in {1..50}

do
	ruby client.rb lipsum$((($N % 20) + 1)) &
done
wait
