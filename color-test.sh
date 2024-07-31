mkdir -p test_data/
cd test_data/

bindir="./build/bin/convolve/"


if [ "x$1" = "x" ]; then
    echo "Path to dir(arg 1) containing astconvolve not specified, \
defaulting to ./build/bin/convolve/"
else
    bindir=$1
    echo "Using ${bindir}astconvolve"
fi

if [ ! -e arithmetic.fits ]; then
    echo "astarithmetic 100 100 2 makenew 10 mknoise-sigma >/dev/null"
    astarithmetic 100 100 2 makenew 10 mknoise-sigma >/dev/null
fi

if [ ! -e kernel.fits ]; then
    echo "astmkprof --kernel=gaussian,2,3 --oversample=1 >/dev/null"
    astmkprof --kernel=gaussian,2,3 --oversample=1 >/dev/null
fi

cd ../

echo "${bindir}astconvolve test_data/arithmetic.fits \
--kernel=test_data/kernel.fits \
--output=test_data/temp.fits --hdu=1 --hdu=1 \
--domain=spatial"
echo 

${bindir}astconvolve test_data/arithmetic.fits \
            --kernel=test_data/kernel.fits \
            --output=test_data/temp.fits --hdu=1 --hdu=1 \
            --domain=spatial

rm -r test_data/
