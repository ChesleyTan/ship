#!/bin/bash

# Using bash to test my shell :/
make
./shell <<< `cat tests.txt`
