bmpstats: bmpstats.c
	gcc -Wall -o $@ $<
	
clean:
	rm -f bmpstats stats.txt

.PHONY: clean
