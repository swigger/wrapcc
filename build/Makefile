HOST=$(shell uname -s)
CFLAGS=-g -O0 -I../../../../utillib/ -I../../../../utillib/include -ffunction-sections -fdata-sections
CXXFLAGS=$(CFLAGS) -std=gnu++11
LIBS=  -lstdc++
EXE=wrapcc
OBJS= cc_args.o loadenv.o main.o

ifeq ($(HOST), Linux)
	LDFLAGS+=-Wl,-gc-sections
	LIBS+=-lpthread
endif
ifeq ($(HOST), Darwin)
	LDFLAGS+=-liconv -Wl,-x,-dead_strip
endif

all:$(EXE)

clean:
	-@ rm -f $(OBJS) $(EXE)

$(EXE):$(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

install:
	@ for i in ~/bin/* ; do \
		[ -f $$i -a -x $$i -a ! -L $$i ] || continue ; \
		grep -l net.swigger.tool.wrapcc $$i >/dev/null || continue ; \
		echo $$i ; install -m 755 $(EXE) $$i ;\
	done

vpath %.cpp ../wrapcc ../../../../utillib/base
vpath %.c ../../../../utillib/include
