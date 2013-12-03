#!/bin/sh
sudo ./scull_unload
make
sudo ./scull_load
sudo chmod 777 /dev/scull*
