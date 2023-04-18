CC = clang

export LDFLAGS = -lpthread -lrt
export CFLAGS = -O3 -D_GNU_SOURCE -fno-strict-aliasing -Wall -Wextra \
	-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
	-Wdeclaration-after-statement -Wno-missing-field-initializers

debug debug32: CFLAGS += -O0 -gdwarf-4 -fno-omit-frame-pointer \
	-fstack-protector-all -fsanitize=address -fsanitize=undefined
debug debug32: LDFLAGS := -lasan -lubsan $(LDFLAGS)

32bit: CFLAGS += -m32
32bit: LDFLAGS += -m32

disasm: CFLAGS += -fverbose-asm

bin = chess-engine
obj = main.o board.o negamax.o list.o
asm = $(obj:.o=.s)

tbin = chess-engine-test
tobj = list.o board-tests.o

all: $(bin)
all: runtest
32bit: $(bin)
32bit: runtest

debug: all
debug32: 32bit
disasm: $(asm)

runtest: $(tbin)
	./$(tbin)

$(tbin): $(tobj)
	$(CC) $(CFLAGS) $(LDFLAGS) $(tobj) -o $@

$(bin): $(obj)
	$(CC) $(CFLAGS) $(LDFLAGS) $(obj) -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDES) -c -o $@

%.s: %.c
	$(CC) $< $(CFLAGS) $(INCLUDES) -c -S -o $@

clean:
	rm -f chess-engine chess-engine-test *.o *.s
