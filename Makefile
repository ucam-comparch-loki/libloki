
TARGET := lib/libloki.a
OBJS := $(patsubst src/%.c, build/%.o, $(wildcard src/*.c))

$(TARGET): $(OBJS) | lib
	loki-elf-ar rc $@ $+
	loki-elf-ranlib $@

build/%.o: src/%.c $(wildcard include/loki/*.h) | build
	loki-clang -ccc-host-triple loki-elf-linux -O3 -mllvm -unroll-threshold=50 -Iinclude -c -Werror -Wall -o $@ $<

.PHONY: clean
clean:
	rm -f $(wildcard $(TARGET) *.o)
	rm -rf $(wildcard lib build)
	rm -rf $(wildcard html latex)

lib:
	mkdir $@
build:
	mkdir $@

.PHONY: docs
docs:
	doxygen
