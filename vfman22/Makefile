SRCS=vf.c logger.c imageCode.c
OBJS=$(SRCS:.c=.o)
CFLAGS=-DDEBUG=0 -Wall

all: vfread vfwrite vformat vferase vfd2d88

vfread: vfread.o $(OBJS)
	$(CC) -o vfread vfread.o $(OBJS) $(LDFLAGS) $(CFLAGS)

vfwrite: vfwrite.o $(OBJS) 
	$(CC) -o vfwrite vfwrite.o $(OBJS) $(LDFLAGS) $(CFLAGS)

vformat: vformat.o $(OBJS)
	$(CC) -o vformat vformat.o $(OBJS) $(LDFLAGS) $(CFLAGS)

vferase: vferase.o $(OBJS) 
	$(CC) -o vferase vferase.o $(OBJS) $(LDFLAGS) $(CFLAGS)

vfd2d88: vfd2d88.o $(OBJS) 
	$(CC) -o vfd2d88 vfd2d88.o $(OBJS) $(LDFLAGS) $(CFLAGS)
	
depend: $(SRCS)
	for i in $(SRCS);do $(CC) -MM $(CPPFLAGS) $$i;done > .depend

clean:
	rm -f *.o vfread vfwrite vformat vferase vfd2d88 *.exe

strip:
# 	strip vfread ; strip vfwrite ; strip vformat ; strip vferase ; strip vfd2d88
