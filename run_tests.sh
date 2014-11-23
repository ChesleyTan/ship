#!/bin/bash

# Using bash to test my shell :/
if [[ $# != 1 ]]; then
    echo "1 arg required: Please specify a program."
    exit 1;
fi
$1 <<< `cat tests.txt`
