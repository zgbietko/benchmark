#define BOOST_TEST_MODULE ModFEM_test_util
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <string>

#include "uth_mat.h"
#include "uth_io_compression.h"

//int add( int i, int j ) { return i+j; }

//BOOST_AUTO_TEST_CASE( Calculator_simple_test )
//{
//    // seven ways to detect and report the same error:
//    BOOST_CHECK( add( 2,2 ) == 4 );        // #1 continues on error

//    BOOST_REQUIRE( add( 2,2 ) == 4 );      // #2 throws on error

//    if( add( 2,2 ) != 4 )
//      BOOST_ERROR( "Ouch..." );            // #3 continues on error

//    if( add( 2,2 ) != 4 )
//      BOOST_FAIL( "Ouch..." );             // #4 throws on error

//    if( add( 2,2 ) != 4 ) throw "Ouch..."; // #5 throws on error

//    BOOST_CHECK_MESSAGE( add( 2,2 ) == 4,  // #6 continues on error
//                         "add(..) result: " << add( 2,2 ) );

//    BOOST_CHECK_EQUAL( add( 2,2 ), 4 );	  // #7 continues on error
//}

BOOST_AUTO_TEST_SUITE( Utilities_materials )

BOOST_AUTO_TEST_CASE( materials_normal )
{
    std::string dir = "../../ctest/modfem_test/materials";
    BOOST_CHECK( UTE_FAIL != utr_mat_read(dir.c_str(),"materials.dat",stdout) );
    BOOST_CHECK( 3 == utr_mat_get_n_materials() );
}
BOOST_AUTO_TEST_CASE( materials_ill_formed_files )
{
    std::string dir = "../../ctest/modfem_test/materials";
    BOOST_CHECK( UTE_FAIL == utr_mat_read(dir.c_str(),"no_such_file.dat",stdout) );
    BOOST_CHECK( UTE_SUCCESS == utr_mat_read(dir.c_str(),"empty_file.dat",stdout) );
}
BOOST_AUTO_TEST_CASE( materials_additonal_files )
{
    std::string dir = "../../ctest/modfem_test/materials";
    BOOST_CHECK( UTE_FAIL != utr_mat_read(dir.c_str(),"1_material.dat",stdout) );
    BOOST_CHECK( 4 == utr_mat_get_n_materials() );
}
BOOST_AUTO_TEST_CASE( materials_normal_zero_id_bug_test )
{
    utr_mat_clear_all();
    std::string dir2 = "../../ctest/modfem_test/materials/zero_material";
    BOOST_CHECK( UTE_FAIL != utr_mat_read(dir2.c_str(),"materials.dat",stdout) );
    BOOST_CHECK( 1 == utr_mat_get_n_materials() );
    BOOST_CHECK( 1 == utr_mat_get_matID(1) );
    BOOST_CHECK( 1 == utr_mat_get_blockID(1) );
}
BOOST_AUTO_TEST_CASE( materials_no_groups )
{
    utr_mat_clear_all();
    std::string dir2 = "../../ctest/modfem_test/materials/zero_material";
    BOOST_CHECK( UTE_FAIL == utr_mat_read(dir2.c_str(),"no_such_file.dat",stdout) );
    BOOST_CHECK( 1 == utr_mat_get_n_materials() );
    BOOST_CHECK( 1 == utr_mat_get_matID(1) );
    BOOST_CHECK( UTE_GROUP_ID_DEFAULT_BLOCK == utr_mat_get_blockID(1) );
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( Utilities_io_compression )

BOOST_AUTO_TEST_CASE( backward_compatiliblity_no_compression )
{
    std::string dir = "../../ctest/modfem_test/compression/";
    BOOST_CHECK( 0 == utr_io_decompress_file(dir.c_str(),"LDC_z_005_20x20_nocomp.nas") );
}

BOOST_AUTO_TEST_CASE( decompression )
{
    std::string dir = "../../ctest/modfem_test/compression";
    BOOST_CHECK( utr_io_decompress_file(dir.c_str(),"LDC_z_005_20x20.nas.zip") );
    BOOST_CHECK( boost::filesystem::exists(dir+"/LDC_z_005_20x20.nas") );

}

BOOST_AUTO_TEST_CASE( compression_dir_with_filename )
{
    std::string dir = "../../ctest/modfem_test/compression";

    boost::filesystem::rename(dir+"/field90.dmp.ref", dir+"/field90.dmp");

    BOOST_CHECK( boost::filesystem::exists(dir+"/field90.dmp") );
    BOOST_CHECK( utr_io_compress_file(dir.c_str(),"field90.dmp") );
    BOOST_CHECK( boost::filesystem::exists(dir+"/field90.dmp.zip") );

    boost::filesystem::remove(dir+"/field90.dmp.zip");
    boost::filesystem::rename(dir+"/field90.dmp", dir+"/field90.dmp.ref");

}

BOOST_AUTO_TEST_CASE( decompression_filename_only )
{
    std::string file = "../../ctest/modfem_test/compression/LDC_z_005_20x20.nas";
    BOOST_CHECK( utr_io_decompress_file( NULL, (file+".zip").c_str() ) );
    BOOST_CHECK( boost::filesystem::exists(file) );

}


BOOST_AUTO_TEST_CASE( compression_filename_only )
{
    std::string dir = "../../ctest/modfem_test/compression";

    boost::filesystem::rename((dir+"/field90.dmp.ref"), (dir+"/field90.dmp") );

    BOOST_CHECK( boost::filesystem::exists(dir+"/field90.dmp") );
    BOOST_CHECK( utr_io_compress_file(NULL,(dir+"/field90.dmp").c_str()) );
    BOOST_CHECK( boost::filesystem::exists(dir+"/field90.dmp.zip") );

    boost::filesystem::remove(dir+"/field90.dmp.zip");
    boost::filesystem::rename(dir+"/field90.dmp", dir+"/field90.dmp.ref");

}

BOOST_AUTO_TEST_SUITE_END()

