#!/bin/bash
./sigusr > tmp & sleep 2; kill -SIGUSR1 $!; wait; cat tmp | grep -q "SIGUSR1 from [0-9]\+" && echo "OK"
./sigusr | grep -q "No signals were caught" && echo "OK"
rm -f ./tmp
