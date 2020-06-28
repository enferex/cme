CC=clang
CFLAGS=
APP=cme

all: debug

.PHONY: debug
debug: CFLAGS += -g3 -O0 -std=gnu11 -Wall
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
