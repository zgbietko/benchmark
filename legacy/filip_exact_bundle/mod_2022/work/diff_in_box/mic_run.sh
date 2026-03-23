export KMP_BLOCKTIME=0
export OMP_WAIT_POLICY=PASSIVE
export OMP_NESTED=TRUE
export VAL=1
export KMP_AFFINITY="granularity=fine,compact"
export OFFLOAD_REPORT=3
export MIC_USE_2MB_BUFFERS=2M
/home/fkruzel/Kod/mod_2022/bin/mic_laplace11/MFEM_conv_diff_prism_std_krb_mic .
