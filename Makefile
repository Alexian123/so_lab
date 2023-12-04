CC=gcc
CFLAGS=-Wall -Iinclude

SRC=filestats.c
BIN=filestats

IN_BAK=in.bak
IN=in
OUT=out
OUT_ARCV=statistics.zip

CHAR=a

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<
	mkdir -p ./$(OUT)

run: clean $(BIN)
	cp -r ./$(IN_BAK) ./$(IN)
	./$(BIN) $(IN) $(OUT) $(CHAR)

clean:
	rm -rf $(BIN) $(IN) $(OUT) $(OUT_ARCV)

.PHONY: clean run