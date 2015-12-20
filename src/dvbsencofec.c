/* 	
	DVBSENCO8.c

	DVBS TS Encoder Test for ARM 

	Brian Jordan, G4EWJ, 21 February 2013
	F5OEO, Sept 2014 Integrate with FEC provided by Brian

	This software is provided free for non-commercial amateur radio use.
	It has no warranty of fitness for any purpose.
	Use at your own risk.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "fec100.h"

typedef unsigned char		uchar ;


extern void 	dvbsenco_init	(void) ;
extern uchar*	dvbsenco	(uchar*) ;		

#define SIZE_BUFFILE 188*10
#define SIZE_BUFFIQ 204*20

int main(int argc, char **argv)
{
	uchar		buff [208] ;
	int		fhi, fho ; 
	uchar		*p ;
	int		packetoutcount, bytesin, bytesout ;
	uchar *FileBuff; 
	uchar *FileBuffIQ;

	printf ("\nDVBSENCOFEC Written by G4EWJ Integrated by F5OEO\n") ;

	dvbsenco_init() ;
	packetoutcount = 0 ;

	FileBuff = malloc(SIZE_BUFFILE);
	FileBuffIQ = malloc(SIZE_BUFFIQ);	 
	
	if (argc > 2) 
	{
        	fhi = open(argv[1], 'r');

		if (fhi < 0)
		{
			printf("Failed to open %s Input TSFile\n",argv[1]);
			exit(1);
		}
		else
			printf("Ts(188 packet) %s -->",argv[1]);

		  fho = creat(argv[2],0644) ;
		  if (fho < 0)
		{
			printf("Failed to open %s Output TSFile\n",argv[2]);
			exit(1);
		}
		  else
            printf("%s with ",argv[2]);

		if(argc==4)
		{
			if((atoi(argv[3])==1)||(atoi(argv[3])==2)||(atoi(argv[3])==3)||(atoi(argv[3])==5)||(atoi(argv[3])==7))
			{
				viterbi_init(atoi(argv[3]));
				printf("FEC = %d\n",atoi(argv[3]));
			}
			else 
			{
				printf("Illegal FEC (See usage)!!! \n");
				exit(1);
			}
		}
		else
		{
			viterbi_init(1);
			printf("Default FEC = %d",1);
		}
	}
	else
	{
		printf("Usage : input.ts output.iq [FEC]\nFEC : 1=1/2 2=2/3 3=3/4 5=5/6 7=7/8\n");
		exit(1);
	};

	
	unsigned int ibits;
	unsigned int qbits;

	unsigned char buffIQ[204*2];
	

	int i,j;
	int NbIQOutput=0;
	int NbIQTotal=0;
	int IndexFile=0;
	do
	{	
		bytesin = read (fhi,FileBuff,SIZE_BUFFILE) ;
		
		IndexFile=0;
		NbIQTotal=0;
		while(IndexFile<SIZE_BUFFILE)
		{
				memcpy(buff,FileBuff+IndexFile,188);
				p = dvbsenco (buff) ;
			    	packetoutcount++ ;

				NbIQOutput=viterbi (p,FileBuffIQ+NbIQTotal);
			
				NbIQTotal+=NbIQOutput;	
				IndexFile+=188;
			
			
		}

		bytesout = write (fho,FileBuffIQ,NbIQTotal) ;
		//printf("IQ=%d\n",bytesout);
			if (bytesout != NbIQTotal)
			{
				printf ("\nCannot write to OutputFile\n") ;
			}
			
	} while (bytesin >=188) ;

	printf ("DVBSENCOFEC Done\n\n") ;
	close (fho) ;	
	close (fhi) ;
}



