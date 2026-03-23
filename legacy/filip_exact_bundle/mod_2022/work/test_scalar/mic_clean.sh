export MOD_FEM_DIR=/home/fkruzel/Kod/mod_2022/
export MOD_FEM_ARCH=mic_test11
cd $MOD_FEM_DIR/src
make -f Makefile_explicit deep_clean
make -f Makefile_explicit clean
cd $MOD_FEM_DIR/work/test_scalar/
