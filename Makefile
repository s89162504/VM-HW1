init:		
	@if [ -d "build.qemu" ] || [ -d "qemu-0.13.0" ]; then \
		echo "Error: Directory build.qumu or qemu-0.13.0 already exsits."; \
		echo "Error: make clean first or use make build instead."; \
		false; \
	fi

	tar -xf qemu_virtual_machine.tar.bz2
	mkdir -p build.qemu
	cd build.qemu && \
		../qemu-0.13.0/configure --target-list=i386-linux-user && \
		make

build:
	cp source/* qemu-0.13.0/
	cd build.qemu && make

benchmark: FORCE
	tar -xf benchmark/automotive.tar.gz -C benchmark
	$(MAKE) -C benchmark all

clean:
	rm -rf qemu-0.13.0 build.qemu
	rm -rf benchmark/automotive
FORCE:

