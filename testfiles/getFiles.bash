#!/bin/bash
touch lipsum{1..20}.txt 

for i in {1..20}; 
do
    tr -dc A-Za-z0-9 </dev/urandom | head -c 10000 > lipsum${i}.txt
done

