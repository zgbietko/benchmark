#ifndef _UTH_IO_COMPRESSION_H_
#define _UTH_IO_COMPRESSION_H_

/**
 \defgroup UTM_IO_COMPRESSION IO Compression Utilities
 \ingroup UTM

  @{
 */
#ifdef __cplusplus
extern "C"
{
#endif

int utr_io_decompress_file(const char* Work_dir, const char* Filename);

int utr_io_compress_file(const char* Work_dir, const char* Filename); 
 
 
#ifdef __cplusplus
}
#endif
 
 /** @} */ // end of group
#endif //_UTH_IO_COMPRESSION_H_