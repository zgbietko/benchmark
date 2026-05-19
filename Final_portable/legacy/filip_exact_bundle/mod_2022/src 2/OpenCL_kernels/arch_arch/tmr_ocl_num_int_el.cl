#ifdef cl_khr_fp64
    #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#elif defined(cl_amd_fp64)
    #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#else
    #error "Double precision floating point not supported by OpenCL implementation."
#endif

#define SCALAR double

#define NR_EXEC_PARAMS 32  // size of array with execution parameters
    // here: the smallest work-group for reading data is selected
    // exec_params are read from global to shared memory and used when needed
    // if shared memory resources are scarce this can be reduced

#define EL_DATA_IN_SIZE_PER_ELEM 32 // must be: EL_DATA_LOC_SIZE > EL_DATA_IN_SIZE_PER_ELEM !!!!
#define EL_DATA_LOC_SIZE 32 // must be: EL_DATA_LOC_SIZE > EL_DATA_IN_SIZE_PER_ELEM !!!!

kernel void tmr_ocl_num_int_el(
  __global int* execution_parameters,
  __global SCALAR* gauss_dat, // integration points data of elements having given p
  __global SCALAR* shape_fun_ref, // shape functions on a reference element
  __global SCALAR* el_data_in, // data for integration of NR_ELEMS_PER_KERNEL elements
  __global SCALAR* stiff_mat_out, // result of integration of NR_ELEMS_PER_KERNEL elements
  __local SCALAR *part_of_stiff_mat,
  __local SCALAR *num_shape_workspace,
  __local SCALAR *pde_coeff_workspace
){

// ASSUMPTION: one element = one work_group (there may be several work_groups per
//             compute unit and of course per device)
//             one work_group = one or more elements processed in a sequence

  int i,j,k;
  const int group_id = get_group_id(0);
  const int thread_id = get_local_id(0);

  __local float el_data_loc[EL_DATA_LOC_SIZE]; // local data for integration of 1 element


  // 1. read to shared memory and possibly to registers execution parameters
  //    and global data (there may be e.g. some problem dependent parameters
  //    that we may send in exec_params and use in problem optimaized kernels)
  __local SCALAR exec_params[NR_EXEC_PARAMS]; // shared memory copy of execution parameters

  if(thread_id < NR_EXEC_PARAMS){

    exec_params[thread_id] = execution_parameters[thread_id];

  }

  // e.g. for NS_SUPG we may store ref_dens, delta_t, alpha, dyn_visc etc.

  // if all reference shape functions at all integration points fit into shared memory
  // read shape functions (and possibly their derivatives) for the reference element 

  // if coordinates and weights for all integration points fit into shared memory
  // read them here from global memory (or possibly constant memory)


  // 2. Based on:
  //    - thread_id
  //    - group_id
  //    - work_group_size
  //    - num_dofs = num_shap * nreq
  //    compute the set of locations (or a single location) in SM that is updated by the thread
  //    i.e.
  //    idof[iter]=...;ieq[iter]=...;jdof[iter]=...;jeq[iter]=...
  //    or
  //    idof[iter]=...;idim[iter]=...;jdof[iter]=...;jdim[iter]=...
  //    or even 
  //    idof[iter]=...;ieq[iter]=...;idim[iter]=...;jdof[iter]=...;jeq[iter]=...;jdim[iter]=...


  // 3. loop over elements processed by work_group 
  int ielem;
  for(ielem = 0; ielem < nr_elems_per_work_group; ielem++){

    // read all necessary element data i.e.:
    // - geo_dofs, - for JAC versions
    // - detadx and vol (at all integration points) - for NOJAC versions
    // - vol, base_dphix, base_dphiy, base_dphiz  (at all integration points) - for GL_DER version
    // - for each method read coefficients constant for the whole element (e.g. solution dofs)
    if(thread_id < EL_DATA_IN_SIZE_PER_ELEM){
      el_data_loc[thread_id] = el_data_in[(group_id*nr_elems_per_work_group + ielem)
					  *EL_DATA_IN_SIZE_PER_ELEM + thread_id];
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);


    // 4. loop over parts of stiffness matrix (if necessary - for low order elements
    //    we prefer not to make this loop (i.e. we prefer to update several entries
    //    by each thread in the inner-most loop)
    int ipart;
    for(ipart = 0; ipart < nr_parts_of_stiff_mat; ipart++){


      // zero part of global stiffness matrix (or the whole SM)


      int igauss;
      // 5. loop over gauss points - we make it external to loops over dofs, eqs, dims
      for(igauss = 0; igauss < ngauss; igauss++){


	// read the values of shape functions to shared memory from global memory
	// for a given point (if they are not yet there)

	// read or compute necessary Jacobian values 
	// i.e.
	// for JAC versions:
	//   - read gauss data from shared mamory to registers 
	//   - compute geo_dofs at point
	//   - compute Jacobian terms (performing inverse)
	// for NOJAC versions:
	//   - read Jacobian terms from shared memory
	// for JAC and NOJAC versions:
	//   - compute derivatives of shape functions at integration point
	// for GL_DER versions:
	//   - read  derivatives of shape functions at integration point (and vol)

	// 6. loop over entries per thread
	for(iiter = 0; iiter < nr_blocks_per_thread; iiter++ ){


	  //    use arrays:
	  //    idof[iter]=...;ieq[iter]=...;jdof[iter]=...;jeq[iter]=...
	  //    or
	  //    idof[iter]=...;idim[iter]=...;jdof[iter]=...;jdim[iter]=...
	  //    or even 
	  //    idof[iter]=..;ieq[iter]=..;idim[iter]=..;jdof[iter]=..;jeq[iter]=..;jdim[iter]=..

	  // read proper values of shape functions (and their derivatives)
	  // read proper values of coefficient

	  // There are many choices for selecting loop order and parallelization options -
	  // each choice may lead to different performance and be related with different
	  // optimal matrix layout in memory

	  // A. if performing update for a single entry and single idim,jdim combination:
	  //   for REG variants:
	  //     reg_value += coeff*shp*shp*vol
	  //     !!! values must be reduced for all threads having the same idof,jdofs,ieq,jeq
	  //     !!! but different idim,jdim combinations (multiplication by vol may be postponed)
	  //   for SHM variants:
	  //     SM_part[...iiter...thread_id...] += coeff*shp*shp*vol
	  //     !!! values must be reduced for all threads having the same idof,jdofs,ieq,jeq
	  //     !!! but different idim,jdim combinations (multiplication by vol may be postponed)

	  // B. if performing update for a single entry and all idim,jdim combinations:
	  //   for REG variants:
	  //     reg_value += (coeff..*shp.*shp. + coeff..*shp.*shp. + etc.) * vol
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!
	  //   for SHM variants:
	  //     SM_part[...iiter...thread_id...] +=
	  //                  (coeff..*shp.*shp. + coeff..*shp.*shp. + etc.) * vol
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!


	  // C. if performing update for a set of entries with the same idof,jdof combination, 
	  // different ieq,jeq combinations and single idim,jdim combination:
	  //   for REG variants:
	  //     temp = shp*shp*vol
	  //     reg_value.. += coeff..*tmp
	  //     there must be several reg_values for different ieq,jeq combinations
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!
	  //     !!! values must be reduced for all threads having the same idof,jdofs,ieq,jeq
	  //     !!! but different idim,jdim combinations (multiplication by vol may be postponed)
	  //   for SHM variants:
	  //     temp = shp*shp*vol
	  //     SM_part[...iiter...thread_id...ieq...jeq] += coeff..*temp
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!
	  //     !!! values must be reduced for all threads having the same idof,jdofs,ieq,jeq
	  //     !!! but different idim,jdim combinations (multiplication by vol may be postponed)


	  // D. if performing update for a set of entries with the same idof,jdof combination, 
	  // different ieq,jeq combinations and  all idim,jdim combination:
	  //   for REG variants:
	  //     reg_value += (coeff..*shp.*shp. + coeff..*shp.*shp. + etc.) * vol
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!
	  //     there must be several reg_values for different ieq,jeq combinations
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!
	  //   for SHM variants:
	  //     SM_part[...iiter...thread_id...ieq...jeq] +=
	  //                  (coeff..*shp.*shp. + coeff..*shp.*shp. + etc.) * vol
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!


	  // E. if performing update for a set of entries with the same ieq,jeq combination, 
	  // different idofs,jdofs combinations and single idim,jdim combination:
	  //   for REG variants:
	  //     reg_value += coeff*shp*shp*vol
	  //     there must be several reg_values for different idofs,jdofs combinations
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!
	  //     !!! values must be reduced for all threads having the same idof,jdofs,ieq,jeq
	  //     !!! but different idim,jdim combinations (multiplication by vol may be postponed)
	  //   for SHM variants:
	  //     SM_part[...iiter...thread_id...idofs...jdofs] += coeff*shp*shp*vol
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!
	  //     !!! values must be reduced for all threads having the same idof,jdofs,ieq,jeq
	  //     !!! but different idim,jdim combinations (multiplication by vol may be postponed)



	  // F. if performing update for a set of entries with the same ieq,jeq combination, 
	  // different idofs,jdofs combinations and all idim,jdim combinations:
	  //   for REG variants:
	  //     reg_value += (coeff..*shp.*shp. + coeff..*shp.*shp. + etc.) * vol
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!
	  //     there must be several reg_values for different idofs,jdofs combinations
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!
	  //   for SHM variants:
	  //     SM_part[...iiter...thread_id...idofs...jdofs] +=
	  //                  (coeff..*shp.*shp. + coeff..*shp.*shp. + etc.) * vol
	  // IF POSSIBLE VECTOR OPERATIONS SHOULD BE USED !!!!!!!!!!!!!!!


	  // G. if performing update for a set of entries with different ieq,jeq combinations, 
	  // different idofs,jdofs combinations and single idim,jdim combination:

	  // H. if performing update for a set of entries with different ieq,jeq combinations, 
	  // different idofs,jdofs combinations and  all idim,jdim combinations:

	  // and many more possible if idofs are treated separately from jdofs and ieq from jeq
	  // not mentioning idim and jdim


	  // 7. UPDATE LOAD VECTOR...
	  
	} // end loop over iiter/entries/blocks per thread
	
      } // end loop over integration points
      
      // 8. rewrite part of SM (or the whole matrix and LV) 
      //    !!! the format should enable fast GPU calculations and fast assembly
      //    !!! by solver 
      if(thread_id < NR_EXEC_PARAMS){
	
	stiff_mat_out[thread_id] = exec_params[thread_id];
	
      }
      
    } // end loop over parts of stiff mat
    
  } // end loop over elements



  if(thread_id==0 && group_id==0){

    // possibly write some debug info...

      //stiff_mat_out[...] = ...;


  }



};
