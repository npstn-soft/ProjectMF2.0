EXE		= mf
CFLAGS	+= -O2 -s
PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
LIBS	= -lm

all:
	$(CC) $(CFLAGS) -c mf.c
	$(CC) $(CFLAGS) -c resample2.c
	$(CC) $(CFLAGS) -o $(EXE) $(LIBS) *.o -lm

install:
	install -m 755 -d $(BINDIR)
	install -m 755 $(EXE) $(BINDIR)/$(EXE)

.PHONY:
	install
