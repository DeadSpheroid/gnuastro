mkdir -p data/
cd data/

if [ ! -e arithmetic.fits ]; then
    astarithmetic 15000 15000 2 makenew 10 mknoise-sigma
fi

if [ ! -e kernel.fits ]; then
    astmkprof --kernel=gaussian,2,3 --oversample=1
    echo
fi

cd ../
echo -e "----------------------\033[1;33mPthread CPU\033[0m----------------------"
astconvolve data/arithmetic.fits --kernel=data/kernel.fits --output=pthread_cpu_conv.fits --hdu=1 --hdu=1 --noedgecorrection --domain=spatial

echo -e "----------------------\033[1;33mOpenCL GPU\033[0m----------------------"
astconvolve data/arithmetic.fits --kernel=data/kernel.fits --output=cl_gpu_conv.fits --hdu=1 --hdu=1 --noedgecorrection --cl=1 --domain=spatial

echo -e "----------------------\033[1;33mOpenCL CPU\033[0m----------------------"
astconvolve data/arithmetic.fits --kernel=data/kernel.fits --output=cl_cpu_conv.fits --hdu=1 --hdu=1 --noedgecorrection --cl=2 --domain=spatial

