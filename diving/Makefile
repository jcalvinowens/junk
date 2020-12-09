CC = gcc

export LDFLAGS = -lm
export CFLAGS = -O2 -D_GNU_SOURCE -fno-strict-aliasing -Wall -Wextra \
	-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
	-Wdeclaration-after-statement -Wno-missing-field-initializers \
	-Wno-unused-parameter

all: altitude-calculator ndl-calculator
obj = args.o haldanian.o

altitude-calculator: altitude-calculator.o
altitude-calculator: model-constants.h $(obj)
	$(CC) $(LDFLAGS) $(obj) altitude-calculator.o -o $@

ndl-calculator: ndl-calculator.o
ndl-calculator: model-constants.h $(obj)
	$(CC) $(LDFLAGS) $(obj) ndl-calculator.o -o $@

model-constants.h:
	./gen-constants.py > model-constants.h

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm -f *.o
	rm -f model-constants.h
	rm -f altitude-calculator
	rm -f ndl-calculator
