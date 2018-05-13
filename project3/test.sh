#!/bin/bash

for i in $(seq 1 2); do
  rm -rf ~/share$i
  mkdir ~/share$i
done

cp -R test_files/* ~/share1

make && ./test
