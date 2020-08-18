#!/bin/bash

sleep 25 
S=$(ps | grep prog | xargs | cut -d' ' -f1)
if [ $S ]
then
	kill -1 $S 
fi
