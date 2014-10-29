lokilib: lokilib.c lokilib.h loki_constants.h Makefile
	loki-clang -ccc-host-triple loki-elf-linux -O2 -c -o lokilib.o lokilib.c
	loki-elf-ar rc liblokilib.a lokilib.o
	loki-elf-ranlib liblokilib.a
	rm -rf lokilib.o

clean:
	rm -rf liblokilib.a
