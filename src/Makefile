all:	rpidatv 

#CFLAGS	= -Wall -g -O2 -D DIGITHIN
CFLAGS	= -Wall -g -O2 -Wno-unused-variable -Wno-unused-function 
LDFLAGS	= -lm -lrt -lpthread 

rpidatv : rpigpio.c rpidatv.c dvbsenco8.s fec100.c mailbox.c rpidma.c
		$(CC) $(CFLAGS) -o rpidatv rpidatv.c rpigpio.c dvbsenco8.s fec100.c mailbox.c rpidma.c raspberry_pi_revision.c $(LDFLAGS) 


CFLAGS_MUX	= -Wall -g -O2 
LDFLAGS_MUX	= -lm -lrt -lpthread -L/libmpegts/libmpegts.o

muxests : muxests.c
	$(CC) $(CFLAGS_MUX) -o muxests muxests.c $(LDFLAGS_MUX) 

install:
	cp rpidatv ../bin/
	cp dt-blob.bin /boot/
	$(info !!! You should reboot if it is the first installation !!!)
clean:

	rm -f rpidatv rpidatv.o fec100.o dvbsenco8.o mailbox.o rpigpio.o rpidma.o
