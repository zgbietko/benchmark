sh cuda_clean.sh
sh cuda_make.sh
sh cuda_sqs_make.sh
sh cuda_ssq_make.sh
cp /home/fkruzel/Kod/mod_2015/bin/dell_cuda_test/MFEM_conv_diff_prism_std_krb_cuda ./exec/prism_QSS_STIFF.exe
cp /home/fkruzel/Kod/mod_2015/bin/dell_cuda_test/MFEM_conv_diff_prism_std_krb_cuda_sqs ./exec/prism_SQS_STIFF.exe
cp /home/fkruzel/Kod/mod_2015/bin/dell_cuda_test/MFEM_conv_diff_prism_std_krb_cuda_ssq ./exec/prism_SSQ_STIFF.exe

