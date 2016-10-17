#define VERSION "tcanim1v16"

// particles.c - Simple particles example using the OpenVG Testbed
// via Nick Williams (github.com/nilliams)
// https://gist.githubusercontent.com/nilliams/7705819/raw/9cbb5a1298d6eef858639095148ede2c33cb6d40/particles.c
//
// Modified by Brian Jordan, G4EWJ, 2016

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "shapes.h"
#include <dirent.h>
#include <fnmatch.h>
#include <libgen.h>

#define NUM_PARTICLES 		100
#define CYCLEFIELDS		0

	int 			angle ;			
	int 			np ;
	int				changes ;
	struct timeval	now ;
	int 			cycles ;
	int				activecount ;
	int				xoffset ;
	int 			yoffset ;
	char			balloontext [6] ;
	char			filename [1024] ;
	char			filename2 [1024] ;
	char			dir [1024] ;
	char			base [1024] ;
	int				frames ;
	int				banneractive ;
	int 			bannertoggle ;
	int				balloonsactive ;
	char			bannertext [65005] ;
	int				counter1, counter2 ;
	int 			w, h ;

#define	MAXNAMES		1000
	VGImage			save [MAXNAMES] ;
    	struct dirent 		**namelist ;
	int			namecount ;
	int			nameindex ;
	int			nameseconds ;
	int			namefields ;

typedef struct particle 
{
	int	active ;
	double	x, y;
	double 	vx, vy;
	int 	r, g, b;
	double	radius;
	int	cycles ;	
} particle_t;

particle_t particles[NUM_PARTICLES];

int showTrails = 0;
int directionRTL = 1 ;
int alternate = 1;
double gravity = 0.4 ;

int		scan		(void) ;
int		filter		(const struct dirent *) ;

// Initialize _all_ the particles
void initParticles(int w, int h) {
	int i;

	for (i = 0; i < NUM_PARTICLES; i++) 
	{
		particle_t *p = &particles[i];
		p->active = 0 ;
		p->x = 0;
		p->y = 0;
		p->vx = (rand() % 10) + 11;
		p->vy = (rand() % 17) + 20 ;
		p->vy = 1 ;
//		p->vx /= 2 ;
//		p->vy /= 2 ;
		p->r = rand() % 256;
		p->g = rand() % 256;
		p->b = rand() % 256;
		p->radius = (rand() % 40) + 80;
		if (directionRTL) 
		{
			p->vx *= -1;
			p->x = w;
		}
		p->cycles = 1 ;
	}
}

void draw(int w, int h) {
	int 		i;
	particle_t 	*p;
	double		fontsize ;
	double		tempf ;
	int 		temp ;
	
	vgSetPixels (0,0,save[nameindex],0,0,w,h) ;
	
	activecount = 0 ;
	for (i = 0; i < NUM_PARTICLES ; i++) 
	{
		p = &particles[i];
		if (p->active)
		{
			activecount++ ;
			Fill(p->r, p->g, p->b, 1.0);
			Circle(p->x, p->y, p->radius);

			tempf = 0.299 * (float)p->r + 0.587 * (float)p->g + 0.114 * (float)p->b ;
			if (tempf >= 128)
			{
				Fill (0,0,0,1) ;
			}
			else
			{
				Fill (255,255,255,1) ;
			}
			fontsize = (14+2*strlen(balloontext)) * p->radius / TextWidth(balloontext,SansTypeface,25) ;	
			TextMid (p->x,p->y-fontsize/2,balloontext,SansTypeface,fontsize) ;	

		// Apply the velocity
			p->x += p->vx / 4 ;
			p->y += p->vy / 4 ;

//			p->vx *= 0.98;
//			if (p->vy > 0) p->vy *= 0.97;

		// Gravity
			p->vy -= gravity / 1.25 ;

		// Stop particles leaving the canvas  
			if (p->x < -50)	p->x = w + 50;
			if (p->x > w + 50) p->x = -50;

		// When particle reaches the bottom of screen reset velocity & start posn
			if (p->y < -50) 
			{
				p->cycles++ ;
				p->x = 0;
				p->y = 0;
				p->vx = (rand() % 20) + 10;
				p->vy = (rand() % 15) + 25;
	
				if (directionRTL) {
					p->vx *= -1;
					p->x = w;
				}
				rand() ;
				p->r = rand() % 256;
				p->g = rand() % 256;
				p->b = rand() % 256;
				p->radius = (rand() % 35) + 35 ;
				if (CYCLEFIELDS && cycles >= CYCLEFIELDS && banneractive == 2)
				{
					p->active = 0 ;
				}
			}

			if (p->y > h + 50) p->y = -50;
		}
	}
		if (banneractive == 1)
		{
			Fill (0,0,0,1) ;
			Rect (0,0,720,54) ;
			if (bannertoggle & 1)
			{
				Fill (128,0,192,1) ;
			}
			else
			{
				Fill (128,0,192,1) ;
			}
			Rect (0,20,720,34) ;
			Fill (255,255,255,1) ;
			Text (w-frames*2,28,bannertext,SansTypeface,19) ;
			frames++ ;
			temp = TextWidth (bannertext,SansTypeface,19) ;
			if (temp - frames * 2 < -720)
			{
				frames = 0 ;
				bannertoggle++ ;
				if (CYCLEFIELDS && cycles >= CYCLEFIELDS)
				{
					banneractive = 2 ;
				}
			}
		}
}

void makeImage ()
{
	for (nameindex = 0 ; nameindex < namecount ; nameindex++)
	{
		Start(w, h);
		save [nameindex] = vgCreateImage (VG_lRGBX_8888,w,h,VG_IMAGE_QUALITY_NONANTIALIASED) ;
		Fill (0,0,0,1);
		Rect (0, 0, w, h);
		sprintf (filename2,"%s/%s",dir,namelist[nameindex]->d_name) ;
		Image (xoffset,yoffset,720-xoffset,576-yoffset,filename2) ;
		vgFinish() ;
		vgGetPixels (save[nameindex],0,0,0,0,w,h) ;
//		End() ;
//		SaveEnd ("png.png") ;
	}
}

// Display Options
// -t  show trails
// -g[value] gravity
//
// Direction (alternates by default)
// -r  right-to-left
// -l  left-to-right

int main(int argc, char **argv) 
{
	int 		i ;
	int		x ;
	int 		temp ;
	int		errors ;
	char		*p ;	

	counter1 = 0 ; counter2 = 0 ;

	errors = 0 ;
	if (argc != 6)
	{
		errors++ ;
	}
	else
	{
		strncpy (filename,argv[1],sizeof(filename)-5) ;
		p = strchr (filename,'*') ;
		if (p)
		{
			nameseconds = atoi (p+1) ;
			if (nameseconds == 0)
			{
				nameseconds = 1 ;
			}
			*p = 0 ;
			p = strstr (filename,".jpg") ;
			if (p)
			{
				*p = 0 ;
			}
			strcat (filename,"*.jpg") ;	
		}
		p = strstr (filename,".jpg") ;
		if (p == 0)
		{
			strcat (filename,".jpg") ;
		}

		strcpy (dir,filename) ;
		strcpy (dir,dirname(dir)) ;
		strcpy (base,filename) ;
		strcpy (base,basename(base)) ;

		namecount = scan() ;
		if (namecount <= 0)
		{
			printf ("Cannot find %s\n\n",filename) ;
			exit (3) ;
		}


		if (namecount > MAXNAMES)
		{
			namecount = MAXNAMES ;
		}

		xoffset = atoi (argv[2]) ;
		if (abs(xoffset) > 720)
		{
			errors++ ;
		}
		yoffset = atoi (argv[3]) ;
		if (abs(yoffset) > 576)
		{
			errors++ ;
		}
		strncpy (balloontext,argv[4],sizeof(balloontext)-1) ;
		strncpy (bannertext,argv[5],sizeof(bannertext)-5) ;
		if (strlen(bannertext))
		{
			strcat (bannertext,"    ") ;
		}
	}
	printf ("\n") ;
	if (errors)
	{
		printf ("Usage: %s \"FILENAME(S)\" XOFFSET YOFFSET \"BALLOONTEXT\" \"BANNERTEXT\" \n",VERSION) ;
		printf ("                                +/-720  +/-576   Max 5         Max 65000    \n") ;
		printf ("FILENAME(S) may be a single file or FILENAMES*n - it is not necessary to specify the .jpg extension\n") ;
		printf ("FILENAMES*n displays all files that start with FILENAMES in alpha order changing every n seconds\n") ;
		printf ("FILENAME(S) should be enclosed in double quotes\n") ;
		printf ("\n") ;
		exit (2) ;
	}

	printf ("\n%s: Test Card Animator\n",VERSION) ;
	if (namecount > 0)
	{
		printf ("Using image files in this order: \n") ;
		for (nameindex = 0 ; nameindex < namecount ; nameindex++)
		{
			printf ("%s/%s\n",dir,namelist[nameindex]->d_name) ;
		}
	}
	printf ("\n") ;

	init (&w, &h);
	w = 720 ; h = 576 ;
	makeImage() ;	
	Start (w,h) ;
	vgSetPixels (0,0,save[0],0,0,w,h) ;
	End() ;

	namefields = 0 ;
	nameindex = 0 ;
	
    while (1)
    {
	counter1++ ;
    memset (particles,0,sizeof(particles)) ;
	frames = 0 ;
	cycles = 0 ;
	changes = 0 ;
	angle = 0 ;
	directionRTL = 1 ;
	alternate = 1 ;

	if (strlen(balloontext) > 0 )
	{
		np = 7 ;
	}
	else
	{
		np = 0 ;
	}
	
	srand(time(NULL));

	initParticles(w, h);
	for (x = 0 ; x < np ; x++)
	{
		particles[x].active = 1 ;		
	}

	usleep (1000000) ;

	if (strlen(bannertext) > 0)
	{
		banneractive = 0 ;
	}
	else
	{
		banneractive = 2 ;
	}
		
	balloonsactive = 0 ;
	while (1) 
	{
		Start (w, h);
		counter2++ ;
		draw(w, h);		
		End() ;
		cycles++ ;

		namefields++ ;
		if (namefields >= nameseconds * 50)
		{
			nameindex++ ;
			namefields = 0 ;
		}
		if (nameindex >= namecount)
		{
			nameindex = 0 ;
		}

		if (activecount == 0 && CYCLEFIELDS && cycles >= CYCLEFIELDS && frames == 0)
		{			
			banneractive = 2 ;
			Start (w, h);
			counter2++ ;
			draw(w, h);		
			End() ;
			break ;
		}

		temp = 1 ;
		for (x = 0 ; x < np ; x++)
		{
			temp &= particles[x].cycles ;
		}
		if (temp && banneractive == 0 && strlen(bannertext))
		{
			frames = 0 ;
			banneractive = 1 ;
		}

		gettimeofday (&now,0) ;
		x = now.tv_sec * 1000 + now.tv_usec / 1000 ;

		// Change launch direction every 25 draws
		i++;
		if (alternate && i == 25) 
		{
			changes++ ;
			directionRTL = directionRTL ? 0 : 1;
			i = 0;
		}
	}
//	finish() ;
    }
}

int filter (const struct dirent *sdx)
{
	if (fnmatch(base,sdx->d_name,0)==0)
	{
		return (1) ;
	}
	else
	{
		return (0) ;
	}
}

int scan()
{    
	int 	n ;

    	n = scandir(dir, &namelist, filter, alphasort) ;
    	if (n < 0)
    	{
//	    perror("scandir") ;
    	}
	return (n) ;
}


