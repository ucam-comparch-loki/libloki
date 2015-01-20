libloki.a: lokilib.o
	loki-elf-ar rc $@ $+
	loki-elf-ranlib $@

lokilib.o: lokilib.c lokilib.h loki_constants.h
	loki-clang -ccc-host-triple loki-elf-linux -O2 -c -o $@ $<

clean:
	rm -f $(wildcard libloki.a *.o)
