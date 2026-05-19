/******************************************************************************
File uth_intf.h - interface to utility procedures (and macros)
                  1. general purpose utilities (implementation in uts_util.c)
                  2. time measurments (implementation system dependent !!!)

Contains declarations of constants and interface routines:

General purpose utilities:

  utr_ctrl_c_signal_handler(int param) - to catch kill signals
  utr_set_interactive - to set up names for interactive input and output
        that control the run (possibly stdin and/or stdout for on-line control)
  utr_initialize_mesh - to initialize mesh of a specified type
  utr_initialize_field - to initialize field of a specified type
  utr_write_paraview_std_lin - to dump data in Paraview format (for std_lin only)
  utr_write_paraview_partmesh - to dump mesh in Paraview format with partition info
  !utr_export_mesh - to export mesh of a specified type in a specified format
  !utr_export_field - to export field of a specified type in a specified format
  utr_create_patches - to create patches of elements containing a given node
                      (uses only mesh data - can be the same for all fields)
  utr_create_patches_small - the same as above but uses lists of elements nodes 
                             instead of lists of elements dof entities
  utr_recover_derivatives - to recover deirvatives using patches of elements
                      (can be called separately for each field on the same mesh) 
  utr_recover_derivatives_small-to recover derivatives using patches of elements
    (does not use apr_get_stiff_mat_data for getting lists of element nodes)
  utr_adapt - to enforce default adaptation strategy for SINGLE problem/field
  utr_test_refinements - to perform sequence of random refinements/unrefinements
  utr_manual_refinement - to perform manual refinement/unrefinement
  utr_get_list_ent - to return the list of:
                      1. integration entities - entities
                         for which stiffness matrices and load vectors are
                         provided by the FEM code to the solver module,
                      2. DOF entities - entities with which there are dofs 
                         associated by the given approximation
  utr_create_assemble_stiff_mat - to create element stiffness matrices
                                 and assemble them to the global SM
  utr_average_sol_el - to compute the average of solution over element

  utr_dvector - to allocate a double vector: name[0..ncom-1]:
                  name=ut_dvector(ncom,error_text) 
  utr_ivector - to allocate an integer vector: name[0..ncom-1]:
                  name=ut_ivector(ncom,error_text) 
  utr_d_zero - to zero a double vector of length Num
  utr_chk_list - to check whether Num is on the list List
	with length Ll
  utr_put_list - to put Num on the list List with length Ll 
	(filled with numbers and zeros at the end)
  utr_mat3_inv - to invert a 3x3 matrix (stored as a vector!)
  utr_vec3_add - to compute vector product of 3D vectors
  utr_vec3_subst - to compute sum of 3D vectors
  utr_vec3_prod - to compute difference of 3D vectors
  utr_vec3_mxpr - to compute mixed vector product of 3D vectors
  utr_vec3_length - to compute length of a 3D vector
  utr_mat3vec - to compute matrix vector product in 3D space
  utr_mat3mat - to compute matrix matrix product in 3D space
	(all matrices are stored by rows as vectors!)
  utr_mat_det - to compute determinant of a matrix
  utr_skip_rest_of_line - to allow for comments in input files
  utr_fprintf_double - to write double number to file with specified accuracy 
                     (in the number of significant digits)
  utr_insert_sort - to sort an array by insertion (for small integer arrays)
  utr_heap_sort - to heap-sort an array (for large double precision arrays)

Time measurments:
  time_C - (C standard procedure) to return time in seconds from some date

  time_init   - to initiate time measurments
  time_clock  - to return wall clock time from initialization
  time_CPU    - to return CPU  time from initialization
  time_print  - to print CPU and wall clock time from initialization

Input Output routines (IO):
 utr_io_read_mesh - to read mesh from given filename(s)


------------------------------  			
History:        
      10.2012 - Kazimierz.Michalik@agh.edu.pl
      08.2012 - Kamil Wachala, kamil.wachala@gmail.com
      2011 - Kazimierz Michalik, kamich@agh.edu.pl
      02.2002 - Krzysztof Banas, kbanas@pk.edu.pl
******************************************************************************/

#ifndef _uth_intf_
#define _uth_intf_

/** standard macro for max and min and abs */
#define utm_max(x,y) ((x)>(y)?(x):(y))
#define utm_min(x,y) ((x)<(y)?(x):(y))
#define utm_abs(x)   ((x)<0?-(x):(x))

#ifndef UTM_SAFE_FREE_PTR
# define UTM_SAFE_FREE_PTR(ptr) if((ptr)!=NULL) { free((ptr));(ptr)=NULL; };
#endif

#ifndef UTM_SAFE_FREE_PTR_ARRAY
# define UTM_SAFE_FREE_PTR_ARRAY(p_ar,n) if((p_ar)!=NULL){int i=0; for(;i<n;++i){UTM_SAFE_FREE_PTR(p_ar[i]);}};
#endif

#ifdef __cplusplus
extern "C"
{
#endif


/** @defgroup UTM Utilities
 *
 *  @{
 */

/** definition of type utt_patches - useful e.g. for error estimation */
#define UTC_MAXEL_PATCH 100 // maximal number of elements in the patch
#define UTC_MAXNO_PATCH 60 // maximal number of nodes in the patch

/*struct for patches - to recover derivatives by uts_recover_derivatives*/
typedef struct {
	int nr_elems;
	int elems[UTC_MAXEL_PATCH];
	int nr_nodes;
	int nodes[UTC_MAXNO_PATCH];
	double *deriv;
} utt_patches;

/**--------------------------------------------------------
  utr_ctrl_c_signal_handler(int param) - to catch kill signals
----------------------------------------------------------*/
  extern int utv_SIGINT_not_caught;
  void utr_ctrl_c_signal_handler(int param);


/**--------------------------------------------------------
  utr_set_interactive - to set up pointers for interactive input and output
         that control the run (possibly stdin and/or stdout for on-line control)
----------------------------------------------------------*/
int utr_set_interactive(  /** returns: >0 - success code, <0 - error code */
  char* Workdir, // working directory
  int Argc, // passed from maine
  char **Argv,
  FILE **Interactive_input,
  FILE **Interactive_output
);

/**--------------------------------------------------------
utr_initialize_mesh - to initialize mesh of a specified type
---------------------------------------------------------*/
int utr_initialize_mesh( /** returns mesh_id */
  FILE *Interactive_output, /** file or stdout to write messages */
  const char* Work_dir, // path to working directory						  
  char Mesh_type, /** letter symbol denoting mesh_type (j, p, t or h) */
  char* Mesh_file /** mesh file name - conforming to naming convention */
  );

/**--------------------------------------------------------
utr_initialize_field - to initialize field of a specified type
---------------------------------------------------------*/
int utr_initialize_field(  /** returns: >0 - field ID, <0 - error code */
  FILE *Interactive_output, /** file or stdout to write messages */
  char Field_type, /** options: s - standard, d - discontinuous */
  char Control,	 /** in: control variable: */
                 /**      z - to initialize the field to zero */
                 /**      r - to read field values from the file */
                 /**      i - to initialize the field using function */
                 /**                 provided by the problem dependent module */
  int Mesh_id,	 /** in: ID of the corresponding mesh */
  int Nreq,	 /** in: number of equations - solution vector components */
  int Nr_sol,	 /** in: number of solution vectors for each dof entity */
  int Pdeg,      /** in: degree of approximating polynomial */
  char *Filename,/** in: name of the file to read approximation data */
  double (*Fun_p)(int, double*, int) /** pointer to function that provides */
		 /** problem dependent initial condition data */
  );

/**--------------------------------------------------------
utr_write_paraview_std_lin - to dump data in Paraview format (for std_lin only)
---------------------------------------------------------*/
int utr_write_paraview_std_lin(
  int Mesh_id,       /** in: ID of the mesh */
  int Field_id,      /** in: ID of the field */
  char *Filename     /** in: name of the file to write data */
  );
  
/**--------------------------------------------------------
/// utr_write_paraview_mesh - to dump mesh in Paraview format
---------------------------------------------------------*/
int utr_write_paraview_mesh(int Mesh_id, FILE  *File);
  
/**--------------------------------------------------------
/// utr_write_paraview_partmesh - to dump mesh in Paraview format includiong partition info
---------------------------------------------------------*/
void utr_write_paraview_partmesh(
  int Mesh_id,
  FILE *File, 
  int *Parts /** partition vector for the elements of the mesh */
);

/**--------------------------------------------------------
/// utr_write_paraview_field - to dump field in Paraview format
---------------------------------------------------------*/
int utr_write_paraview_field(
  int Field_id, 
  FILE *File,
  int* Dofs_write // dofs to write: dofs_write[0] - number of dofs
                   //                dofs_write[i] - IDs of dofs to write
							 );
/**--------------------------------------------------------
/// utr_write_paraview_bc - to dump boundary conditions 'field' in Paraview format
---------------------------------------------------------*/
int utr_write_paraview_bc(
  int Mesh_id,
  FILE *File
);  

/**--------------------------------------------------------
  utr_create_patches - to create patches of elements containing a given node
                      (uses only mesh data - can be the same for all fields)
---------------------------------------------------------*/
int utr_create_patches( /** returns: >0 - Nr_patches, <=0 - failure */
  int Problem_id,       /** in: problem data structure to be used */
  utt_patches **patches_p /** in/out - array of patches for real nodes */
);

/**--------------------------------------------------------
  utr_create_patches - to create patches of elements containing a given node
                      (uses only mesh data - can be the same for all fields)
---------------------------------------------------------*/
int utr_create_patches_small( /** returns: >0 - Nr_patches, <=0 - failure */
  int Problem_id,       /** in: problem data structure to be used */
  utt_patches **patches_p /** in/out - array of patches for real nodes */
);

/**--------------------------------------------------------
  utr_recover_derivatives - to recover derivatives using patches of elements
                      (can be called separately for each field on the same mesh) 
---------------------------------------------------------*/
int utr_recover_derivatives( /** returns: >0 - success, <=0 - failure */
  int Field_id,         /** in: field data structure to be used */
  int Sol_vec_id,       /*in: which solution vector to take into account */
  int Nr_patches,
  utt_patches *patches /** in - array of patches for real nodes */
          /** out: array of patches with computed derivatives for ALL nodes */
  );

/**--------------------------------------------------------
  utr_recover_derivatives_small-to recover derivatives using patches of elements
    (does not use apr_get_stiff_mat_data for getting lists of element nodes)
                      (can be called separately for each field on the same mesh) 
---------------------------------------------------------*/
int utr_recover_derivatives_small( /** returns: >0 - success, <=0 - failure */
  int Field_id,         /** in: field data structure to be used */
  int Sol_vec_id,       /*in: which solution vector to take into account */
  int Nr_patches,
  utt_patches *patches /** in - array of patches for real nodes */
          /** out: array of patches with computed derivatives for ALL nodes */
  );

/**--------------------------------------------------------
  utr_adapt - to enforce default adaptation strategy for SINGLE problem/field
---------------------------------------------------------*/
extern int utr_adapt( /** returns: >0 - success, <=0 - failure */
  int Problem_id,       /** in: problem data structure to be used */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );

/**--------------------------------------------------------
utr_test_refinements - to perform sequence of random refinements/unrefinements
---------------------------------------------------------*/
int utr_test_refinements(  /** returns: >0 - success, <=0 - failure */
  int Problem_id,       /** in: problem data structure to be used */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );

/**--------------------------------------------------------
utr_manual_refinement - to perform manual refinement/unrefinement
---------------------------------------------------------*/
int utr_manual_refinement(  /** returns: >0 - success, <=0 - failure */
  int Problem_id,       /** in: problem data structure to be used */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );

/**-----------------------------------------------------------
  utr_get_list_ent - to return the list of integration entities - entities
                         for which stiffness matrices and load vectors are
                         provided by the FEM code to the solver module,
                         and DOF entities - entities with which there are dofs 
                         associated by the given approximation 
------------------------------------------------------------*/
int utr_get_list_ent( /** returns: >=0 - success code, <0 - error code */
  int Problem_id,     /** in:  problem (and solver) identification */
  int* Nr_int_ent,    /** out: number of integration entitites */
	/** GHOST ENTITIES HAVE NEGATIVE TYPE !!! */
  int** List_int_ent_type,/** out: list of types of integration entitites */
  int** List_int_ent_id,  /** out: list of IDs of integration entitites */
  int* Nr_dof_ent,    /** out: number of dof entities (entities with which there
		              are dofs associated by the given approximation) */
  int** List_dof_ent_type,/** out: list of types of integration entitites */
  int** List_dof_ent_id,  /** out: list of IDs of integration entitites */
  int** List_dof_ent_nrdofs,/** out: list of no of dofs for 'dof' entity */
  int* Nrdofs_glob,    /** out: global number of degrees of freedom (unknowns) */
  int* Max_dofs_per_dof_ent/** out: maximal number of dofs per dof entity */
  );

/**-----------------------------------------------------------
 utr_create_assemble_stiff_mat - to create element stiffness matrices
                                 and assemble them to the global SM
(implementation provided separately for different multithreaded environments)
------------------------------------------------------------*/
int utr_create_assemble_stiff_mat(
  int Problem_id, 
  int Level_id, 
  int Comp_type,         /** in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /** do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /** compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /** compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /** compute entries for sm and rhsv */
  int Nr_int_ent,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
				  );

/**--------------------------------------------------------
utr_average_sol_el - to compute the average of solution over element
----------------------------------------------------------*/
double utr_average_sol_el( /** returns: the average of solution over element */
        int Field_id,      /** in: data structure to be used  */
        int El          /** in: element number */
			   );

/**--------------------------------------------------------
utr_dvector - to allocate a double vector: name[0..ncom-1]:
                  name=ut_dvector(ncom,error_text) 
---------------------------------------------------------*/
double *utr_dvector( /** return: pointer to allocated vector */
	int ncom,  	/** in: number of components */
	char error_text[]/** in: error text to be printed */
	);

/**--------------------------------------------------------
utr_ivector - to allocate an integer vector: name[0..ncom-1]:
                  name=ut_ivector(ncom,error_text) 
---------------------------------------------------------*/
int *utr_ivector(    /** return: pointer to allocated vector */
	int ncom, 	/** in: number of components */
	char error_text[]/** in: error text to be printed */
	);


/**--------------------------------------------------------
utr_d_zero - to zero a double vector of length Num
---------------------------------------------------------*/
void utr_d_zero(
	double* Vec, 	/** in, out: vector to initialize */
	int Num		/** in: vector length */
	);

/**--------------------------------------------------------
utr_chk_list - to check whether Num is on the list List
	with length Ll
---------------------------------------------------------*/
int utr_chk_list(	/** returns: */
			/** >0 - position on the list */
            		/** 0 - not found on the list */
	int Num, 	/** number to be checked */
	int* List, 	/** list of numbers */
	int Ll		/** length of the list */
	);

/**--------------------------------------------------------
utr_put_list - to put Num on the list List with length Ll 
	(filled with numbers and zeros at the end)
---------------------------------------------------------*/
int utr_put_list( /** returns*/
		/**  >0 - position already occupied on the list */
             	/**  <0 - position at which put on the list */
            	/**   0 - list full, not found on the list */
	int Num, 	/** in: number to put on the list */
	int* List, 	/** in: list */
	int Ll		/** in: total list's lengths */
	);

/**--------------------------------------------------------
utr_mat3_inv - to invert a 3x3 matrix (stored as a vector!)
---------------------------------------------------------*/
double utr_mat3_inv(
	/** returns: determinant of matrix to invert */
	const double *mat,	/** matrix to invert */
	double *mat_inv	/** inverted matrix */
	);

//----------------------------------------------------------
//utr_vec3_add - to compute sum of 3D vectors
//----------------------------------------------------------

void utr_vec3_add(
	const double vec_a[3], //IN
	const double vec_b[3], //IN
	double vec_c[3]);	//OUT

//----------------------------------------------------------
//utr_vec3_subst - to compute the difference of 3D vectors
//----------------------------------------------------------

void utr_vec3_subst(
	const double vec_a[3],	//IN
	const double vec_b[3],	// IN
	double vec_c[3]);	// OUT

/**--------------------------------------------------------
utr_vec3_prod - to compute vector product of 3D vectors
---------------------------------------------------------*/
void utr_vec3_prod(
	const double* vec_a, 	/** in: vector a */
	const double* vec_b, 	/** in: vector b */
	double* vec_c	/** out: vector product axb */
	);

/**--------------------------------------------------------
utr_vec3_mxpr - to compute mixed vector product of 3D vectors
---------------------------------------------------------*/
double utr_vec3_mxpr( /** returns: mixed product [a,b,c] */
	const double* vec_a, 	/** in: vector a */
	const double* vec_b, 	/** in: vector b */
	const double* vec_c	/** in: vector c */
	);

/**--------------------------------------------------------
utr_vec3_length - to compute length of a 3D vector
---------------------------------------------------------*/
double utr_vec3_length(	/** returns: vector length */
	const double* vec	/** in: vector */
	);

/**--------------------------------------------------------
utr_mat3vec - to compute matrix vector product in 3D space
---------------------------------------------------------*/
void utr_mat3vec(
	const double* m1, 	/** in: matrix (stored by rows as a vector!) */
	const double* v1, 	/** in: vector */
	double* v2	/** out: resulting vector */
	);

/**--------------------------------------------------------
utr_mat3mat - to compute matrix matrix product in 3D space
	(all matrices are stored by rows as vectors!)
---------------------------------------------------------*/
void utr_mat3mat(
	const double* m1,	/** in: matrix */
	const double* m2,	/** in: matrix */
	double* m3	/** out: matrix m1*m2 */
	);


//----------------------------------------------------------
//utr_vec3_dot - to compute vector dot product of 3D vectors
//----------------------------------------------------------
double utr_vec3_dot(
					const double vec_a[3],
					const double vec_b[3]);

/**--------------------------------------------------------
  utr_mat_det - to compute determinant of a matrix
---------------------------------------------------------*/
double utr_mat_det(const double *m, int n,char store, double * det); 

/**--------------------------------------------------------
utr_skip_rest_of_line - to allow for comments in input files
---------------------------------------------------------*/
void utr_skip_rest_of_line(
			  FILE *Fp  /** in: input file */
			   );
                           
                           
/**------------------------------------------------------------
utr_fprintf_double -	prints double to stream in "%f" fashion
			with specified significant digits number
-------------------------------------------------------------*/
int utr_fprintf_double(
  FILE* stream,	// output stream
  int sdigits,	// number of significant digits to write - put 0 for
		// full accuracy (.12) written in "%g" fashion
  double value	// double to write
  );

/**--------------------------------------------------------
utr_heap_sort - to heap-sort an array (code taken from fortran...)
---------------------------------------------------------*/
void utr_heap_sort(
   int    *Ind_array,    /** in/out: index array for sorting */
                         /** Ind_array[0] - the length of both arrays */
   double *Val_array     /** in: array of values used for sorting */
   );

/**--------------------------------------------------------
utr_insert_sort - to sort an array by insertion (for small integer arrays)
---------------------------------------------------------*/
int utr_insert_sort(
		    int* A, // array
		    int p,  //first index
		    int k   // last index
);       

/**--------------------------------------------------------
  time_init   - to initiate time measurments
---------------------------------------------------------*/
void time_init();

/**--------------------------------------------------------
  time_C - (C standard procedure) to return time in seconds from some date
---------------------------------------------------------*/
double time_C();

/**--------------------------------------------------------
  time_clock  - to return wall clock time from initialization
---------------------------------------------------------*/
double time_clock();

/**--------------------------------------------------------
  time_CPU    - to return CPU  time from initialization
 ---------------------------------------------------------*/
double time_CPU();

/**--------------------------------------------------------
  time_print  - to print CPU and wall clock time from initialization
 ---------------------------------------------------------*/
void time_print();



//--------------------------------------------------------
//  utr_io_read_mesh - to read mesh with given filename(s)
//  Reads all mesh files from current working directory
//  that matches with passed regular expression Mesh_files_regex.
//  All files HAVE TO be the same type (ex. nas,dat,in etc.).
//  NOTE: single filename ex. "mesh.dat" is also a valid regular expression.  
//--------------------------------------------------------
int utr_io_read_mesh( 
  // returns: > 0 - number of files correctly readed; <=0 - error
  const int Mesh_id, // IN: id of mesh to read to
  const char * Working_dir, // IN: directory with mesh file(s)
  const char * Mesh_files_regex, // IN: regular expression pattern
  // NOTE: using regex 'POSIX grep' standard  
  const char Mesh_type // IN: type of mesh 
		      );


/**--------------------------------------------------------
utr_io_export_field - to export mesh of a specified type
---------------------------------------------------------*/
int utr_io_export_field(/** returns number of files exported */
    int Field_id,    /* in: field ID */
    int Nreq,        /* in: number of equations (scalar dofs) */
    int Select,      /* in: parameter to select written vectors */
    int Accuracy,    /* in: parameter specyfying accuracy - significant digits */
             /* (put 0 for full accuracy in "%g" format) */
    char *Filename   /* in: name of the file to write field data */
  );

/**--------------------------------------------------------
utr_io_export_mesh - to export mesh of a specified type
---------------------------------------------------------*/
int utr_io_export_mesh(/** returns number of files exported */
  FILE *Interactive_output, /** file or stdout to write messages */
  const int Mesh_id, // id of mesh to export
  char Mesh_type, /** symbol denoting \link  mmt_file_type  mesh_type \endlink  */
  char* Mesh_file /** mesh file name - conforming to naming convention */
  );


/**--------------------------------------------------------
utr_io_export_mesh - to display menu for exporting mesh
---------------------------------------------------------*/
int utr_menu_export_mesh(/** returns number of files exported */
  FILE *Interactive_output, /** file or stdout to write messages */
  FILE *Interactive_input,
  const int Mesh_id, /// id of mesh to export
  const char *Work_dir, /// Working directory
  char* Mesh_file /** mesh file name - conforming to naming convention */
  );


			  
int utr_io_write_img_to_pbm(// returns 0 if all ok.
        const char * Work_dir,  // Directory to write in.
        const char * Filename,  // without extension
        const char * Comment, // written into img file header, can be NULL if no comment.
        const int Width,  // >0
        const int Height, // >0
        const int Max_color_component_value,
        const int Magic_number, // {4,5,6}
        const unsigned char* Img_data, // pointer to the array with image data
        FILE* Interactive_output
        );
			  
			  
			  
			  
			  

int utr_io_write_mesh_info_to_PAM(int Mesh_id
        , const char *Work_dir, const char *Filename, const char *Comment, FILE *Interactive_output);

// utr_io_write_img_to_pam - to write data into PAM image file.
// http://en.wikipedia.org/wiki/Netpbm
// Allowed combinations of parameters:
// -----------------------------------------------------------
// TUPLTYPE           |MAXVAL |DEPTH  |comment
// -----------------------------------------------------------
// BLACKANDWHITE        1       1       special case of GRAYSCALE
// GRAYSCALE            2…65535	1       2 bytes per pixel for MAXVAL > 255
// RGB                  1…65535	3       6 bytes per pixel for MAXVAL > 255
// BLACKANDWHITE_ALPHA	1       2       2 bytes per pixel
// GRAYSCALE_ALPHA      2…65535	2       4 bytes per pixel for MAXVAL > 255
// RGB_ALPHA            1…65535	4       8 bytes per pixel for MAXVAL > 255s of parameters:
// -----------------------------------------------------------
int utr_io_write_img_to_pam( // returns 0 if all ok.
        const char * Work_dir,  // Directory to write in.
        const char * Filename,  // without extension
        const int Width,  // >0
        const int Height, // >0
        const int Depth,  // <1:4>
        const int Maxval, // <1:65535>,
        const char* TUPLTYPE,
        const char* Img_data, // pointer to the array with image data
        FILE* Interactive_output
        );

// utr_io_write_img_to_pnm - to write data into PNM image file.
// The Portable Bit/Grey/PixMap formats PBM, PGM, PPM.
// They are collectively referred to as PNM (Portable aNy Map).
// http://en.wikipedia.org/wiki/Netpbm_format
// Allowed combinations:
// -----------------------------------------------------------
// Type                 Magic number	Extension	Colors
// -----------------------------------------------------------
// Portable BitMap		P4	binary      .pbm        0–1 (black & white)
// Portable GrayMap		P5	binary      .pgm        0–255 (gray scale)
// Portable PixMap		P6	binary      .ppm        0–255 (RGB)
// -----------------------------------------------------------
int utr_io_write_img_to_pnm(// returns 0 if all ok.
        const char * Work_dir,  // Directory to write in.
        const char * Filename,  // without extension
        const char * Comment, // written into img file header, can be NULL if no comment.
        const int Width,  // >0
        const int Height, // >0
        const int Max_color_component_value, // see Colors above
        const int Magic_number, // {4,5,6}
        const char* Img_data, // pointer to the array with image data
        FILE* Interactive_output
        );


//////// UTR_IO_RESULT (file with infomrations)

typedef enum {
    RESULT_STEP = 0,
    RESULT_NON_LIN_STEP,
    RESULT_CUR_TIME,
    RESULT_DT,
    RESULT_CFL_MIN,
    RESULT_CFL_MAX,
    RESULT_CFL_AVG,
    RESULT_N_DOFS,
    RESULT_NORM_NS_SUPG,
    RESULT_NORM_HEAT,
    RESULT_OUT_FILENAME,
    RESULT_LAST
} utt_io_result_name;

int utr_io_result_set_filename(const char* Workdir,const char* Filename);

int utr_io_result_clear_columns();

int	utr_io_result_add_column(utt_io_result_name column);

int	utr_io_result_write_columns();

int utr_io_result_add_value(utt_io_result_name name, const char* value);

int utr_io_result_add_value_int(utt_io_result_name name, int value);

int utr_io_result_add_value_double(utt_io_result_name name, double value);

int utr_io_result_write_values_and_proceed();

///////////////////////////////////////////////////////


/** @} */ // end of group


#ifdef __cplusplus
}
#endif

#endif
