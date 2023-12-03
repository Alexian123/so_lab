#!/bin/sh

while read line
do
	new_line=`echo $line | grep -E  "^[A-Z].*\.$" | grep -E -v ", ?si"`
	if test -z "$new_line"
	then
		echo "Eroare"
	else
		echo $new_line
	fi
done

exit 0
