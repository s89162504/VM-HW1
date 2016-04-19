# VM-HW1
QEMU indirect branch handling optimization (Shadow Stack and IBTC)


### Setup QEMU
The source codes are in `qemu_virtual_machine.tar.bz2`.

You need to install `zlib1g-dev` first. Otherwise you may get error `zlib check failed`.
```
$ sudo apt-get install zlib1g-dev
```
Extract the source codes and build QEMU:
```
$ make init
```
This will create two directories `qemu-0.13.0` (contains the source codes extracted) and `build.qemu` (contains the built QEMU binaries).


### Modify codes
Put your modified codes in directory `source` and use
```
$ make build
```
to copy your modified codes to `qemu-0.13.0` and build QEMU again.

My source codes implements two optimizations: shadow stack and indirect branch target cache(IBTC).
File `optimization.c` has two flags indicating whether to use these two optimizations.


### Run Benchmark
I used
[MiBench-automotive](http://vhosts.eecs.umich.edu/mibench/source.html) and 
[CoreMark](http://www.eembc.org/coremark/download.php)
to measure the correctness and effectiveness of my implementation.
Due to the license issue, I didn't upload CoreMark's source codes. However, it is free to download from the website after registration.

Extrack Mibench: 
```
$ make benchmark
```

Run benchmarks:
```
$ ./run-mibench.sh 
$ ./run-coremark.sh
```

Make sure to put the extracted Coremark source codes in directory `benchmark/coremark_v1.0`.


### Clean Up
```
$ make clean
```

### Others
The implementation details can be found at `report/vm_hw1.pdf`.



