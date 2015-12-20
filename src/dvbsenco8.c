/* 	
	DVBSENCO8.c

	DVBS TS Encoder Test for ARM 

	Brian Jordan, G4EWJ, 21 February 2013

	This software is provided free for non-commercial amateur radio use.
	It has no warranty of fitness for any purpose.
	Use at your own risk.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


typedef unsigned char		uchar ;


extern void 	dvbsenco_init	(void) ;
extern uchar*	dvbsenco	(uchar*) ;		


void main()
{
	uchar		buff [208] ;
	int		fhi, fho ; ;
	uchar		*p ;
	int		packetoutcount, bytesin, bytesout ;

	printf ("\nDVBSENCO8 - DVBS Encoder Test\n") ;

	dvbsenco_init() ;
	packetoutcount = 0 ;

	printf ("Opening raw TS file ts13 for input (188 byte packets)\n") ;
	fhi = open ("ts13",O_RDONLY) ;
	if (fhi <= 0) 
	{
		printf ("Cannot open ts13 for input\n") ;
		exit (1) ;
	}

	printf ("Opening ts13erix for output (204 byte packets - should be identical to ts13eri produced by DigiLite Transmit)\n") ;
	fho = creat ("ts13erix",0644) ;
	if (fho <= 0) 
	{
		printf ("Cannot open ts13erix for output\n") ;
		close (fhi) ;
		exit (1) ;
	}

	do
	{	
		bytesin = read (fhi,buff,188) ;
		if (bytesin == 188)
		{
			p = dvbsenco (buff) ;
		    	packetoutcount++ ;
			printf ("\rPackets out: %d ",packetoutcount) ;
			bytesout = write (fho,p,204) ;
			if (bytesout != 204)
			{
				printf ("\nCannot write to ts13erix\n") ;
				bytesin = 0 ;
			}
		}
	} while (bytesin == 188) ;

	printf ("\n\n") ;
	close (fho) ;	
	close (fhi) ;
}



