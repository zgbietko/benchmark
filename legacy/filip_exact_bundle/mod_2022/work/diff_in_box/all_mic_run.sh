sh all_mic_clean.sh
sh all_mic_make.sh
export KMP_BLOCKTIME=0
export OMP_WAIT_POLICY=PASSIVE
export OMP_NESTED=TRUE
export VAL=1
#export KMP_AFFINITY=verbose,none
for((i=0;$i<$VAL;i++)) ; do
#/home/fkruzel/Kod/mod_2015/bin/mic_laplace/MFEM_conv_diff_prism_std_krb_mic .
#/home/fkruzel/Kod/mod_2015/bin/mic_laplace10/MFEM_conv_diff_prism_std_krb_mic .
#/home/fkruzel/Kod/mod_2015/bin/mic_laplace01/MFEM_conv_diff_prism_std_krb_mic .
/home/fkruzel/Kod/mod_2015/bin/mic_laplace11/MFEM_conv_diff_prism_std_krb_mic .
done
cat header.csv result.csv > result_laplace.csv
rm header.csv
rm result.csv

#amplxe-cl -collect-with runsa -knob event-config=CPU_CLK_THREAD_UNHALTED.REF_XCLK:sa=2000003,CPU_CLK_UNHALTED.THREAD:sa=2000003,FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.X87:sa=2000003,INST_RETIRED.ANY:sa=2000003,SIMD_FP_256.PACKED_DOUBLE:sa=2000003,SIMD_FP_256.PACKED_SINGLE:sa=2000003 -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace11/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box
:'
amplxe-cl -collect-with runsa -knob event-config=FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.SSE_PACKED_SINGLE:sa=2000003,FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE:sa=2000003,FP_COMP_OPS_EXE.X87:sa=2000003,SIMD_FP_256.PACKED_DOUBLE:sa=2000003,SIMD_FP_256.PACKED_SINGLE:sa=2000003,MEM_UOPS_RETIRED.ALL_LOADS:sa=2000003,MEM_UOPS_RETIRED.ALL_LOADS_PS:sa=2000003,MEM_UOPS_RETIRED.ALL_STORES:sa=2000003,MEM_UOPS_RETIRED.ALL_STORES_PS:sa=2000003 -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

amplxe-cl -collect-with runsa -knob event-config=FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.SSE_PACKED_SINGLE:sa=2000003,FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE:sa=2000003,FP_COMP_OPS_EXE.X87:sa=2000003,SIMD_FP_256.PACKED_DOUBLE:sa=2000003,SIMD_FP_256.PACKED_SINGLE:sa=2000003,MEM_UOPS_RETIRED.ALL_LOADS:sa=2000003,MEM_UOPS_RETIRED.ALL_LOADS_PS:sa=2000003,MEM_UOPS_RETIRED.ALL_STORES:sa=2000003,MEM_UOPS_RETIRED.ALL_STORES_PS:sa=2000003 -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace01/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

amplxe-cl -collect-with runsa -knob event-config=FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.SSE_PACKED_SINGLE:sa=2000003,FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE:sa=2000003,FP_COMP_OPS_EXE.X87:sa=2000003,SIMD_FP_256.PACKED_DOUBLE:sa=2000003,SIMD_FP_256.PACKED_SINGLE:sa=2000003,MEM_UOPS_RETIRED.ALL_LOADS:sa=2000003,MEM_UOPS_RETIRED.ALL_LOADS_PS:sa=2000003,MEM_UOPS_RETIRED.ALL_STORES:sa=2000003,MEM_UOPS_RETIRED.ALL_STORES_PS:sa=2000003 -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace10/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box
'

amplxe-cl -collect-with runsa -knob event-config=FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.SSE_PACKED_SINGLE:sa=2000003,FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE:sa=2000003,FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE:sa=2000003,FP_COMP_OPS_EXE.X87:sa=2000003,SIMD_FP_256.PACKED_DOUBLE:sa=2000003,SIMD_FP_256.PACKED_SINGLE:sa=2000003,MEM_UOPS_RETIRED.ALL_LOADS:sa=2000003,MEM_UOPS_RETIRED.ALL_LOADS_PS:sa=2000003,MEM_UOPS_RETIRED.ALL_STORES:sa=2000003,MEM_UOPS_RETIRED.ALL_STORES_PS:sa=2000003,MEM_LOAD_UOPS_MISC_RETIRED.LLC_MISS_PS:sa=100007,MEM_LOAD_UOPS_RETIRED.LLC_MISS:sa=100007 -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace11/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box



#amplxe-cl -collect hotspots -mrte-mode=managed -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect hotspots -mrte-mode=managed -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace10/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect hotspots -mrte-mode=managed -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace01/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect hotspots -mrte-mode=managed -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace11/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect concurrency -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect concurrency -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace10/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect concurrency -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace01/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect concurrency -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace11/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect locksandwaits -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect locksandwaits -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace10/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect locksandwaits -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace01/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

#amplxe-cl -collect locksandwaits -app-working-dir /home/fkruzel/Kod/mod_2015/work/diff_in_box --start-paused -- /home/fkruzel/Kod/mod_2015/bin/mic_laplace11/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/diff_in_box

