#!/bin/bash
#BINDIR=BAD
BINDIR="$(basename `pwd`)"
#echo $BINDIR
varCXX=$(echo ${BINDIR//*_})
#echo "varCXX" $varCXX
woCXX=$(echo ${BINDIR%_$varCXX})
#echo "woCXX" $woCXX
varCC=$(echo ${woCXX//*_})
#echo "varCC" $varCC
woCXX_CC=$(echo ${woCXX%_$varCC})
#echo "woCXX_CC" $woCXX_CC
accel=$(echo ${woCXX_CC//*_})
#echo "accel" $accel
woCXX_CC_accel=$(echo ${woCXX_CC%_$accel})
#echo "woCXX_CC_accel" $woCXX_CC_accel
mpi=$(echo ${woCXX_CC_accel//*_})
#echo "mpi" $mpi
cmakefile=$(echo ${woCXX_CC_accel%_$mpi})
#echo "cmakefile" $cmakefile

#echo $compiler
#echo "Cmake config file is:" $cmakefile".cmake"
export MOD_FEM_ARCH_CMAKE="$cmakefile"
echo "MOD_FEM_ARCH_CMAKE is:" $MOD_FEM_ARCH_CMAKE
echo "Cmake config file is:" $cmakefile".cmake"

if [ ! -f ../../src/cmake/Platforms/"$cmakefile".cmake ]
then
    echo "File" $cmakefile".cmake not found in ../../src/cmake/Platforms."
    echo "Name your directory: [CMAKE_CONFIG_FILENAME]_[mpi|nompi]_[none|opencl|cuda]_[CC]_[CXX]"
    echo "Exiting."
    exit
fi

export CC="$varCC"
export CXX="$varCXX"

#if [ "$compiler" == "gcc" ]
#then
#    export CC=gcc
#    export CXX=g++
#elif [ "$compiler" == "icc" ]
#then
#    export CC=icc
#    export CXX=icpc
#else
#    export CC=gcc
#    export CXX=g++
#    echo "Compiler unknown/not present in folder name. Setting default."
#    echo "Name your directory: [CMAKE_CONFIG_FILENAME]_[gcc|icc]"
#fi

echo "C compiler is:" $CC
echo "C++ compiler is:" $CXX
echo "MODFEM_MPI:" $mpi
echo "MODFEM_ACCEL:" $accel

echo "Starting cmake..."
cmake -DMODFEM_MPI:STRING=$mpi -DMODFEM_ACCEL:STRING=$accel ../../src

#for x in $arr
#do
#    echo "> [$x]"
#done
