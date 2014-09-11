#
# oht Makefile
#

# Compiler settings
CC = gcc
CFLAGS = -O2 -fPIC -Wall -ansi -pedantic -std=gnu99

# Ar settings to build the library
AR = ar
ARFLAGS = rcv

SHLD = ${CC} ${CFLAGS}
LDSHFLAGS = -shared -Wl,-Bsymbolic  -Wl,-rpath -Wl,/usr/lib -Wl,-rpath,/usr/lib
LDFLAGS = -Wl,-rpath -Wl,/usr/lib -Wl,-rpath,/usr/lib

RANLIB = true

RM = rm -f


# Implicit rules

SUFFIXES = .o .c .h .a .so .sl

COMPILE.c=$(CC) $(CFLAGS) -c
.c.o:
	@(echo "compiling $< ...")
	@($(COMPILE.c) -o $@ $<)


SRCS = src/oht.c

OBJS = $(SRCS:.c=.o)


default:	liboht.a liboht.so

liboht.a:	$(OBJS)
	@($(AR) $(ARFLAGS) liboht.a $(OBJS))
	@($(RANLIB) liboht.a)

liboht.so:	$(OBJS)
	@$(SHLD) $(LDSHFLAGS) -o $@.0 $(OBJS) $(LDFLAGS) \
		-Wl,-soname=`basename $@`.0

clean:
	$(RM) $(OBJS)

cleanall:
	$(RM) $(OBJS) liboht.a liboht.so*

check:
	@(cd test ; $(MAKE))

