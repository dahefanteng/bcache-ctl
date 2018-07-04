.PHONY: clean
CFLAGS+=`pkg-config --cflags blkid uuid`
LDFLAGS+=`pkg-config --libs blkid uuid`

all:depend bcache-ctl 


SRCS=bcache-ctl.c bcache.c lib.c make-bcache.c
depend: .depend
.depend: $(SRCS)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ > ./.depend;

include .depend

bcache-ctl: bcache-ctl.o bcache.o lib.o make-bcache.o
clean:
	rm -rf *.o
	rm -rf bcache-ctl
	rm -rf *.d
	rm -rf .depend
