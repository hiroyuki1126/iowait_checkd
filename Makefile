CC = gcc
CFLAGS = -Wall -O2
C_DEBUG_FLAGS = -g -DIOC_DEBUG

SCRIPT = iowait_checkd.sh
PROGRAM = iowait_checkd
CONF = iowait_checkd.conf
SRCS = iowait_checkd.c
OBJS = $(SRCS:.c=.o)

bindir = /root/bin
sysconfdir = /root/conf

.SUFFIXES:
.SUFFIXES: .c .o

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c $<

debug: CFLAGS+=$(C_DEBUG_FLAGS)
debug: all

clean:
	rm -f $(PROGRAM) $(OBJS)

install: $(PROGRAM)
	install -o root -g root -m 0755 -d $(bindir)
	install -o root -g root -m 0755 $(SCRIPT) $(bindir)
	install -o root -g root -m 0755 $(PROGRAM) $(bindir)
	install -o root -g root -m 0755 -d $(sysconfdir)
	install -o root -g root -m 0644 $(CONF) $(sysconfdir)
