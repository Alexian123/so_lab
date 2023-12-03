CC=gcc
CFLAGS=-Wall -Iinclude

SRC=filestats.c
BIN=filestats

IN_BAK=in.bak
IN=in
OUT=out

CHAR=a

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<
	mkdir -p ./$(OUT)

run: clean $(BIN)
	cp -r ./$(IN_BAK) ./$(IN)
	./$(BIN) $(IN) $(OUT) $(CHAR)

clean:
	rm -rf $(BIN) $(IN) $(OUT)

.PHONY: clean run