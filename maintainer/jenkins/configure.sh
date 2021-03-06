#!/bin/bash --login 
# Copyright (C) 2013 Olaf Lenz
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
DIR=`dirname $0`
source $DIR/common.sh

start "CONFIGURE"

# DEFAULTS
[ ! -v configure_params ] && configure_params=""
[ ! -v configure_vars ] && configure_vars=""
[ ! -v with_cuda ] && with_cuda="true"
[ ! -v with_mpi ] && with_mpi="true"
[ ! -v with_fftw ] && with_fftw="true"
outp configure_params configure_vars with_cuda with_mpi with_fftw

# change into build dir
pushd $builddir

# set up configure params
if $with_mpi; then
    configure_params="--with-mpi $configure_params"
else
    configure_params="--without-mpi $configure_params"
fi

CUDA_HEADER=$srcdir/src/cuda.h
if $with_cuda; then
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64
    configure_params="--with-cuda $configure_params"
    if [ -e $CUDA_HEADER ]; then
        echo "Using CUDA => deleting mock $CUDA_HEADER..."
        rm $CUDA_HEADER
    fi
else
    configure_params="--without-cuda $configure_params"
    configure_vars="$configure_vars NVCC=/bin/false"
    echo "Not using CUDA => generating mock $CUDA_HEADER..."
    echo "#error ERROR: cuda is not really present but used somewhere." \
        > $CUDA_HEADER
fi

FFTW_HEADER=$srcdir/src/fftw3.h
if $with_fftw; then
    configure_params="--with-fftw $configure_params"
    if [ -e $FFTW_HEADER ]; then
        echo "Using FFTW => deleting mock $FFTW_HEADER..."
        rm $FFTW_HEADER
    fi
else
    configure_params="--without-fftw $configure_params"
    echo "Not using FFTW => generating mock $FFTW_HEADER..."
    echo "#error ERROR: fftw is not really present but used somewhere." \
        > $FFTW_HEADER
fi

cmd "$srcdir/configure $configure_params $configure_vars" || exit $?
end "CONFIGURE"

popd



