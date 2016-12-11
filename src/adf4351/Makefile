all: adf4351

CFLAGS	= -Wall -g -O2 -Wno-unused-variable
LDFLAGS	= -lm -lrt -lpthread  -lwiringPi

adf4351 : adf4351.h adf4351.c ctladf4351.c
		$(CC) $(CFLAGS) -o adf4351 adf4351.c ctladf4351.c $(LDFLAGS) 
clean:
	rm -f  *.o




