all: novadis oct2bin hex2bin

novadis: novadis.o
	$(CC) -o $@ $^

oct2bin: oct2bin.o
	$(CC) -o $@ $^

hex2bin: hex2bin.o
	$(CC) -o $@ $^
