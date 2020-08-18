#!/bin/bash

K=$( grep K ./conf/config.txt | cut -f 1 -d= --complement)
N=$(wc -l ./logs/log.txt | cut -f 1 -d' ')
T=$(($N-$K))
echo "| id cliente | n. prodotti acquistati | tempo totale nel super. | tempo tot. speso in coda | n. di code visitate |"
head -$T ./logs/log.txt
echo "| id cassa | n. prodotti elaborati | n. di clienti | tempo tot. di apertura | tempo medio di servizio | n. di chiusure |"
tail -$K ./logs/log.txt 


