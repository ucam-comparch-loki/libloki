
.PHONY: all
all: $(patsubst %.c, %.test, $(wildcard *.c))

%.test: %.elf
	`which lokisim` -run $<

%.elf: %.c
	loki-clang -ccc-host-triple loki-elf-linux -O3 -L../lib -I../include $< -lloki -o $@

.PHONY: clean
clean:
	rm -f $(wildcard *.elf)
