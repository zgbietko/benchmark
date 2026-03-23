include  $(MOD_FEM_DIR)/src/platform_files/make.$(MOD_FEM_ARCH)

pds_conv_diff_num_int_prism.$(MOD_FEM_ARCH).NORMAL.s:
	$(CC) -opt-assume-safe-padding -std=c99 -restrict -c $(CFL) $(INC) -S -fsource-asm $(MOD_FEM_DIR)/src/pdd_conv_diff/linear_solver_interface/mic/std_lin/pds_conv_diff_num_int_prism.c -o $@

	cp $(MOD_FEM_DIR)/bin/$(MOD_FEM_ARCH)/MFEM_conv_diff_prism* .
	find ./ -type f -name 'MFEM*' ! -name 'MFEM*.*' -exec mv '{}' '{}.$(MOD_FEM_ARCH).NORMAL' ';'

	cp $(MOD_FEM_DIR)/obj/$(MOD_FEM_ARCH)/pds_conv_diff_num_int_prism.optrpt .
	find ./ -type f -name 'pds_conv_diff_num_int_prism.optrpt' ! -name 'pds_conv_diff_num_int_prism.optrpt.*' -exec mv '{}' '{}.$(MOD_FEM_ARCH).NORMAL' ';'
