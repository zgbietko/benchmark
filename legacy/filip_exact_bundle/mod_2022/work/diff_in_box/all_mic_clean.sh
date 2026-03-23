export MOD_FEM_DIR=/home/fkruzel/Kod/mod_2015/
export MOD_FEM_ARCH=mic_laplace
:'
cd $MOD_FEM_DIR/src
make -f Makefile_explicit deep_clean
make -f Makefile_explicit clean
cd $MOD_FEM_DIR/work/diff_in_box/
export MOD_FEM_ARCH=mic_laplace10
cd $MOD_FEM_DIR/src
make -f Makefile_explicit deep_clean
make -f Makefile_explicit clean
cd $MOD_FEM_DIR/work/diff_in_box/
export MOD_FEM_ARCH=mic_laplace01
cd $MOD_FEM_DIR/src
make -f Makefile_explicit deep_clean
make -f Makefile_explicit clean
cd $MOD_FEM_DIR/work/diff_in_box/
'
export MOD_FEM_ARCH=mic_laplace11
cd $MOD_FEM_DIR/src
make -f Makefile_explicit deep_clean
make -f Makefile_explicit clean
cd $MOD_FEM_DIR/work/diff_in_box/
