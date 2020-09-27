CC=clang
CFLAGS=-std=gnu11 -Wall
APP=cme

all: debug

.PHONY: debug
debug: CFLAGS += -g3 -O0
debug: $(APP)

.PHONY: test
test: test.c
	$(CC) $^ -o $@
	./test

$(APP): main.c
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	$(RM) $(APP)
