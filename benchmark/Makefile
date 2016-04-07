all: automotive

automotive: FORCE
	$(MAKE) -C automotive/basicmath
	$(MAKE) -C automotive/bitcount
	$(MAKE) -C automotive/qsort
	$(MAKE) -C automotive/susan

automotive-clean:	
	$(MAKE) -C automotive/basicmath clean
	$(MAKE) -C automotive/bitcount clean
	$(MAKE) -C automotive/qsort clean
	$(MAKE) -C automotive/susan clean



clean: automotive-clean
	
FORCE:
