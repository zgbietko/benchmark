config_script="labtopKM_config_cbp.sh"

function do_cmake_generation() {
	res="$(./$config_script)"
	echo " Altering linker input"
	linker_files="$(find ./ -name 'link.txt')"
	echo " Found linker files: $linker_files. Now updateing "
	old="libmkl_core.a -Wl,--end-group"
	new="libmkl_core.a  -lm -ldl -Wl,--end-group"
	replaced="$(sed -i "s/$old/$new/g" $linker_files)"	
        echo "Creating lunch script for mpi"
        lsmpi="$(echo mpirun -n 2 xterm -hold -e gdb --args pdd_ns_supg/Debug/MOD_FEM_ns_supg_hybrid_std ~/code/examples/test_LDC/nas_20x20 >> run_mpi_gdb.sh)"
        lsmpi="$(echo mpirun -n 2 xterm -hold -e valgrind pdd_ns_supg/Debug/MOD_FEM_ns_supg_hybrid_std ~/code/examples/test_LDC/nas_20x20 >> run_mpi_val.sh)"
        lsmpi="$(echo mpirun -n 2 xterm -hold -e valgrind --vgdb-error=0 pdd_ns_supg/Debug/MOD_FEM_ns_supg_hybrid_std ~/code/examples/test_LDC/nas_20x20 >> run_mpi_val_gdb.sh)"
}


if [ "$1" = "" ]; then
	echo " Missing folder name."
	me=`basename $0`
	echo " usage: $me folder_name_for_binaries"
else
	echo " Clearing $1"
	echo "$(rm -rf $1/*)"
	echo " Copying $config_script into $1"
	echo "$(cp $config_script $1/$config_script)"
	echo "Entering $1"
	cd $1
	echo "$(pwd)"
	echo " Running configuration script $config_script"
	do_cmake_generation
        echo "Done."
fi
