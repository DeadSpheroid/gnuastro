main: main.c conv.c conv.h cl_utils.c cl_utils.h conv.cl
	@gcc -c cl_utils.c conv.c -lgnuastro -lcfitsio -lOpenCL
	@gcc main.c cl_utils.o conv.o -o main -lgnuastro -lcfitsio -lOpenCL

input:
	$(info Removing any previous input files)
	rm -f data/*.fits
	$(info Generating input image and kernel)
	cd data/ && astmkprof --kernel=gaussian,2,3 --oversample=1 && astarithmetic 5000 5000 2 makenew 10 mknoise-sigma

cpu: main
	@./main cpu

gpu: main
	@./main gpu

both: main
	@./main cpu
	@echo ""
	@./main gpu

view:
	@astscript-fits-view conv_cpu_.fits conv_gpu_.fits data/arithmetic.fits

clean: 
	@rm -f main *.o *.fits

cl-check:
	@clinfo | grep "Device Name"
	$(info Make sure to have clinfo installed with apt-get install clinfo)
	$(info You should be seeing your CPU and GPU here, otherwise OpenCl has not been setup properly on your system)