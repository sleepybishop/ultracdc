OBJS=\
ultracdc.o

CPPFLAGS := -D_DEFAULT_SOURCE
CFLAGS := -g -O3 -std=c11 -Wall -march=native -funroll-loops -ftree-vectorize
LDLIBS := -lm

all: chunker

chunker: $(OBJS) mmapring.o

.PHONY: clean indent scan 
clean:
	$(RM) *.o *.a

indent:
	clang-format -i *.h *.c

gperf: LDLIBS = -lprofiler -ltcmalloc -lm
gperf: clean chunker
	CPUPROFILE_FREQUENCY=100000 CPUPROFILE=gperf.prof ./chunker dummy.bin
	pprof ./chunker gperf.prof --callgrind > callgrind.gperf
	gprof2dot -c custom --format=callgrind callgrind.gperf -z main | dot -T svg > gperf.svg

ubsan: CC=clang
ubsan: CFLAGS += -fsanitize=undefined,implicit-conversion,integer
ubsan: LDLIBS += -lubsan
ubsan: clean chunker
	./chunker dummy.bin

scan:
	scan-build --status-bugs $(MAKE) all
