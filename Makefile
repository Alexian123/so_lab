filestats: filestats.c
	gcc -Wall -o $@ $<
	
clean:
	rm -rf filestats

.PHONY: clean