# Ubuntu, required library packages: 
#   libmysqlclient-dev sqlite3-dev libc6-dev 

SHELL = /bin/sh
SYSTEM = $(shell uname)
C++ = g++
CC = gcc
LDFLAGS = -L. -lboost_system-mt -lboost_asio-mt -lboost_thread-mt -lboost_regex-mt -lboost_chrono-mt -lboost_date_time-mt
CFLAGS =

ifeq ($(SYSTEM),Darwin)
DFLAGS += -D__APPLE__
OFLAGS += -flat_namespace
else
LFLAGS += -lrt
endif

ifeq ($(SYSTEM),FreeBSD)
DFLAGS += -D__FREEBSD__
endif

ifeq ($(SYSTEM),SunOS)
DFLAGS += -D__SOLARIS__
LFLAGS += -lresolv -lsocket -lnsl
endif

CFLAGS += $(OFLAGS) $(DFLAGS) -I.

ifeq ($(SYSTEM),Darwin)
CFLAGS += -I../mysql/include/
endif

OBJS = bf2available.o gpcm.o gpsp.o main.o protocol.o
COBJS =
PROGS = ./gsloginemulator

all: $(OBJS) $(COBJS) $(PROGS)

./gsloginemulator: $(OBJS) $(COBJS)
	$(C++) -o ./gsloginemulator $(OBJS) $(COBJS) $(LFLAGS)

clean:
	rm -f $(OBJS) $(COBJS) $(PROGS)

$(OBJS): %.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -c $<

$(COBJS): %.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

./gsloginemulator: $(OBJS) $(COBJS)

all: $(PROGS)

bf2available.o: bf2available.h
gpcm.o: gpcm.h protocol.h database.h
gpsp.o: gpsp.h socket.h client.h gs_server.h
protocol.o: protocol.h
main.o: memorydb.h bf2available.h gpcm.h gpsp.h