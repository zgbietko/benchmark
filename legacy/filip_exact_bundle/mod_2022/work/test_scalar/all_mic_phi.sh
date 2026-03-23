export KMP_BLOCKTIME=0
export OMP_WAIT_POLICY=PASSIVE
export OMP_NESTED=TRUE
export VAL=1
export KMP_AFFINITY="granularity=fine,compact"
export OFFLOAD_REPORT=3
export MIC_USE_2MB_BUFFERS=2M
sh all_mic_clean.sh
sh all_mic_phi_make.sh
/home/fkruzel/Kod/mod_2015/bin/mic_test/MFEM_conv_diff_prism_std_krb_mic .
/home/fkruzel/Kod/mod_2015/bin/mic_test10/MFEM_conv_diff_prism_std_krb_mic .
/home/fkruzel/Kod/mod_2015/bin/mic_test01/MFEM_conv_diff_prism_std_krb_mic .
/home/fkruzel/Kod/mod_2015/bin/mic_test11/MFEM_conv_diff_prism_std_krb_mic .
cat header.csv result.csv > result_test.csv
rm result.csv
rm header.csv

amplxe-cl -target-system mic-host-launch:0 -collect advanced-hotspots -- /home/fkruzel/Kod/mod_2015/bin/mic_test/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect advanced-hotspots -- /home/fkruzel/Kod/mod_2015/bin/mic_test10/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect advanced-hotspots -- /home/fkruzel/Kod/mod_2015/bin/mic_test01/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect advanced-hotspots -- /home/fkruzel/Kod/mod_2015/bin/mic_test11/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect general-exploration -knob enable-vpu-metrics=true -- /home/fkruzel/Kod/mod_2015/bin/mic_test/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect general-exploration -knob enable-vpu-metrics=true -- /home/fkruzel/Kod/mod_2015/bin/mic_test10/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect general-exploration -knob enable-vpu-metrics=true -- /home/fkruzel/Kod/mod_2015/bin/mic_test01/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect general-exploration -knob enable-vpu-metrics=true -- /home/fkruzel/Kod/mod_2015/bin/mic_test11/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect bandwidth -- /home/fkruzel/Kod/mod_2015/bin/mic_test/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect bandwidth -- /home/fkruzel/Kod/mod_2015/bin/mic_test10/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect bandwidth -- /home/fkruzel/Kod/mod_2015/bin/mic_test01/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

amplxe-cl -target-system mic-host-launch:0 -collect bandwidth -- /home/fkruzel/Kod/mod_2015/bin/mic_test11/MFEM_conv_diff_prism_std_krb_mic /home/fkruzel/Kod/mod_2015/work/test_scalar

