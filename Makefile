CC := gcc
CFLAGS := -O3 -s
MRUBY_CONFIG := build_config.rb

build:
	MRUBY_CONFIG=$(MRUBY_CONFIG) rake -f mruby/Rakefile
	$(CC) -shared -fPIC $(CFLAGS) -omruby.so mruby.c -Imruby/include mruby/build/host/lib/libmruby.a

clean:
	rake -f mruby/Rakefile clean
	rm -f mruby.so
