filestats: filestats.c
	gcc -Wall -o $@ $<
	
clean:
	rm -f filestats stats.txt

.PHONY: clean
