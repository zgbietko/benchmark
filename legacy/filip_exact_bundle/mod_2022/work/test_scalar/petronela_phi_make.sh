export MOD_FEM_DIR=/home/fkruzel/Kod/mod_2015/
export MOD_FEM_ARCH=petronela_ocl_test_phi
cd $MOD_FEM_DIR/src
#svn up
make -f Makefile_explicit
make -f Makefile_explicit conv_diff_prism_std_krb_ocl
cd $MOD_FEM_DIR/work/test_scalar/
