PROGRAM = skcmaputil
STRIP = strip
CFLAGS = -O2 -Wall

.PHONY: all clean

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM)

$(PROGRAM): $(PROGRAM).c
	$(CC) $(CFLAGS) -o $@ $^
	$(STRIP) -s $(PROGRAM)
