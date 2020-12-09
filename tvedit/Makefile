CC = clang

LDFLAGS = -lm
CFLAGS = -O3 -D_GNU_SOURCE -fno-strict-aliasing -Wall -Wextra \
	-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
	-Wdeclaration-after-statement -Wno-missing-field-initializers \
	-Wno-unused-parameter

debug debug32: CFLAGS += -O0 -gdwarf-4 -fno-omit-frame-pointer \
	-fstack-protector-all -fsanitize=address -fsanitize=undefined
debug debug32: LDFLAGS := -lasan -lubsan $(LDFLAGS)

32bit: CFLAGS += -m32
32bit: LDFLAGS += -m32

disasm: CFLAGS += -fverbose-asm

bin = synctool
obj = synctool.o
asm = $(obj:.o=.s)

all: $(bin)
32bit: $(bin)

debug: all
debug32: 32bit
disasm: $(asm)

synctool: synctool.o
	$(CC) $< $(LDFLAGS) -lfftw3f -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDES) -c -o $@

%.s: %.c
	$(CC) $< $(CFLAGS) $(INCLUDES) -c -S -o $@

clean:
	rm -f synctool *.o *.s
