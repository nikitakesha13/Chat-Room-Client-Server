all: crsd crc

crc: crc.c
	gcc -o crc crc.c -lpthread

crsd: crsd.c
	gcc -o crsd crsd.c -lpthread

clean:
	rm -rf *.o crc crsd 