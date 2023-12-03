#!/bin/sh

# Script must take 1 argument
if ! test $# -eq 1
then
	echo "Usage: ./$0 <c>"
	exit 1
fi

# Check if argument is alfanumeric character
test_alfanum=`echo $1 | grep "^[a-zA-Z0-9]$"`
if test -z "$test_alfanum"
then
	echo "Argument must be an alfanumeric character!"
	exit 1
fi

# Count correct sentences
count=0
while read line
do
	pattern_line=`echo $line | grep -E  "^[A-Z][a-zA-Z0-9 ,]*[.!?]$" | grep -E -v ", ?si" | grep $1`
	if ! test -z "$pattern_line"
	then
		count=`expr $count + 1`	
	fi
done

# Output result
echo "$count"

exit 0
