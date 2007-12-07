INCLUDE=/usr/include
all:
	cd OCORE; make;
clean:
	cd OCORE; make clean
install: all
	cd OCORE; make install
	mkdir -p $(INCLUDE)/ocore
	cp include/*.h $(INCLUDE)/ocore
