///***********************************************************
/// File uts_io.cpp - utility routines for Input/Output logic.
/// To be used as IO proxy for all problem modules.

/// Contains definitions of routines:
///
/// Interface routines:
///   utr_io_read_mesh - to read mesh from given filename(s)
/// History:
///    Kazimierz.Michalik@agh.edu.pl - initial version
///
///*******************************************************

#ifndef _utr_io_intf_
#define _utr_io_intf_


/// For file io.
#include <fstream>
/// For portable filesystem
#include <boost/filesystem.hpp>
#include <boost/version.hpp>
/// For portable regex
/// (should be C++11 <regex>, but not yet supported)
#include <boost/regex.hpp>
/// For not making all stuff again
#include <iterator>
#include <vector>
#include <algorithm>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <sstream>

/// Includes:
#include "uth_log.h"
#include "uth_intf.h"
#include "uth_io_compression.h"
#include "mmh_intf.h"
#include "pch_intf.h"
#include "svnversion.h"
#include "aph_intf.h"

/// Internal types
/// pointer to a function to read/write specyfic file type.
typedef int (*mmpt_io_fptr)( /// returns: numer of readed elements
    const int Mesh_field_id, /// IN: mesh or field id
    const char * Filename);  /// IN: filename to I/O operation

using std::string;



/// Forward declarations of internal routines:
///---------------------------------------------------------
/// utr_io_gather_filenames_from_dir - to write all filenames into given vector
///---------------------------------------------------------
void utr_io_gather_filenames_from_dir(
    const char * Dir, /// IN: path of directory
    std::vector<string> & Filenames /// OUT: vector of filenames in Dir
	);

///---------------------------------------------------------
/// utr_io_filter_filename - to filter filenames with given regular expression
///---------------------------------------------------------
void utr_io_filter_filenames(
    std::vector<string>& Filenames, /// IN: list of filenames to filter
                             ///OUT: filtered filenames
    const char * Regex /// IN: regular expression pattern to filter with
	);

///---------------------------------------------------------
/// utr_io_get_reading_function - to return pointer to appropriate reading function
///  for given mesh type
///---------------------------------------------------------
mmpt_io_fptr utr_io_get_reading_function(/// returns: pointer to read function for Mesh_type
                                         const char Mesh_type /// IN: type of mesh filename
										 );


/// Definitions:
#ifdef __cplusplus
extern "C"
{
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// UTR_IO_RESULT (file with infomrations)

///typedef enum {
///    RESULT_STEP = 0,
///    RESULT_NON_LIN_STEP,
///    RESULT_CUR_TIME,
///    RESULT_DT,
///    RESULT_OUT_FILENAME,
///    RESULT_CFL_MIN,
///    RESULT_CFL_MAX,
///    RESULT_CFL_AVG,
///    RESULT_N_DOFS,
///    RESULT_NORM_NS_SUPG,
///    RESULT_NORM_HEAT,
///    RESULT_LAST
///} utt_io_result_name;

const char*               utv_io_result_column_names[RESULT_LAST]
= { "STEP",
    "NLIN",
    "  TIME",
    "DTIME",
    "CFL_MIN",
    "CFL_MAX",
    "CFL_AVG",
    "N_DOFS",
    "NORM_NS_SUPG",
    "NORM_HEAT",
    "FILES"
  };

const int utv_io_result_column_width[RESULT_LAST]
= {
  4,
  4,
  6,
  6,
  6,
  6,
  6,
  6,
  6,
  6,
};

std::vector<std::string> utv_io_result_record(RESULT_LAST);
std::vector<bool>   utv_io_result_columns(RESULT_LAST);
std::string         utv_io_result_filename("result_test.csv");

int utr_io_result_set_filename(const char* Workdir,const char* Filename)
{
    utv_io_result_filename = Workdir;
    utv_io_result_filename.append("/").append(Filename);
    return 0;
}

int utr_io_result_clear_columns()
{
    for(int i=0; i < RESULT_LAST; ++i) {
        utv_io_result_columns[i]=false;
        utv_io_result_record[i].clear();
    }
    return 0;
}

int	utr_io_result_add_column(utt_io_result_name column)
{
    utv_io_result_columns[column] = true;
    return 0;
}

int utr_io_result_add_value(utt_io_result_name name, const char* value)
{
    utv_io_result_record[name].append(value);
    return 0;
}

int utr_io_result_add_value_int(utt_io_result_name name, int value)
{
	std::ostringstream strs;
    strs << std::setw(utv_io_result_column_width[name]) << value;
	return utr_io_result_add_value(name, strs.str().c_str());
}

int utr_io_result_add_value_double(utt_io_result_name name, double value)
{
	std::ostringstream strs;
    strs << std::setw(utv_io_result_column_width[name]) << value;
	return utr_io_result_add_value(name, strs.str().c_str());
}

int	utr_io_result_write_columns()
{
    std::ofstream   file(utv_io_result_filename.c_str());
    file << "TIME    , ";
    for(int i=0; i < RESULT_LAST; ++i) {
        if(utv_io_result_columns[i]==true) {
            file << utv_io_result_column_names[i] << ", ";
        }
    }
    file << std::endl;
    file.close();
    return 0;
}

int utr_io_result_write_values_and_proceed()
{
    char date[10];
    time_t t = time(0);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", gmtime(&t));
    std::ofstream   file(utv_io_result_filename.c_str(), std::fstream::out | std::fstream::app);
    file << date << ", ";
    for(int i=0; i < RESULT_LAST; ++i) {
        if(utv_io_result_columns[i]==true) {
            file << utv_io_result_record[i] << ", ";
            utv_io_result_record[i].clear();
        }
    }
    file << std::endl;
    file.close();
    return 0;
}


///--------------------------------------------------------
///  utr_io_read_mesh - to read mesh with given filename(s)
///-------------------------------------------------------
///  Reads all mesh files from current working directory
///  that matches with passed regular expression Mesh_files_regex.
///  All files HAVE TO be the same type (ex. nas,dat,in etc.).
///  NOTE: single filename ex. "mesh.dat" is also a valid regular expression.
///-------------------------------------------------------
/// What this function does in short:
/// 1. Gather files list form directory.
/// 2. Filter files list with mesh filename regex.
/// 3. Sort in alphabetic order.
/// 4. Loop over selected files and read each one.
///--------------------------------------------------------
///!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/// IMPORTANT NOTE: if there is available more than one process,
/// then each process will recive one file to read.
/// NOTE: if there is more processes than files,
/// mesh needs to be redistributed (balancing)!
/// NOTE: if there is more files than processes,
/// processes will recive (in continues manner) more files to read.
///!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int utr_io_read_mesh( /// returns: > 0 - number of files correctly readed; <=0 - error
                     const int Mesh_id, ///IN: id of mesh to write to
                     const char * Working_dir, ///IN: directory with mesh files
                     const char * Mesh_files_regex, ///IN: regular expression pattern
                       /// NOTE: using regex 'POSIX grep' standard
                       const char Mesh_type ///IN: mesh type character (the same for all files!)
					  )
{
  assert(Working_dir != NULL);
  assert(Mesh_files_regex != NULL);

  mf_check(Mesh_files_regex[0]!='\0',"Mesh file regex(%s) is invalid!",Mesh_files_regex);

  std::string dir(Working_dir);
  std::string file(Mesh_files_regex);
  if(std::string::npos != file.find('/') ) {
      dir.append(Mesh_files_regex);
      boost::filesystem::path p(dir);
      p = boost::filesystem::canonical(p);
      dir = p.parent_path().string();
      file = p.filename().string();
  }

  mf_debug("\nDir: %s, Filename: %s",dir.c_str(),file.c_str());

  std::vector<string> filenames;
  typedef std::vector<string>::iterator filename_iter;
  
  /// 1. Gather files list form directory.
  utr_io_gather_filenames_from_dir(dir.c_str(), filenames);

  /// 2. Filter files list with mesh filename regex.
  utr_io_filter_filenames(filenames, file.c_str());
  mf_check_debug(!filenames.empty(),"\nNot found any files matching pattern %s", file.c_str());
  
  /// 3. Sort in alphabetic order.
  sort(filenames.begin(), filenames.end());
  
  /// 4. Loop over selected files and read each one.
  int n_elems_readed=0,n_total_elems=0;
  const mmpt_io_fptr read_file= utr_io_get_reading_function(Mesh_type);  
  filename_iter it= filenames.begin(),
	end = filenames.end();

#ifdef PARALLEL
  const int nr_sub = pcr_nr_proc(),
	my_proc_id = pcr_my_proc_id();
  
  /// It have to be at least 2 processes/subdomains.
  if( nr_sub > 1 ) {
    /// Compute how many mesh files per process.
    const int mesh_file_per_proc = filenames.size() / nr_sub;
    /// Compute how many mesh files left.
    /// The remaining mesh files will be passed to processes with lowest id's.
    const int mesh_files_left = filenames.size() % nr_sub;
	
    /// n_mesh_files > subdomain_id(=process_id)
	if( filenames.size() >= static_cast<size_t>(my_proc_id) ) {
	  
      /// Assign evenly (if possible) and continously (neighboring mesh-files at the same process)
      /// mesh-files per processes until there are no more meshes....
	  it += (my_proc_id-PCC_MASTER_PROC_ID) * mesh_file_per_proc
        /// Take into account earlier distribution of mesh_files_left.
		+ std::min(my_proc_id-PCC_MASTER_PROC_ID, mesh_files_left);
	  
      /// Point 'end' to the file for next subdomain/process.
      /// (loop will end after reading required  mesh files)
	  end = it+mesh_file_per_proc+ ((my_proc_id-PCC_MASTER_PROC_ID) < mesh_files_left ? 1 : 0); 
	}
	else {
      /// ... to other processes assign NO mesh files.
      it=end; /// (this will end loop at beginning)
    }///!if
  }///!if nr_sub>1
#endif ///!PARALLEL
  
  for( ; it != end ; ++it) {
    /// Make filename a valid path to file.
    it->insert(0,"/");
    it->insert(0,dir);

    mmr_init_read(Mesh_id,0,0,0,0);

	n_elems_readed=read_file(Mesh_id, it->c_str());
	
    mmr_finish_read(Mesh_id);

	assert( n_elems_readed > 0 );
	n_total_elems+=n_elems_readed;
  }  
  if(n_total_elems > 0) {
    mmr_test_mesh(Mesh_id);


    time_init();

    time_print();
  }
  assert(n_total_elems <=  mmr_get_nr_elem(Mesh_id));
  return filenames.size();
}



// forward declaration
mmpt_io_fptr utr_io_get_writing_function(const char Mesh_type);

/**--------------------------------------------------------
utr_io_export_mesh - to export mesh of a specified type
---------------------------------------------------------*/
int utr_io_export_mesh( /** returns number of files exported */
  FILE *Interactive_output, /** file or stdout to write messages */
  const int Mesh_id,
  char Mesh_type, /** symbol denoting \link  mmt_file_type  mesh_type \endlink  */
  char* Mesh_file /** mesh file name - conforming to naming convention */
  )
{
    int ret_val=0;

    utr_io_result_add_value(RESULT_OUT_FILENAME,Mesh_file);

    mmpt_io_fptr writing_function = utr_io_get_writing_function(Mesh_type);

    // Handling request at global level
    if(writing_function != NULL) {
        //mf_log_info("Exporting mesh %d into file (%c) %s",Mesh_id, Mesh_type, Mesh_file );
        writing_function(Mesh_id,Mesh_file);
        ++ret_val;
    }
    else { // if not supproted passing to mesh module
         mmr_export_mesh(Mesh_id,Mesh_type,Mesh_file);
         ++ret_val;
    }

	utr_io_compress_file(NULL, Mesh_file);

    return ret_val;
}

/**--------------------------------------------------------
utr_io_export_mesh - to display menu for exporting mesh
---------------------------------------------------------*/
int utr_menu_export_mesh(/** returns number of files exported */
  FILE *Interactive_output, /** file or stdout to write messages */
  FILE *Interactive_input,
  const int Mesh_id, // id of mesh to export
  const char * Work_dir,
  char* Mesh_file /** mesh file name - conforming to naming convention */
  )
{
    // select export type

    printf("\t Select output:\n");

    for(int i=0; i < MMC_MAX_FILE_TYPE; ++i) {
        if( utr_io_get_writing_function(i) != NULL) {
            printf("\t\t %c\n",i);
        }

    }

    char c=0;
    do {
        fscanf(Interactive_input, "%c", &c);
    }while(c != 'n' && c!= 's' && c!='l' && c != 'f');

    char mesh_file[256]={0};
    sprintf(mesh_file, "%s/%s.dmp", Work_dir,Mesh_file);

    utr_io_export_mesh(Interactive_output,
                       Mesh_id, c ,mesh_file);

    return 0;
}

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
  )
{
    mf_check_mem(Filename);

    int nf =  apr_write_field(Field_id, Nreq, Select, Accuracy, Filename);

    utr_io_compress_file(NULL, Filename);

    return nf;
}

/// utr_io_write_img_to_pam - to write data into PAM image file.
/// http:///en.wikipedia.org/wiki/Netpbm
/// Allowed combinations of parameters:
/// -----------------------------------------------------------
/// TUPLTYPE           |MAXVAL |DEPTH  |comment
/// -----------------------------------------------------------
/// BLACKANDWHITE        1       1       special case of GRAYSCALE
/// GRAYSCALE            2…65535	1       2 bytes per pixel for MAXVAL > 255
/// RGB                  1…65535	3       6 bytes per pixel for MAXVAL > 255
/// BLACKANDWHITE_ALPHA	1       2       2 bytes per pixel
/// GRAYSCALE_ALPHA      2…65535	2       4 bytes per pixel for MAXVAL > 255
/// RGB_ALPHA            1…65535	4       8 bytes per pixel for MAXVAL > 255
/// -----------------------------------------------------------
int utr_io_write_img_to_pam( /// returns 0 if all ok.
        const char * Work_dir,  /// Directory to write in.
        const char * Filename,  /// without extension
        const int Width,  /// >0
        const int Height, /// >0
        const int Depth,  /// <1:4>
        const int Maxval, /// <1:65535>,
        const char* TUPLTYPE,
        const char* Img_data, /// [Width*Height*byte per pixel]  pointer to the array with image data
        FILE* Interactive_output
        )
{
    mf_check_mem(Work_dir);
    mf_check_mem(Filename);
    mf_check_mem(TUPLTYPE);
    mf_check_mem(Img_data);

    mf_check(Width > 0,"utr_io_write_img_to_pam : Width=%d",Width);
    mf_check(Height > 0,"utr_io_write_img_to_pam : Height=%d",Height);

    std::string fn(Work_dir);
    fn.append("/");
    fn.append(Filename);
    fn.append(".pam");

    int bpp = Depth;
    if(Maxval > 255) {
        bpp *= 2 ;
    }

    /* write the whole data array to ppm file in one step */
    /* create new file, give it a name and open it in binary mode */
    FILE * fp = fopen(fn.c_str(), "wb");
    /* write header to the file */
    fprintf(fp, "P7\nWIDTH %d\nHEIGHT %d\nDEPTH %d\nMAXVAL %d\nTUPLTYPE %s\nENDHDR\n", Width, Height, Depth,
            Maxval,TUPLTYPE);
    /* write image data bytes to the file */
    fwrite(Img_data, Width*Height*bpp, 1, fp);
    fclose(fp);
    printf("File %s saved.\n", fn.c_str());
    return 0;
}

/// utr_io_write_img_to_pnm - to write data into PNM image file.
/// The Portable Bit/Grey/PixMap formats PBM, PGM, PPM.
/// They are collectively referred to as PNM (Portable aNy Map).
/// http:///en.wikipedia.org/wiki/Netpbm_format
/// Allowed combinations:
/// -----------------------------------------------------------
/// Type                 Magic number	Extension	Colors
/// -----------------------------------------------------------
/// Portable BitMap		P4	binary      .pbm        0–1 (black & white)
/// Portable GrayMap		P5	binary      .pgm        0–255 (gray scale)
/// Portable PixMap		P6	binary      .ppm        0–255 (RGB)
/// -----------------------------------------------------------
int utr_io_write_img_to_pnm(/// returns 0 if all ok.
        const char * Work_dir,  /// Directory to write in.
        const char * Filename,  /// without extension
        const char * Comment, /// written into img file header, can be NULL if no comment.
        const int Width,  /// >0
        const int Height, /// >0
        const int Max_color_component_value,
        const int Magic_number, /// {4,5,6}
        const char* Img_data, /// pointer to the array with image data
        FILE* Interactive_output
        )
{

    /* write the whole data array to ppm file in one step */
    /* create new file, give it a name and open it in binary mode */
    std::string fn(Work_dir);
    fn.append("/");
    fn.append(Filename);

    switch(Magic_number) {
    case 4: fn.append(".pbm");
        break;
    case 5: fn.append(".pgm");
        break;
    case 6: fn.append(".ppm");
        break;
    default: mf_log_err("utr_io_write_img_to_pnm: unknown Magic number (%d)",Magic_number);
            return -1;
        break;
    }

    FILE * fp = fopen(fn.c_str(), "wb");
    /* write header to the file */
    fprintf(fp, "P%d\n %s\n %d\n %d\n %d\n", Magic_number, Comment, Width, Height,
            Max_color_component_value);
    /* write image data bytes to the file */
    fwrite(Img_data, Width*Height, 1, fp); /// bytes per pixel = 1
    fclose(fp);
    printf("File %s saved.\n", fn.c_str());
    return 0;
}

int utr_io_write_img_to_pbm(/// returns 0 if all ok.
        const char * Work_dir,  /// Directory to write in.
        const char * Filename,  /// without extension
        const char * Comment, /// written into img file header, can be NULL if no comment.
        const int Width,  /// >0
        const int Height, /// >0
        const int Max_color_component_value,
        const int Magic_number, /// {1,4}
        const unsigned char* Img_data, /// pointer to the array with image data
        FILE* Interactive_output
        )
{

    /* write the whole data array to ppm file in one step */
    /* create new file, give it a name and open it in binary mode */
    std::string fn(Work_dir);
    fn.append("/");
    fn.append(Filename);
	fn.append(".pbm");
	FILE * fp;

	switch(Magic_number) {
    case 1: fp = fopen(fn.c_str(), "w");
        break;
    case 4: fp = fopen(fn.c_str(), "wb");
        break;
    default: mf_log_err("utr_io_write_img_to_pbm: unknown Magic number (%d)",Magic_number);
            return -1;
        break;
    }

    /* write header to the file */
    fprintf(fp, "P%d\n%s\n%d %d ", Magic_number, Comment, Width, Height);
    /* write image data bytes to the file */
    fwrite(Img_data, (Width*Height/8), 1, fp); /// bit per pixel = 1
    fclose(fp);
    printf("File %s saved.\n", fn.c_str());
    return 0;
	

}


/// ! extern "C"
#ifdef __cplusplus
}
#endif

/// Internal routines (not extern "C")

///---------------------------------------------------------
/// utr_io_gather_filenames_from_dir - to write all filenames into given vector
///---------------------------------------------------------
void utr_io_gather_filenames_from_dir(const char * Dir,
                                      std::vector<string> & Filenames)
{
  using namespace  boost::filesystem;

  assert( Dir != NULL);

  Filenames.clear();

  try {
    path p(Dir); /// this should be working directory...
    if(exists(p)) {
      if(is_directory(p)) {
        for(directory_iterator it(p); it != directory_iterator(); ++it) {
#if(BOOST_VERSION>104600)
            Filenames.push_back( it->path().filename().string() );
#else
          Filenames.push_back(it->path().filename());
#endif
        }///!for
      }///!if
    }///!if
  }///!try
  catch( const filesystem_error& ex) {
    throw ex;
  }
}

///---------------------------------------------------------
/// utr_io_filter_filename - to filter filenames with given regular expression
///---------------------------------------------------------
void utr_io_filter_filenames(std::vector<string>& Filenames, /// vector of filenames
                               const char * Regex /// regular expression to test against
                               /// during filtering
                             )
{
  using boost::regex;
  using boost::smatch;
  using boost::regex_match;

  typedef std::vector<string>::iterator iter;

  assert( Regex != NULL );
  assert( Filenames.size() > 0 );

  const char * r = Regex;
  if(Regex[0]=='.' && Regex[1]=='/') {
      mf_log_warn(" 'File name regex' begins with './' - stripping that from regular expression.");
      r += 2;
  }

  regex re(r);
  smatch what;

  /// Find entries that DON'T MATCH and remove them.
  iter it(Filenames.begin());
  while( it != Filenames.end() ) {
    if( regex_match(*it,what,re) == false ) {
      it = Filenames.erase(it);
    } else {
      ++it;
    }
  }///!for
}

int utr_io_count_string_in_file(
                                std::ifstream & File,
                                const string & To_find
)
{
  int starting_point = File.tellg();
  int count = std::count(std::istream_iterator<string>(File),
                       std::istream_iterator<string>(),
                         To_find);
  File.clear();
  File.seekg(starting_point);
  assert(File.good());
  return count;
}


///--------------------------------------------
///  utr_io_read_nas - to read single .nas file
///--------------------------------------------
int utr_io_read_nas(const int Mesh_id,const char* filename)
{
  const string node_str("GRID"),
    tria_str("CTRIAX"),
    quad_str("CQUADX"),
    tetra_str("CTETRA"),
    prism_str("CPRIZM");

  std::ifstream file(filename);

  assert(file.good());

  const int n_nodes=utr_io_count_string_in_file(file,node_str),
    n_tria=utr_io_count_string_in_file(file,tria_str),
    n_quad=utr_io_count_string_in_file(file,quad_str),
    n_tetra=utr_io_count_string_in_file(file,tetra_str),
    n_prism=utr_io_count_string_in_file(file,prism_str);

  assert(n_nodes > 0);
  assert(n_prism + n_tetra > 0);

  /// below: tetrahedron have 6 edges
  /// prismatic element have 9 edges
  /// in case of expanding mesh we reserve also for already exisintg entities
  mmr_init_read(Mesh_id,
              mmr_get_nr_node(Mesh_id) + n_nodes,
              mmr_get_nr_edge(Mesh_id) + n_tetra*6+n_prism*9,
              mmr_get_nr_face(Mesh_id) + n_tria+n_quad+n_tetra*4+n_prism*5,
              mmr_get_nr_elem(Mesh_id) + n_tetra+n_prism);

  /////////////// 1. Reading Nodes. ///////////////////////////////////////////////////////////
  /// Find first node
  std::string s_tmp;
  do {
    file >> s_tmp;
  } while ( 0 != s_tmp.compare(node_str) );
  /// Move back before 'node_str'
  file.seekg (file.tellg() - static_cast<std::streampos>(node_str.length()));

  double val,coords[3]={0};
  int n_readed_nodes=0,id=0;
  while(n_readed_nodes < n_nodes && file.good())
    {
      file >> s_tmp;
      file >> id >> coords[0] >> coords[1] >> coords[2] >> val;
      mmr_add_node(Mesh_id,MMC_AUTO_GENERATE_ID,coords);
      ++n_readed_nodes;
    }
  assert(n_readed_nodes == n_nodes);
  assert(file.good());

  ///////////////// 2. Reading edges - omitted. ///////////////////////////////////////////////////////////
  /// In .nas file there are no edges

  /////////////// 3. Reading faces. ///////////////////////////////////////////////////////////
  int n_readed_tria=0, n_readed_quad=0;
  int bc(0),vertices[6]={0};
  while ( (n_readed_tria+n_readed_quad) < (n_tria+n_quad) && file.good() ) {
    file >> s_tmp;

    if(0 == s_tmp.compare(tria_str)) {
        file >> id;	/// number - ignore it
        file >> bc;  /// element group no. = bc no.
        file >> vertices[0];/// fPtr->verts(0) = tmp2; /// vertices no. !!! (not edges)
        file >> vertices[1]; ///fPtr->verts(1) = tmp2;
        file >> vertices[2]; ///fPtr->verts(2) = tmp2;
        std::sort(vertices, vertices+3);

        mmr_add_face(Mesh_id,MMC_AUTO_GENERATE_ID,MMC_TRIA,vertices,NULL,NULL,bc);
        ++n_readed_tria;
      }
    else if(0 == s_tmp.compare(quad_str)) {
        file >> val;	/// number - ignore it
        file >> bc;  /// element group no. = bc no.
        file >> vertices[0];/// fPtr->verts(0) = tmp2; /// vertices no. !!! (not edges)
        file >> vertices[1]; ///fPtr->verts(1) = tmp2;
        file >> vertices[3]; ///fPtr->verts(2) = tmp2;
        file >> vertices[2];

        mmr_add_face(Mesh_id,MMC_AUTO_GENERATE_ID,MMC_QUAD,vertices,NULL,NULL,bc);
        ++n_readed_quad;
      }
  } ///! while - faces
  assert(file.good());
  assert(n_readed_tria == n_tria);
  assert(n_readed_quad == n_quad);


  /////////////// 4. Reading elements. ///////////////////////////////////////////////////////////////
  /////////
  int n_readed_tetra(0),n_readed_prism(0),tmp2;

  file.seekg(0);

  while( (n_readed_tetra+n_readed_prism) < (n_tetra+n_prism) && file.good()) {
    file >> s_tmp;

    if(0 == s_tmp.compare(tetra_str)) {
      file >> val;	/// number - ignore it
      file >> tmp2; /// element group no. = material
      file >> vertices[0]; /// vertices no. !!! (not edges)
      file >> vertices[1];
      file >> vertices[2];
      file >> vertices[3];
      std::sort(vertices,vertices+4);

      mmr_add_elem(Mesh_id,MMC_AUTO_GENERATE_ID,MMC_TETRA,vertices,NULL,tmp2);

      ++n_readed_tetra;
            }
    else if(0 == s_tmp.compare(prism_str))  {
      file >> val;	/// number - ignore it
      file >> tmp2; /// element group material
      file >> vertices[3]; /// vertices no. !!! (not edges)
      file >> vertices[4];
      file >> vertices[5];
      file >> vertices[0];
      file >> vertices[1];
      file >> vertices[2];

      mmr_add_elem(Mesh_id,MMC_AUTO_GENERATE_ID,MMC_PRISM,vertices,NULL,tmp2);

      ++n_readed_prism;

    }

  } ///! while - elements
  file.close();
  assert(n_readed_tetra == n_tetra);
  assert(n_readed_prism == n_prism);

  return n_readed_tetra+n_readed_prism;
}

///--------------------------------------------
///  utr_io_read_in - to read single .in file
///--------------------------------------------
int utr_io_read_in(const int Mesh_id,const char* filename)
{
  return 0;
}

///--------------------------------------------
///  utr_io_read_binary - to read single binary dump file
///--------------------------------------------
int utr_io_read_binary(const int Mesh_id,const char* filename)
{
  return 0;
}


///--------------------------------------------
///  utr_io_read_hybrid - to read single .kaz file
///--------------------------------------------
int utr_io_read_hybrid(const int Mesh_id,const char* filename)
{
  return 0;
}

///--------------------------------------------
///  utr_io_read_am - to read single .am file
///--------------------------------------------
int utr_io_read_am(const int Mesh_id,const char* filename)
{
  return 0;
}

///--------------------------------------------
///  utr_io_read_dat - to read single .dat file
///--------------------------------------------
int utr_io_read_dat(const int Mesh_id,const char* filename)
{
    std::ifstream file(filename);
    assert(file.good());

    int tmp,next_index(0), vert_count(0);
    file >> tmp >> tmp >> tmp >> tmp >> vert_count >> next_index;

    /// Vertices
    mmr_init_read(Mesh_id,vert_count,0,0,0);
    int vert_readed=0;
    double coords[3]={0.0};
    while(vert_readed < vert_count)
    {
        file >> coords[0] >> coords[1] >> coords[2];
        mmr_add_node(Mesh_id,MMC_AUTO_GENERATE_ID,coords);
        ++vert_readed;
    }

    /// Edges
    int edge_count=0;
    file >> edge_count >>  tmp;
    mmr_init_read(Mesh_id,0,edge_count,0,0);

    int v[6]={0},type=0,edge_readed=0;
    while(edge_readed < edge_count)
    {
        file >>  type;
        file >> v[0] >> v[1];
        ///if(vertices[0] > vertices[1])
        ///{
        ///	std::swap(vertices[0],vertices[1]);
        ///}
        mmr_add_edge(Mesh_id,MMC_AUTO_GENERATE_ID,v);
        ++edge_readed;
    }

    /// Faces
    int face_count=0;
    file >> face_count >> tmp;

    mmr_init_read(Mesh_id,0,0,face_count,0);

    int face_readed=0,params[2]={0},edges[5]={0};
    bool edges_flip[4]={false};
    while((face_readed < face_count) && file.good())
    {
        file >> type;
        if(type < 2 && type > 4)
        {
            throw "\nDmpFileImporter::GetNextFace: wrong face_type.\n Make sure that file has platform-specyfic file endings (Win/Linux).";
        }
        file >> tmp;
        file >> params[0] >> params[1];

        /// check negative values
        for(int i(0); i < type; ++i)
        {
            edges_flip[i]=false;
            file >> edges[i];
            edges_flip[i]=(edges[i] < 0);
            if(edges_flip[i])
            {
                edges[i]*=-1;
            }

            assert(edges[i] >= 1);
        }

        ///for(register int i(0); i < face_type; ++i) {
        ///    edgePtrs[i]=&m.edges_[edges[i]];
        ///    assert(edgePtrs[i] != NULL);
        ///}

        ///std::sort(edgePtrs,edgePtrs+face_type, & Edge::comparePtrs );

        ///for(register int i(0); i < face_type; ++i) {
        ///	edges[i]=edgePtrs[i]->pos_;
        ///}
        int ed_nodes[2]={0};
        mmr_edge_nodes(Mesh_id,edges[0],ed_nodes);
        v[0]= ed_nodes[ edges_flip[0] ? 1 : 0 ];
        v[1]= ed_nodes[ edges_flip[0] ? 0 : 1 ];
        switch(type)
        {
        case 3: /// MMC_TRIA:
            {
                mmr_edge_nodes(Mesh_id,edges[1],ed_nodes);
                v[2]= ed_nodes[ edges_flip[1] ? 0 : 1 ];
                v[3]=0;
              } break;

        case 4: ///MMC_QUAD:
            {
                mmr_edge_nodes(Mesh_id,edges[2],ed_nodes);
                v[2]= ed_nodes[ edges_flip[2] ? 0 : 1 ];
                v[3]= ed_nodes[ edges_flip[2] ? 1 : 0 ];
            } break;
        default: throw "Unknown face type!"; break;
        }
        mmr_add_face(Mesh_id,MMC_AUTO_GENERATE_ID,type,v,edges,params,tmp);
        ++face_readed;
    }

    int elem_count=0,elem_readed=0;
    file >> elem_count >> tmp;

    mmr_init_read(Mesh_id, 0,0,0,elem_count);
    while(elem_readed < elem_count)
    {
        int mt,rt,father,faces[5],face_fliped[5];
        file >> type;
        file >> mt >> father >> rt ;

        const int n_faces = type==MMC_TETRA? 4: 5;

        for(unsigned int i(0); i < n_faces; ++i)
        {
            file >> faces[i];
            if(faces[i] < 0) {
                face_fliped[i]=true;
                faces[i]*=-1;
            }
            else {
                face_fliped[i]=false;
            }
        }
        int vertices[6]={0},fa_nodes[5]={0};
        mmr_fa_node_coor(Mesh_id,faces[0],fa_nodes,NULL);
        switch(type)
        {
        case 7: ///MMC_TETRA:
        {
            vertices[0] = fa_nodes[1+0];
            vertices[1] = fa_nodes[1+1];
            vertices[2] = fa_nodes[1+2];
            mmr_fa_node_coor(Mesh_id,faces[1],fa_nodes,NULL);
            vertices[3] = fa_nodes[1+2];
        } break;
        case 5: ///MMC_PRISM:
        {
            vertices[0] = fa_nodes[1+0];
            /// HACK below: to conform KB ordering of nodes faceFlip is... flipped ;)
            /// see PHP_FEM.pdf and hHybridMesh.pdf for details
            vertices[1] = fa_nodes[1+(face_fliped[0]?1:2)];
            vertices[2] = fa_nodes[1+(face_fliped[0]?2:1)];
            mmr_fa_node_coor(Mesh_id,faces[1],fa_nodes,NULL);
            vertices[3] = fa_nodes[1+0];
            vertices[4] = fa_nodes[1+(face_fliped[1]?2:1)];
            vertices[5] = fa_nodes[1+(face_fliped[1]?1:2)];
        } break;
        default: {
            assert(!"DmpFileImporter::doRead unknown element type");
            throw "DmpFileImporter::doRead unknown element type";
        } break;
        }///!switch(element_type)

        mmr_add_elem(Mesh_id,MMC_AUTO_GENERATE_ID,type,vertices,faces,mt);
        ++elem_readed;
    }

    assert(elem_readed > 0);
  return elem_readed;
}

///--------------------------------------------
///  utr_io_read_vtk - to read single .vtk file
///--------------------------------------------
int utr_io_read_vtk(const int Mesh_id,const char* filename)
{
  return 0;
}

///--------------------------------------------
///  utr_io_read_jk - to read single .jk file
///--------------------------------------------
int utr_io_read_jk(const int Mesh_id,const char* filename)
{
    std::ifstream file(filename);

    assert(file.good());
    /// Reading initial data.
    int mx_nodes=0, mx_edges=0, mx_faces=0, mx_elems=0;
    file >> mx_nodes >> mx_edges >> mx_faces >> mx_elems;

    int num_nod_jk=0,z_bottom=0,z_top=0,num_el_lay=0,bottom_bc=0,top_bc=0,
            n_prisms=0;
    file >> num_nod_jk >> z_bottom >> z_top >> num_el_lay >> bottom_bc >> top_bc;

    if(mx_nodes < (num_nod_jk * (num_el_lay+1))) {
        mx_nodes = num_nod_jk * (num_el_lay+1);
    }

    if(num_el_lay < 1) {
        num_el_lay = 1;
    }

    int dz = (z_top-z_bottom)/num_el_lay;

    if(mx_edges < (4*num_nod_jk*(num_el_lay+1)) ) {
        mx_edges = 4*num_nod_jk*(num_el_lay+1);
    }
    /// Reserving space for all kind of nodes.
    mmr_init_read(Mesh_id,mx_nodes,mx_edges,mx_faces,mx_elems);

    /// Reading vertices.
    for(int istr=1; istr <= num_nod_jk; ++istr) {
        double coords[3]={0.0};
        /// Read plain x y coords.
        assert(file.good());
        file >> coords[0] >> coords[1] ;
        for(int ilayer=0; ilayer < num_el_lay; ++ilayer) {
            coords[2]=z_bottom + ilayer*dz;
            mmr_add_node(Mesh_id, istr+ilayer*num_nod_jk, coords);
        }
    }

    /// read the number of stored element structures
    int num_el_jk=0;
    file >> num_el_jk;

    for(int istr=1;istr<=num_el_jk; ++istr) {
        int mat_num;
        assert(file.good());
        file >>  mat_num;
        int nr_nodes = mat_num>0 ? 3 : 4;
        mat_num = abs(mat_num);

        assert(nr_nodes == 3);

        int el_nodes[8]={0};
        assert(file.good());
        file >> el_nodes[0] >> el_nodes[1] >> el_nodes[2];

        for(int ilayer=0; ilayer<num_el_lay; ++ilayer) {
            el_nodes[3] = el_nodes[0]+ilayer*num_nod_jk;
            el_nodes[4] = el_nodes[1]+ilayer*num_nod_jk;
            el_nodes[5] = el_nodes[2]+ilayer*num_nod_jk;
            mmr_add_elem(Mesh_id,MMC_AUTO_GENERATE_ID,MMC_PRISM,el_nodes,NULL,0);
            ++n_prisms;
        }
    }

    file.close();
    return n_prisms;
}

///--------------------------------------------
///  utr_io_read_msh - to read single .msh (ansys) file
///--------------------------------------------
int utr_io_read_msh(const int Mesh_id,const char* filename)
{

    /// mapping from MSH cell/element/face types to MMC_constans
    static const int msh_el_type2mf_type[] = {
        MMC_ENTITY_UNKNOWN, /// mixed
        MMC_ENTITY_UNKNOWN, ///triangular
        MMC_TETRA, ///tetrahedral
        MMC_ENTITY_UNKNOWN, ///quadrilateral
        MMC_ENTITY_UNKNOWN, ///hexahedral
        MMC_ENTITY_UNKNOWN, ///pyramid
        MMC_PRISM, ///wedge
    };

    static const int msh_fa_type2mf_type[] = {
        MMC_ENTITY_UNKNOWN, /// mixed
        MMC_ENTITY_UNKNOWN, ///
        MMC_ENTITY_UNKNOWN, ///linear
        MMC_TRIA, ///triangular
        MMC_QUAD, ///quadrilateral
    };

    std::ifstream file(filename);

    int section_type_index = 0;
    std::string tmp;
    int n_elems=0,n_faces=0, n_edges=0, n_nodes=0;
    bool reserved = false;

    while(file.good()) {
        // go to next section

        // read index

        switch(section_type_index) {
        case 0: {// comment
            file >> tmp;
            mf_log_info("File %s has comment %s: ",filename,tmp.c_str());
        }
            break;

        case 2: { // dimensions
            int dim=0;
            file >> dim;
            mf_check(dim == 3, "Mesh file has incorrect not 3D coordinates! (%d)",dim);
        }
            break;

        case 10: { // nodes
            file >> tmp; // read whitespaces and '('
            int zone_id=-1,
                    first_index=-1,
                    last_index=-1,
                    type=-1,
                    ND=-1;
            file >> zone_id;
            file >> std::hex >> first_index;
            file >> std::hex >> last_index;
            file >> type >> ND;
            file.ignore(255,'\n');

            if(zone_id == 0) { // total number of nodes in grid
                n_nodes = last_index;
            }
            else { // nodes definition

                if(!reserved) {
                    reserved=true;
                    mmr_init_read(Mesh_id,n_nodes, n_edges, n_faces, n_elems);
                }

                double x[3]={0.0};

                for(int i=first_index; i <= last_index; ++i) {
                    file >> x[0] >> x[1] >> x[2];

                    mf_log_info("Reading nodes %lf, %lf, %lf",x[0],x[1],x[2]);

                    mmr_add_node(Mesh_id,i,x);
                }
            }
        }
            break;

        case 12: { // cells (elements)
            file >> tmp; // read whitespaces and '('
            int zone_id=-1,
                    first_index=-1,
                    last_index=-1,
                    type=-1,
                    element_type=-1;
            file >> zone_id;
            file >> std::hex >> first_index;
            file >> std::hex >> last_index;
            file >> type >> element_type;
            file.ignore(255,'\n');

            if(zone_id == 0) { // total number of nodes in grid
                n_elems = last_index;
                n_edges = 6*n_elems;
                if(n_faces==0) {
                    n_faces = 5*n_elems;
                }
            }
            else { // nodes definition

                if(!reserved) {
                    reserved=true;
                    mmr_init_read(Mesh_id,n_nodes, n_edges, n_faces, n_elems);
                }

                double x[3]={0.0};

                int nodes[MMC_MAXELVNO]={0};
                for(int i=first_index; i <= last_index; ++i) {


                    mf_log_info("Reading elem ");

                    mmr_add_elem(Mesh_id,i,msh_el_type2mf_type[element_type],nodes,NULL,zone_id);
                }
            }
        }
            break;

        case 13: { // faces
            file >> tmp; // read whitespaces and '('
            int zone_id=-1,
                    first_index=-1,
                    last_index=-1,
                    type=-1,
                    element_type=-1;
            file >> zone_id;
            file >> std::hex >> first_index;
            file >> std::hex >> last_index;
            file >> type >> element_type;
            file.ignore(255,'\n');

            if(zone_id == 0) { // total number of nodes in grid
                n_elems = last_index;
                n_edges = 6*n_elems;
                if(n_faces==0) {
                    n_faces = 5*n_elems;
                }
            }
            else { // nodes definition

                if(!reserved) {
                    reserved=true;
                    mmr_init_read(Mesh_id,n_nodes, n_edges, n_faces, n_elems);
                }

                double x[3]={0.0};

                int nodes[MMC_MAXELVNO]={0};
                for(int i=first_index; i <= last_index; ++i) {


                    mf_log_info("Reading elem ");

                    mmr_add_elem(Mesh_id,i,msh_el_type2mf_type[element_type],nodes,NULL,zone_id);
                }
            }
        }
            break;

        case 45: // zone assigment
//            For each cell and face zone you will have the line
//            (45 (id type name)())
//            where
//            type = wall, solid, interior, etc
//            name = name of the boundary that'll appear in GUI
        case 1: // header
        case 18: // periodic shadow faces
        case 58: // element tree
        case 59: // face tree
        case 61: // interface face parents
            mf_log_warn("Ignored section with index %d in file %s.",section_type_index,filename);
            break;
        default:
            mf_log_err("Unknown secion type %d in file %s!",section_type_index,filename);
            break;
        }// !switch
    } // !while(file.good())

    return 0;
}

///--------------------------------------------
///  utr_io_write_vtk - to write single .vtk file
///--------------------------------------------
int utr_io_write_vtk(const int Mesh_id,const char* filename)
{
    return 0;
}


///--------------------------------------------
///  utr_io_write_msh - to write single .msh file
///--------------------------------------------
int utr_io_write_msh(const int Mesh_id,const char* filename)
{
	return 0;
}

///--------------------------------------------
///  utr_io_write_nas - to read single .nas file
///--------------------------------------------
int utr_io_write_nas(const int Mesh_id,const char* Filename)
{

    using std::ios_base;
    using std::setw;

    std::string filename(Filename);
    filename.append(".nas");

     std::ofstream file(filename.c_str());
	 file.setf(ios_base::fixed);

    file  << "TITLE=ModFEM " << "ver." << SVNVERSION << "\n"
       << "$" << "\n"
       << "BEGIN BULK" << "\n";

	std::string d = ", ";

    // write points
    file  << "$" << "\n"
          << "$ Points" << "\n"
          << "$" << "\n";

    file << std::showpoint;

	int glob_id = 0;
    int iNode=0;
    while(0!=(iNode=mmr_get_next_node_all(Mesh_id,iNode))) {

        double x[3]={0.0};
        mmr_node_coor(Mesh_id,iNode,x);

        // Fixed short/long formats:
       // 1 GRID
       // 2 ID   : point ID - requires starting index of 1
       // 3 CP   : co-ordinate system ID                (blank)
       // 4 X1   : point x cp-ordinate
       // 5 X2   : point x cp-ordinate
       // 6 X3   : point x cp-ordinate
       // 7 CD   : co-ordinate system for displacements (blank)
       // 8 PS   : single point constraints             (blank)
       // 9 SEID : super-element ID


        file  << "GRID"
			<< d << glob_id++
			<< d
			<< d << x[0]
			<< d << x[1]
			<< d << x[2]
            << "\n";
    }




      // write faces
      file  << "$" << "\n"
          << "$ Faces" << "\n"
          << "$" << "\n";


      int iFace=0;
      int face_type=0;
      std::string face_string;
      int facePts[MMC_MAXFAVNO+1]={0};

      while(0!=(iFace=mmr_get_next_act_face(Mesh_id,iFace))){
        face_type = mmr_fa_type(Mesh_id,iFace);
          switch(face_type) {
        case 3: face_string = "CTRIA3"; // MMC_TRIA
            break;
        case 4: face_string = "CQUAD4"; // MMC_QUAD
            break;
        }


          mmr_fa_node_coor(Mesh_id,iFace,facePts,NULL);

          // Fixed short/long formats:
          // 1 CQUAD4
          // 2 EID  : element ID
          // 3 PID  : property element ID; default = EID   (blank)
          // 4 G1   : grid point index - requires starting index of 1
          // 5 G2   : grid point index
          // 6 G3   : grid point index
          // 7 G4   : grid point index
          // 8 onwards - not used

          // For CTRIA3 elements, cols 7 onwards are not used

		  file << face_string << d
			  << glob_id++ << d
			  << mmr_fa_bc(Mesh_id, iFace) << d;

          for(int i=0; i < face_type; ++i)
          {
			  file << d << facePts[1 + i];
          }

          file  << "\n";

      } //!while

      // write element
      file  << "$" << "\n"
          << "$ Elements" << "\n"
          << "$" << "\n";

      std::string elem_string;
      int iElem=0;
      int el_nodes[MMC_MAXELVNO+1]={0};

      while(0 != (iElem=mmr_get_next_act_elem(Mesh_id,iElem))) {
          int type = mmr_el_type(Mesh_id,iElem);

          switch(type) {
          case MMC_TETRA: elem_string="CTETRA"; break;
          case MMC_PRISM: elem_string="CPENTA"; break;
          default: mf_fatal_err("Uknown element type(%d)!",type);
          }

          mmr_el_node_coor(Mesh_id,iElem,el_nodes,NULL);

		  file << elem_string << d << glob_id++ << d << mmr_el_groupID(Mesh_id, iElem);

          for(int i=0; i < el_nodes[0]; ++i) {
			  file << d << el_nodes[1 + i];
          }

         file << "\n";

      }

    file  << "ENDDATA" << std::endl;

    mf_log_info("Nastran free successfully exported to %s", filename.c_str());

    return 0;
}

///--------------------------------------------
///  utr_io_write_nas_short - to read single .nas file (short format)
///--------------------------------------------
int utr_io_write_nas_short(const int Mesh_id,const char* Filename)
{

    using std::ios_base;
    using std::setw;


    std::string filename(Filename);
    filename.append(".nas");

     std::ofstream file(filename.c_str());
	 file.setf(ios_base::fixed);
    file  << "TITLE=ModFEM " << filename << "\n"
//		<< "CEND" << "\n"
       << "$" << "\n"
       << "BEGIN BULK" << "\n";



    // write points
    file  << "$" << "\n"
          << "$ Points" << "\n"
          << "$" << "\n";


    file.setf(ios_base::left);
    int iNode=0;


    std::stringstream tmp;
	tmp.setf(ios_base::fixed);
    tmp << std::showpoint;

    while(0!=(iNode=mmr_get_next_node_all(Mesh_id,iNode))) {

        double x[3]={0.0};
        mmr_node_coor(Mesh_id,iNode,x);

        // Fixed short/long formats:
       // 1 GRID
       // 2 ID   : point ID - requires starting index of 1
       // 3 CP   : co-ordinate system ID                (blank)
       // 4 X1   : point x cp-ordinate
       // 5 X2   : point x cp-ordinate
       // 6 X3   : point x cp-ordinate
       // 7 CD   : co-ordinate system for displacements (blank)
       // 8 PS   : single point constraints             (blank)
       // 9 SEID : super-element ID



        file  << setw(8) << "GRID";
        //file.unsetf(ios_base::left);
        //file.setf(ios_base::right);

		file << setw(8) << iNode
            << "        ";


        for(int i=0; i < 3; ++i) {
            tmp << setw(8) << x[i];
            file.write(tmp.str().c_str(),8);
            tmp.str( std::string() );
        }
        file << "\n";
        //file.unsetf(ios_base::right);

    }
    file.unsetf(ios_base::left);

int globalID = 0;

/*
      // write faces
      file  << "$" << "\n"
          << "$ Faces" << "\n"
          << "$" << "\n";

      int iFace=0;
      int face_type=0;
      std::string face_string;
      int facePts[MMC_MAXFAVNO+1]={0};
	  
    //  while(0!=(iFace=mmr_get_next_act_face(Mesh_id,iFace))){
    //    face_type = mmr_fa_type(Mesh_id,iFace);
    //      switch(face_type) {
    //    case 3: face_string = "CTRIA3"; // MMC_TRIA
    //        break;
    //    case 4: face_string = "CQUAD4"; // MMC_QUAD
    //        break;
    //    }


    //      mmr_fa_node_coor(Mesh_id,iFace,facePts,NULL);

    //      // Fixed short/long formats:
    //      // 1 CQUAD4
    //      // 2 EID  : element ID
    //      // 3 PID  : property element ID; default = EID   (blank)
    //      // 4 G1   : grid point index - requires starting index of 1
    //      // 5 G2   : grid point index
    //      // 6 G3   : grid point index
    //      // 7 G4   : grid point index
    //      // 8 onwards - not used

    //      // For CTRIA3 elements, cols 7 onwards are not used




    //      file.setf(ios_base::left);
    //      file  << setw(8) << face_string;
    //      file.unsetf(ios_base::left);
    //      file.setf(ios_base::right);
    //      //file  << setw(8) << iFace;
		  //file  << setw(8) << ++globalID;
    //      file << setw(8) << mmr_fa_bc(Mesh_id,iFace);

    //      for(int i=0; i < face_type; ++i)
    //      {
    //          file  << setw(8) << facePts[1+i];
    //      }

    //      file  << "\n";
    //      file.unsetf(ios_base::right);

    //  } //!while
*/
      // write element
      file  << "$" << "\n"
          << "$ Elements" << "\n"
          << "$" << "\n";

      std::string elem_string;
      int iElem=0;
      int el_nodes[MMC_MAXELVNO+1]={0};

      while(0 != (iElem=mmr_get_next_act_elem(Mesh_id,iElem))) {
          int type = mmr_el_type(Mesh_id,iElem);

          switch(type) {
          case MMC_TETRA: elem_string="CTETRA"; break;
          case MMC_PRISM: elem_string="CPENTA"; break;
          default: mf_fatal_err("Uknown element type(%d)!",type);
          }

          mmr_el_node_coor(Mesh_id,iElem,el_nodes,NULL);

          file.setf(ios_base::left);
          file  << setw(8) << elem_string;
          file.unsetf(ios_base::left);
          file.setf(ios_base::right);
		  //file << setw(8) << iElem;
          file << setw(8) << ++globalID;
          file << setw(8) << mmr_el_groupID(Mesh_id,iElem);

          for(int i=0; i < el_nodes[0]; ++i) {
              file  << setw(8) << el_nodes[1+i];
          }

          file  << "\n";
          file.unsetf(ios_base::right);

      }

    file  << "ENDDATA" << std::endl;

    mf_log_info("Nastran short successfully exported to %s", filename.c_str());
    return 0;
}

///--------------------------------------------
///  utr_io_write_nas_long - to read single .nas file (long format)
///--------------------------------------------
int utr_io_write_nas_long(const int Mesh_id,const char* Filename)
{

    using std::ios_base;
    using std::setw;

    std::string filename(Filename);
    filename.append(".nas");

    std::ofstream file(filename.c_str());
	file.setf(ios_base::fixed);

    file  << "TITLE=ModFEM " << filename << " mesh" << "\n"
       << "$" << "\n"
       << "BEGIN BULK" << "\n";



    // write points
    file  << "$" << "\n"
          << "$ Points" << "\n"
          << "$" << "\n";

    file << std::showpoint << std::setprecision(14);

    int iNode=0;
    while(0!=(iNode=mmr_get_next_node_all(Mesh_id,iNode))) {

        double x[3]={0.0};
        mmr_node_coor(Mesh_id,iNode,x);

        // Fixed short/long formats:
       // 1 GRID
       // 2 ID   : point ID - requires starting index of 1
       // 3 CP   : co-ordinate system ID                (blank)
       // 4 X1   : point x cp-ordinate
       // 5 X2   : point x cp-ordinate
       // 6 X3   : point x cp-ordinate
       // 7 CD   : co-ordinate system for displacements (blank)
       // 8 PS   : single point constraints             (blank)
       // 9 SEID : super-element ID


        file.setf(ios_base::left);
        file  << setw(8) << "GRID*";
//        file.unsetf(ios_base::left);
//        file.setf(ios_base::right);
        file  << setw(16) << iNode
            << "                "
            << setw(16) << x[0]
            << setw(16) << x[1]
            << setw(8) << "*"  << "\n";
        file.unsetf(ios_base::right);
        file.setf(ios_base::left);
        file  << setw(8) << "*";
        file.unsetf(ios_base::left);
        file.setf(ios_base::right);
        file  << setw(16) << x[2]
            << "\n";
        file.unsetf(ios_base::right);

    }




      // write faces
      file  << "$" << "\n"
          << "$ Faces" << "\n"
          << "$" << "\n";
      int iFace=0;
	  int glob_id = 0;
      int face_type=0;
      std::string face_string;
      int facePts[MMC_MAXFAVNO+1]={0};

      while(0!=(iFace=mmr_get_next_act_face(Mesh_id,iFace))){
        face_type = mmr_fa_type(Mesh_id,iFace);
          switch(face_type) {
        case 3: face_string = "CTRIA3"; // MMC_TRIA
            break;
        case 4: face_string = "CQUAD4"; // MMC_QUAD
            break;
        }


          mmr_fa_node_coor(Mesh_id,iFace,facePts,NULL);

          // Fixed short/long formats:
          // 1 CQUAD4
          // 2 EID  : element ID
          // 3 PID  : property element ID; default = EID   (blank)
          // 4 G1   : grid point index - requires starting index of 1
          // 5 G2   : grid point index
          // 6 G3   : grid point index
          // 7 G4   : grid point index
          // 8 onwards - not used

          // For CTRIA3 elements, cols 7 onwards are not used

          file.setf(ios_base::left);
          file  << setw(8) << (face_string.append("*"));
          file.unsetf(ios_base::left);
          file.setf(ios_base::right);
		  file << setw(16) << ++glob_id;
          file  << setw(16) << mmr_fa_bc(Mesh_id,iFace);

          for(int i=0; i < facePts[0]; ++i) {
              file  << setw(16) << facePts[1+i];
              if (i == 3) {
                  file  << setw(8)<< "*\n";
                  file.unsetf(ios_base::right);
                  file.setf(ios_base::left);
                  file  << setw(8) << "*";
                  file.unsetf(ios_base::left);
                  file.setf(ios_base::right);
              }
          }

          file  << "\n";
          file.unsetf(ios_base::right);

      } //!while

      // write element
      file  << "$" << "\n"
          << "$ Elements" << "\n"
          << "$" << "\n";

      std::string elem_string;
      int iElem=0;
      int el_nodes[MMC_MAXELVNO+1]={0};

      while(0 != (iElem=mmr_get_next_act_elem(Mesh_id,iElem))) {
          int type = mmr_el_type(Mesh_id,iElem);

          switch(type) {
          case MMC_TETRA: elem_string="CTETRA"; break;
          case MMC_PRISM: elem_string="CPENTA"; break;
          default: mf_fatal_err("Uknown element type(%d)!",type);
          }

          mmr_el_node_coor(Mesh_id,iElem,el_nodes,NULL);

          file.setf(ios_base::left);
          file  << setw(8) << (elem_string.append("*"));
          file.unsetf(ios_base::left);
          file.setf(ios_base::right);
		  file << setw(16) << ++glob_id;
          file << setw(16) << mmr_el_groupID(Mesh_id,iElem);

          for(int i=0; i < el_nodes[0]; ++i) {
              file  << setw(16) << el_nodes[1+i];
              if(i==2) { //next line
                  file  << setw(8) << "*\n";
                  file  << setw(8) << "*";
              }
          }

          file  << "\n";
          file.unsetf(ios_base::right);

      }


    file  << "ENDDATA" << std::endl;

    mf_log_info("Nastran long successfully exported to %s", filename.c_str());

    return 0;
}






///---------------------------------------------------------
/// utr_io_get_reading_function - to return pointer to appropriate reading function
///  for given mesh type
///---------------------------------------------------------
mmpt_io_fptr utr_io_get_reading_function(const char Mesh_type)
{
  static bool reading_functions_initialized=false;
  static mmpt_io_fptr reading_functions[MMC_MAX_FILE_TYPE]={NULL};

  /// IMPORTANT! Registering reading functions.
  if(reading_functions_initialized == false) {
	
	reading_functions[ MMC_MOD_FEM_HYBRID_DATA ]=utr_io_read_hybrid;
	reading_functions[ MMC_MOD_FEM_TETRA_DATA  ]=utr_io_read_hybrid;
	reading_functions[ MMC_MOD_FEM_MESH_DATA   ]=utr_io_read_dat;
	reading_functions[ MMC_MOD_FEM_PRISM_DATA  ]=utr_io_read_dat;
	reading_functions[ MMC_NASTRAN_DATA        ]=utr_io_read_nas;
    reading_functions[ MMC_NASTRAN_FREE_DATA   ]=utr_io_read_nas;
    reading_functions[ MMC_NASTRAN_SHORT_DATA  ]=utr_io_read_nas;
    reading_functions[ MMC_NASTRAN_LONG_DATA   ]=utr_io_read_nas;
    reading_functions[ MMC_GRADMESH_DATA       ]=utr_io_read_jk;
	reading_functions[ MMC_BINARY_DATA         ]=utr_io_read_binary;
	reading_functions[ MMC_IN_ANSYS_DATA       ]=utr_io_read_in;
    reading_functions[ MMC_PARAVIEW_VTK_DATA   ]=utr_io_read_vtk;
    reading_functions[ MMC_MSH_DATA            ]=utr_io_read_msh;
	
    /// notify that initialization is done
	reading_functions_initialized=true;
  }
  assert( reading_functions[static_cast<int>(Mesh_type)] != NULL );
  return reading_functions[static_cast<int>(Mesh_type)];
}

///---------------------------------------------------------
/// utr_io_get_writing_function - to return pointer to appropriate reading function
///  for given mesh type
///---------------------------------------------------------
mmpt_io_fptr utr_io_get_writing_function(const char Mesh_type)
{
  static bool writing_functions_initialized=false;
  static mmpt_io_fptr writing_functions[MMC_MAX_FILE_TYPE]={NULL};

  /// IMPORTANT! Registering reading functions.
  if(writing_functions_initialized == false) {

      memset(writing_functions,0,sizeof(mmpt_io_fptr)*MMC_MAX_FILE_TYPE);

///    writing_functions[ MMC_MOD_FEM_HYBRID_DATA ]=utr_io_write_hybrid;
///    writing_functions[ MMC_MOD_FEM_TETRA_DATA  ]=utr_io_write_hybrid;
///    writing_functions[ MMC_MOD_FEM_MESH_DATA   ]=utr_io_write_dat;
///    writing_functions[ MMC_MOD_FEM_PRISM_DATA  ]=utr_io_write_dat;
    writing_functions[ MMC_NASTRAN_DATA        ]=utr_io_write_nas;
    writing_functions[ MMC_NASTRAN_SHORT_DATA  ]=utr_io_write_nas_short;
    writing_functions[ MMC_NASTRAN_LONG_DATA   ]=utr_io_write_nas_long;
///    writing_functions[ MMC_GRADMESH_DATA       ]=utr_io_write_jk;
///    writing_functions[ MMC_BINARY_DATA         ]=utr_io_write_binary;
///    writing_functions[ MMC_IN_ANSYS_DATA       ]=utr_io_write_in;
///    writing_functions[ MMC_PARAVIEW_VTK_DATA   ]=utr_io_write_vtk;
    writing_functions[ MMC_MSH_DATA            ]=utr_io_write_msh;

    /// notify that initialization is done
    writing_functions_initialized=true;
  }
  //assert( writing_functions[static_cast<int>(Mesh_type)] != NULL );
  return writing_functions[static_cast<int>(Mesh_type)];
}





#endif /// _utr_io_intf_
