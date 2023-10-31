bmpstats: bmpstats.c
	gcc -Wall -o $@ $<
	
clean:
	rm -f bmpstats

.PHONY: clean
