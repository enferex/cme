CC=clang
CFLAGS=
APP=cme

all: debug

.PHONY: debug
debug: CFLAGS += -g3 -O0 -std=gnu11 -Wall
debug: $(APP)

.PHONY: test-debug
test-debug: CFLAGS += -DTEST_DEBUG
test-debug: debug $(APP)

$(APP): main.c
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	$(RM) $(APP)
