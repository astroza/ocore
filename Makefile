INCLUDE=/usr/include
all:
	cd OCORE; make;
	mkdir -p $(INCLUDE)/ocore
	cp include/*.h $(INCLUDE)/ocore
clean:
	cd OCORE; make clean
install:
	cd OCORE; make install
