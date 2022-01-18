mkdir /tmp/LIPSUM
for i in {001..015}; do touch /tmp/LIPSUM/randfile$i.txt && head -c 1M </dev/urandom > /tmp/LIPSUM/randfile$i.txt; done
