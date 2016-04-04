#!/bin/bash
./sigusr > tmp & sleep 2; kill -SIGUSR1 $!; wait; cat tmp | grep -q "SIGUSR1 from [0-9]\+" && echo "OK"
./sigusr > tmp & sleep 2; kill -SIGUSR1 $!; wait; cat tmp | grep -q "No signals were caught" && echo "Fail"
if
 ./sigusr | grep -q "No signals were caught"
then
 echo "OK"
else
 echo "Fail"
fi
./sigusr > tmp & sleep 2; kill -SIGCONT $!; wait; cat tmp | grep -q "No signals were caught" && echo "OK"
./sigusr > tmp & sleep 2; kill -SIGCONT $!; kill -SIGUSR1 $!; wait; cat tmp | grep -q "SIGUSR1 from [0-9]\+" && echo "OK"
./sigusr > tmp & sleep 2; kill -SIGSTOP $!; kill -SIGCONT $!; kill -SIGUSR1 $!; wait; cat tmp | grep -q "No signals were caught" && echo "Fail"
rm -f ./tmp
