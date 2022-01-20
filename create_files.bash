#!/bin/bash

mkdir /tmp/LIPSUM
for i in {001..015}; 
    do  touch /tmp/LIPSUM/randfile$i.txt && 
        base64 /dev/urandom | head -c 1M > /tmp/LIPSUM/randfile$i.txt; 
    
    done
