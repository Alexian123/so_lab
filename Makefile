CC=gcc
CFLAGS=-Wall -Iinclude -g

SRC=filestats.c
BIN=filestats

IN=in
IN_BAK=in.bak
OUT=out

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<
	mkdir -p ./$(OUT)

run: clean $(BIN)
	cp -r ./$(IN_BAK) ./$(IN)
	./$(BIN) $(IN) $(OUT)

clean:
	rm -rf $(BIN) $(IN) $(OUT)

.PHONY: clean run