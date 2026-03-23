export MOD_FEM_DIR=/home/fkruzel/Kod/mod_2022/
export MOD_FEM_ARCH=mic_laplace11
cd $MOD_FEM_DIR/src
svn up
make -f Makefile_explicit
make -f Makefile_explicit conv_diff_prism_std_krb_mic
cd $MOD_FEM_DIR/work/diff_in_box/
