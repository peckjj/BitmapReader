#!/bin/sh

i=0

while [ $i -lt 50000 ]
do
	echo `expr $i + 1`
	i=`expr $i + 1`
	./go flower.bmp
done
