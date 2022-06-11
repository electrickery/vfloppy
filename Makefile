CFLAGS= -Wall -fsigned-char

epspdv3 : epspCode.o epspdv3.o epspProtocol.o imageCode.o logger.o
	cc -o epspdv3 epspdv3.o epspCode.o epspProtocol.o imageCode.o logger.o $(CFLAGS)

epspProtocol.o : epspProtocol.c epspv3.h epspHeaders.h 
	cc -c epspProtocol.c $(CFLAGS)
	
epspCode.o : epspCode.c fdc.h epspv3.h epspHeaders.h
	cc -c epspCode.c $(CFLAGS)

imageCode.o : imageCode.c fdc.h epspv3.h epspHeaders.h
	cc -c imageCode.c $(CFLAGS)

epspdv3.o : epspdv3.c fdc.h epspv3.h logger.h
	cc -c epspdv3.c logger.c $(CFLAGS)

logger.o : logger.c logger.h
	cc -c logger.c $(CFLAGS)


clean:
	rm -f *.o ; rm -f epspdv3 

strip:
	strip epspdv3 ; strip

package:
	tar cvzf ../vfloppy23.tgz ./Makefile ./gpl.txt ./*.c ./*.h ./images/*.d88 ./epspdrun.sh 
