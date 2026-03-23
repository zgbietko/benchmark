export MOD_FEM_DIR=/home/fkruzel/Kod/mod_2015/
export MOD_FEM_ARCH=dell_cuda_laplace
cd $MOD_FEM_DIR/src
#svn up
make -f Makefile_explicit
make -f Makefile_explicit conv_diff_prism_std_krb_cuda_ssq
cd $MOD_FEM_DIR/work/diff_in_box/
