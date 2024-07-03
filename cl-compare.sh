if [ -e cl_cpu_conv.fits ] && [ -e cl_gpu_conv.fits ]; then
    echo -e "----------------------\033[1;33mOpenCL CPU versus\
 OpenCL GPU\033[0m----------------------"

    astarithmetic --output=diff_cl_cpu_vs_cl_gpu.fits --hdu=1 --hdu=1 \
                    cl_cpu_conv.fits cl_gpu_conv.fits - > /dev/null

    aststatistics diff_cl_cpu_vs_cl_gpu.fits

    echo
fi



if [ -e pthread_cpu_conv.fits ] && [ -e cl_gpu_conv.fits ]; then
    echo -e "----------------------\033[1;33mPthread CPU versus\
 OpenCL GPU\033[0m----------------------"

    astarithmetic --output=diff_pthread_cpu_vs_cl_gpu.fits --hdu=1 \
                  --hdu=1 pthread_cpu_conv.fits cl_gpu_conv.fits - > /dev/null

    aststatistics diff_pthread_cpu_vs_cl_gpu.fits

    echo
fi



if [ -e pthread_cpu_conv.fits ] && [ -e cl_cpu_conv.fits ]; then
    echo -e "----------------------\033[1;33mPthread CPU versus\
 OpenCL CPU\033[0m----------------------"

    astarithmetic --output=diff_pthread_cpu_vs_cl_cpu.fits --hdu=1 \
                  --hdu=1 pthread_cpu_conv.fits cl_cpu_conv.fits - > /dev/null

    aststatistics diff_pthread_cpu_vs_cl_cpu.fits

    echo
fi
