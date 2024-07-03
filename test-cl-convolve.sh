mkdir -p data/
cd data/

bindir="./bin/convolve/"
size=10000

if [ "x$1" = "x" ]; then
    echo "Path to dir(arg 1) containing astconvolve not specified, \
defaulting to ./bin/convolve/"
else
    bindir=$1
    echo "Using ${bindir}astconvolve"
fi

if [ "x$2" = "x" ]; then
    echo "Using 10000 x 10000 image size"
else
    size=$2
    echo "Using $2 x $2 image size"
fi

if [ ! -e arithmetic.fits ]; then
    astarithmetic $size $size 2 makenew 10 mknoise-sigma
fi

if [ ! -e kernel.fits ]; then
    astmkprof --kernel=gaussian,2,3 --oversample=1
    echo
fi

cd ../

echo -e "----------------------\033[1;33mPthread CPU\033[0m--\
--------------------"

${bindir}astconvolve data/arithmetic.fits \
            --kernel=data/kernel.fits \
            --output=pthread_cpu_conv.fits --hdu=1 --hdu=1 \
            --noedgecorrection --domain=spatial


echo -e "----------------------\033[1;33mOpenCL GPU\033[0m----\
------------------"

${bindir}astconvolve data/arithmetic.fits --kernel=data/kernel.fits \
            --output=cl_gpu_conv.fits --hdu=1 --hdu=1 \
            --noedgecorrection --cl=1 --domain=spatial


echo -e "----------------------\033[1;33mOpenCL CPU\033[0m----\
------------------"

${bindir}astconvolve data/arithmetic.fits --kernel=data/kernel.fits \
            --output=cl_cpu_conv.fits --hdu=1 --hdu=1 \
            --noedgecorrection --cl=2 --domain=spatial

