#!/bin/bash

sleep 15 
S=$(ps | grep prog | xargs | cut -d' ' -f1)
if [ $S ]
then
	kill -3 $S 
fi
