
TARGET := lib/libloki.a
OBJS := $(patsubst src/%.c, build/%.o, $(wildcard src/*.c))

$(TARGET): $(OBJS) | lib
	loki-elf-ar rc $@ $+
	loki-elf-ranlib $@

build/%.o: src/%.c $(wildcard include/loki/*.h) | build
	loki-clang -ccc-host-triple loki-elf-linux -O3 -Iinclude -c -o $@ $<

clean:
	rm -f $(wildcard $(TARGET) *.o)
	rm -rf $(wildcard lib build)

lib:
	mkdir $@
build:
	mkdir $@
