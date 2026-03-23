export MOD_FEM_DIR=/home/fkruzel/Kod/mod_2015/
export MOD_FEM_ARCH=mic_test
:'
cd $MOD_FEM_DIR/src
make -f Makefile_explicit
make -f Makefile_explicit conv_diff_prism_std_krb_mic
cd $MOD_FEM_DIR/work/test_scalar/
make
export MOD_FEM_ARCH=mic_test01
cd $MOD_FEM_DIR/src
make -f Makefile_explicit
make -f Makefile_explicit conv_diff_prism_std_krb_mic
cd $MOD_FEM_DIR/work/test_scalar/
make
export MOD_FEM_ARCH=mic_test10
cd $MOD_FEM_DIR/src
make -f Makefile_explicit
make -f Makefile_explicit conv_diff_prism_std_krb_mic
cd $MOD_FEM_DIR/work/test_scalar/
make
'
export MOD_FEM_ARCH=mic_test11
cd $MOD_FEM_DIR/src
make -f Makefile_explicit
make -f Makefile_explicit conv_diff_prism_std_krb_mic
cd $MOD_FEM_DIR/work/test_scalar/
make

