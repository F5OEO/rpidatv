INCLUDEFLAGS=-I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -fPIC
LIBFLAGS=-L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -ljpeg
FONTLIB=/usr/share/fonts/truetype/ttf-dejavu
FONTFILES=DejaVuSans.inc  DejaVuSansMono.inc DejaVuSerif.inc
FFT_C = ./hello_fft/mailbox.c ./hello_fft/gpu_fft.c ./hello_fft/gpu_fft_base.c ./hello_fft/gpu_fft_twiddles.c ./hello_fft/gpu_fft_shaders.c
all:	font2openvg fonts library rpidatvtouch tcanim	

libshapes.o:	libshapes.c shapes.h fontinfo.h fonts 
	gcc -O2 -Wall $(INCLUDEFLAGS) -c libshapes.c

gopenvg:	openvg.go
	go install .

oglinit.o:	oglinit.c
	gcc -O2 -Wall $(INCLUDEFLAGS) -c oglinit.c

font2openvg:	fontutil/font2openvg.cpp
	g++ -I/usr/include/freetype2 fontutil/font2openvg.cpp -o font2openvg -lfreetype

fonts:	$(FONTFILES)

DejaVuSans.inc: font2openvg $(FONTLIB)/DejaVuSans.ttf
	./font2openvg $(FONTLIB)/DejaVuSans.ttf DejaVuSans.inc DejaVuSans

DejaVuSerif.inc: font2openvg $(FONTLIB)/DejaVuSerif.ttf
	./font2openvg $(FONTLIB)/DejaVuSerif.ttf DejaVuSerif.inc DejaVuSerif

DejaVuSansMono.inc: font2openvg $(FONTLIB)/DejaVuSansMono.ttf
	./font2openvg $(FONTLIB)/DejaVuSansMono.ttf DejaVuSansMono.inc DejaVuSansMono

clean:
	rm -f *.o *.inc *.so font2openvg *.c~ *.h~
	indent -linux -c 60 -brf -l 132  libshapes.c oglinit.c shapes.h fontinfo.h

library: oglinit.o libshapes.o
	gcc $(LIBFLAGS) -shared -o libshapes.so oglinit.o libshapes.o 

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..
LIBFLAGS2=-L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -lbcm_host -lpthread  -ljpeg -lrt -lm -ldl -lfftw3f

rpidatvtouch: rpidatvtouch.c  
	gcc -Wall $(INCLUDEFLAGS2) -o  rpidatvgui rpidatvtouch.c  libshapes.o oglinit.o $(LIBFLAGS2) 

tcanim: tcanim.c
	gcc -Wall $(INCLUDEFLAGS2) -o  tcanim tcanim.c  libshapes.o oglinit.o $(LIBFLAGS2)

install:
	install -m 755 -p font2openvg /usr/bin/
	#install -m 755 -p rpidatvgui /usr/local/bin/
	cp rpidatvgui ../../bin/
	cp tcanim ../../bin
	install -m 755 -p libshapes.so /usr/lib/libshapes.so.1.0.0
	strip --strip-unneeded /usr/lib/libshapes.so.1.0.0
	ln -f -s /usr/lib/libshapes.so.1.0.0 /usr/lib/libshapes.so
	ln -f -s /usr/lib/libshapes.so.1.0.0 /usr/lib/libshapes.so.1
	ln -f -s /usr/lib/libshapes.so.1.0.0 /usr/lib/libshapes.so.1.0
	install -m 644 -p shapes.h /usr/include/
	install -m 644 -p fontinfo.h /usr/include/

uninstall:
	rm -f /usr/bin/font2openvg
	rm -f /usr/lib/libshapes.so.1.0.0 /usr/lib/libshapes.so.1.0 /usr/lib/libshapes.so.1 /usr/lib/libshapes.so
	rm -f /usr/include/shapes.h /usr/include/fontinfo.h
