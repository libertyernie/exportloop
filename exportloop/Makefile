export SHELL = /bin/sh
export CFLAGS=-Wall -O3 -I../ext_includes
export LDFLAGS=-L../src -L ../ext_libs -lvgmstream -lvorbisfile -lmpg123 -lm
export CC=gcc
export AR=ar
export STRIP=strip

.PHONY: libvgmstream.a

exportloop.exe: libvgmstream.a
	$(CC) $(CFLAGS) $(LDFLAGS) "-DVERSION=\"`../version.sh`\"" exportloop.c $(LDFLAGS) -o exportloop
	$(STRIP) exportloop

libvgmstream.a:
	$(MAKE) -C ../src $@

clean:
	rm -f exportloop exportloop.o
