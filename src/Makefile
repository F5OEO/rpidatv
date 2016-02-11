all:	rpidatv

#CFLAGS	= -Wall -g -O2 -D DIGITHIN
CFLAGS	= -Wall -g -O2 
LDFLAGS	= -lm -lrt -lpthread 


rpidatv : rpigpio.c rpidatv.c dvbsenco8.s fec100.c mailbox.c rpidma.c
		$(CC) $(CFLAGS) -o ../bin/rpidatv rpidatv.c rpigpio.c dvbsenco8.s fec100.c mailbox.c rpidma.c $(LDFLAGS) 

clean:

	rm -f rpidatv rpidatv.o fec100.o dvbsenco8.o mailbox.o rpigpio.o rpidma.o
