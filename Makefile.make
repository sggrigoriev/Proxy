all:
	$(MAKE) all -e -C WUD -f Makefile.make
	$(MAKE) all -e -C Proxy -f Makefile.make

clean:
	$(MAKE) clean -e -C WUD -f Makefile.make
	$(MAKE) clean -e -C Proxy -f Makefile.make
