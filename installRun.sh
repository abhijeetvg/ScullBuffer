#!/bin/sh
gcc -g producer.c -o prod
gcc -g consumer.c -o cons
./prod &
./cons &
