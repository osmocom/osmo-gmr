CC=gcc
CFLAGS=-I../include/ `pkg-config --cflags libosmocore` -Wall
LDFLAGS=`pkg-config --libs libosmocore`

bcch_rx:bcch_rx.o gsmtap.o l1/conv.o l1/crc.o l1/interleave.o l1/punct.o l1/scramb.o l1/bcch.o

clean:
	rm *.o l1/*.o -f
