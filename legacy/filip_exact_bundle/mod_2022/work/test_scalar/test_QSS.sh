#!/bin/bash
export ARCH=dell_ocl_test
export VAL=80
cp ../../src/OpenCL_kernels/apr_ocl_num_int_el_reference_prism_QSS.cl tmr_ocl_num_int_el.cl
for((i=0;$i<$VAL;i++)) ; do
/home/fkruzel/Kod/mod_2015/bin/$ARCH/MFEM_conv_diff_prism_std_krb_ocl .
done
cat header.csv result.csv > result_GTX570M_QSS.csv
rm header.csv result.csv
