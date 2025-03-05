all: novadis oct2bin

novadis: novadis.o
	$(CC) -o $@ $^

oct2bin: oct2bin.o
	$(CC) -o $@ $^
