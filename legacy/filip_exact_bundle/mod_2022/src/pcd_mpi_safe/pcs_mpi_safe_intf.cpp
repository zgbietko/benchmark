/************************************************************************
File pcs_mpi_intf.c
Contains definitions of routines: 
  pcr_init_parallel - to initialize parallel communication library
  pcr_my_proc_id - to get id of current process
  pcr_nr_proc - to get number of all processes  

  pcr_send_buffer_open - to open a buffer for sending data
  pcr_buffer_pack_int - to pack an array of integers to a buffer
  pcr_buffer_pack_double - to pack an array of doubles to a buffer
  pcr_buffer_pack_char - to pack an array of characters to a buffer
  pcr_buffer_send - to send a buffer to destination processor
  !pcr_buffer_start_send - to start sending a buffer to destination processor
  !pcr_buffer_finish_send - to finish sending a buffer to destination processor
  pcr_buffer_bcast - to broadcast the content of a buffer 

  pcr_buffer_receive - to receive a buffer 
  !pcr_buffer_start_receive - to start receiving a buffer
  !pcr_buffer_finish_receive - to finish receiving a buffer 
  pcr_buffer_unpack_int - to unpack an array of integers from a buffer
  pcr_buffer_unpack_double - to unpack an array of doubles from a buffer
  pcr_buffer_unpack_char - to unpack an array of characters from a buffer
  pcr_recv_buffer_close - to close a buffer for receiveing data

  pcr_send_int - to send without buffering an array of integers
  pcr_send_double - to send without buffering an array of doubles 
  pcr_receive_int - to receive without buffering an array of integers
  pcr_receive_double - to receive without buffering an array of doubles 
  pcr_bcast_double - to broadcast an array of doubles
  pcr_bcast_int - to broadcast an array of integers
  pcr_bcast_char - to broadcast an array of characyers
  pcr_allreduce_sum_int - to reduce (sum) and broadcast an array of integers 
  pcr_allreduce_sum_double - to reduce (sum) and broadcast an array of doubles
  pcr_allreduce_max_int - to reduce (max) and broadcast an array of integers 
  pcr_allreduce_max_double - to reduce (max) and broadcast an array of doubles
  pcr_exit_parallel - to exit parallel communication library
  pcr_barrier - to synchronise paralell execution
  pcr_is_parallel_initialized - to determine wheter parallelism is initialized or not

NOTE: the module is prepared to serve multiple buffers;
NOTE: the module is prepared to self-mf_check safety of buffers

////////////////// IMPORTANT INFO ////////////////////////////////
PROCESSORS (PROCESSES) ARE NUMBERED FROM 1 to NR_PROC !!!!!!!!!!!!
////////////////// !IMPORTANT INFO ////////////////////////////////
------------------------------  			
History:        
    11.2013 - Kazimierz Michalik, version 2.1
*************************************************************************/
#include<mpi.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<assert.h>
#include<stdint.h>
#include<vector>
#include<stack>
#include<memory>
//#include<random>

/* parallel communication interface specification */
#include "../include/pch_intf.h"
#include "../include/uth_log.h"

#ifdef __cplusplus
extern "C"{
#endif

const int PCC_DEFAULT_BUFFER_SIZE=1*1000000 + MPI_BSEND_OVERHEAD; /* 1 MB*/
const int PCC_MPI_ATTACHED_BUFFER_SIZE = 1000*1000000+ MPI_BSEND_OVERHEAD; // 1GB


/*** CONSTANTS ***/
const int PCC_ANY_PROC = -1; /* wildcard for arbitrary processor id */
const int PCC_USE_CURRENT_BUFFER = -1;
const int PCC_MASTER_PROC_ID = 1;

const int PCC_MSG_ID_CONTROL_DATA = 4444;
const int PCC_OK = MPI_SUCCESS;

/*** TYPEDEFS ***/
enum PCE_Type {
    PCE_INT=sizeof(int),
    PCE_CHAR=sizeof(char),
    PCE_DOUBLE=sizeof(double),
    PCE_LONG_INT=sizeof(long int)
};

typedef char PCT_BYTE;

struct pct_buffer{
  static const int control_data_default_size = 32,
  control_data_n_parts_pos = 0,
  control_data_total_size_pos = 1;
  int buffer_id;
  int message_id;
  int sender_rank; // in MPI style -> 0 <= sender_rank < nr_proc
  int receiver_rank; // in MPI style -> 0 <= sender_rank < nr_proc
  MPI_Request request;
  MPI_Status status;
  std::vector<PCT_BYTE> storage;
  std::vector<int> control_data;
  int position;
  int cur_part;

  pct_buffer() : buffer_id(-1),message_id(-1),sender_rank(-1),receiver_rank(-1),position(0),cur_part(0){
      storage.reserve(PCC_DEFAULT_BUFFER_SIZE);
      control_data.reserve(control_data_default_size);
  }

  inline int& cur_part_type()  {
      return control_data[2*(cur_part+1)+control_data_n_parts_pos];
  }

  inline int& cur_part_n_types()  {
    return control_data[2*(cur_part+1)+control_data_total_size_pos];
  }

  inline int& n_parts() {
      return control_data[control_data_n_parts_pos];
  }

  inline int& total_size() {
      return control_data[control_data_total_size_pos];
  }

  void clear() {
      message_id = -1;
      sender_rank = -1;
      receiver_rank = -1;
      storage.clear();
      control_data.clear();
      position =0;
      cur_part=0;
  }
};


// GLOBALS (internal)
FILE *pcv_output_stream;
// for implementation with buffered sends an internal MPI buffer is created
std::vector<PCT_BYTE> pcv_MPI_buffer;
std::vector<pct_buffer> pcv_buffers;
std::stack<int> pcv_empty_buffers;


/*** GLOBAL VARIABLES ***/
// my_proc_id = my_rank + 1 !!!!!!
int pcv_my_rank;
int pcv_nr_proc;
int pcv_my_proc_id;
 
/// Internal functions etc.
int pcr_print_mpi_error(int error_code);

inline int PCR_HANDLE_MPI(int mpi_return_code, const char * msg)
{
    if (mpi_return_code != MPI_SUCCESS) {

        char error_string[BUFSIZ];
        int length_of_error_string;

        MPI_Error_string(mpi_return_code, error_string, &length_of_error_string);
        mf_check(mpi_return_code == MPI_SUCCESS, "%s (proc %d). MPI Error: %s",msg, pcv_my_proc_id, error_string);
        //fprintf(stderr, "%3d: %s\n", pcv_my_rank, error_string);
        MPI_Abort(MPI_COMM_WORLD,mpi_return_code);
    }
    return mpi_return_code;
}

inline int pcr_get_free_buffer() {

    int buffer_id = -1;
    if(pcv_empty_buffers.empty()) {
        pcv_buffers.push_back(pct_buffer());
        buffer_id = pcv_buffers.size()-1;
    }
    else {
        buffer_id = pcv_empty_buffers.top();
        pcv_empty_buffers.pop();
    }
    mf_check(buffer_id > -1, "Unable to get free buffer!");
    pcv_buffers[buffer_id].clear();

    return buffer_id;
}

/*---------------------------------------------------------
  pcr_buffer_pack_type - to pack an array to a buffer
---------------------------------------------------------*/
inline int pcr_buffer_pack_type( /* returns: >=0 - success code, <0 - error code */
  const int Message_id,  /* in: message ID */
  const int Buffer_id,  /* in: buffer ID */
  const int Nr_types,     /* in: number of values to pack to the buffer */
  const PCT_BYTE* Bytes,    /* in: array of numbers to pack */
  const PCE_Type Type,
  const MPI_Datatype Mpi_type
)
{
    pct_buffer & buf = pcv_buffers.at(Buffer_id);
    mf_check_debug(buf.buffer_id == Buffer_id,"Mismatched buffer id!");
    mf_check_debug(buf.message_id == Message_id, "Mismatched message id!");
    //mf_check_debug( (Nr_types > 16), "pcr_mpi_safe::pack_type: small pack size (%d), packing and unpacking will be ineffecitve!",Nr_types);

    // getting part size
    int sizeof_pack=0;
    PCR_HANDLE_MPI( MPI_Pack_size(Nr_types, Mpi_type, MPI_COMM_WORLD, &sizeof_pack),"getting part size" );

    if( ( sizeof_pack+buf.position ) > buf.storage.size() ) {
        buf.storage.resize(buf.storage.size()+sizeof_pack);

        //mf_check(buf.storage.size() < PCC_MPI_ATTACHED_BUFFER_SIZE,"Message buffer exceeds MPI buffer!");
    }

    //mf_debug("Packing data (size %d) to message buffer (size %d).\n",sizeof_pack,buf.storage.size());

    // packing data
    PCR_HANDLE_MPI( MPI_Pack(const_cast<PCT_BYTE*>(Bytes),Nr_types,Mpi_type, buf.storage.data(),buf.storage.size(),&buf.position,MPI_COMM_WORLD) ,
                    "Packing data to message buffer failed.\n");


    // updateing control info for this data
    buf.control_data.resize(buf.control_data.size()+2);
    buf.cur_part_n_types() = Nr_types;
    buf.cur_part_type() =Type;

    // updating header of the whole buf
    ++buf.n_parts();
    buf.total_size()+=sizeof_pack;

    mf_check_debug(Nr_types == buf.cur_part_n_types(),
                   "Number of packet types(%d) for part(%d) incorrect (should be %d)!",Nr_types , buf.cur_part ,buf.cur_part_n_types());

    mf_check_debug(buf.cur_part_type() =Type,
                   "Packed type(%d) for part (%d) incorrect (should be %d)",
                   buf.cur_part_type(),buf.cur_part ,Type);

    // goto next part
    ++buf.cur_part;
    return(0);
}

/*---------------------------------------------------------
  pcr_buffer_unpack_type - to unpack an array from a buffer
---------------------------------------------------------*/
inline int pcr_buffer_unpack_type( /* returns: >=0 - success code, <0 - error code */
  int Message_id,  /* in: message ID */
  int Buffer_id,  /* in: buffer ID */
  int Nr_types,     /* in: number of values to pack to the buffer */
  PCT_BYTE* Bytes,    /* in: array of numbers to pack */
  PCE_Type Type,
  MPI_Datatype Mpi_type
)
{

  assert(Buffer_id > -1);
  pct_buffer& buf = pcv_buffers.at(Buffer_id);

  mf_check_debug( (Message_id == buf.message_id), "pcd_mpi_safe::given Message_id (%d): mismatched buffer message id (%d)!",Message_id,buf.message_id);
  mf_check_debug( Type == PCE_Type( buf.cur_part_type() ), "pcd_mpi_safe::unpack_type: wrong type requested to unpack!");
  mf_check_debug( Nr_types <= buf.cur_part_n_types() ,"pcd_mpi_safe::unpack_type: wrong size(%d) requested to unpack (recived size=%d)!",Nr_types,buf.cur_part_n_types());
  //mf_check_info( (Nr_types > 16) ,"pcr_mpi_safe::pack_type: small pack size (%d), packing and unpacking will be ineffecitve!",Nr_types);

  PCR_HANDLE_MPI( MPI_Unpack(buf.storage.data(), buf.storage.size(), &buf.position,
                             Bytes, Nr_types, Mpi_type, MPI_COMM_WORLD),"pcr_buffer_unpack_type" );

  ++buf.cur_part;
  return(0);
}



//////////////// Interface functions /////////////////////////

//------------------------------------------------------//
// pcr_is_parallel_initialized - to determine wheter parallelism is initialized or not
//------------------------------------------------------//
extern int pcr_is_parallel_initialized( //returns 1 - is initialized, 0 - is NOT initialized
void
)
{
  int is_mpi_initialized=0;
  return ( MPI_SUCCESS == MPI_Initialized(&is_mpi_initialized) );
}

//------------------------------------------------------//
// pcr_my_proc_rank - to get rank of current process (id = rank +1)
//------------------------------------------------------//
int pcr_my_proc_rank( //returns id of current process
 							  void
 						   )
{
   return pcv_my_rank;
}
 
//------------------------------------------------------//
// pcr_my_proc_id - to get id of current process (id = rank +1)
//------------------------------------------------------//
int pcr_my_proc_id( //returns id of current process
 							  void
 						   )
{
   return pcv_my_proc_id;
}
 
//------------------------------------------------------//
// pcr_nr_proc - to get number of all processes  
//------------------------------------------------------//
int pcr_nr_proc( //returns number of all process
 					  void
 					   )
{
  return pcv_nr_proc;
}
 
int pcr_test();

/*---------------------------------------------------------
  pcr_init_parallel - to initialize parallel communication library
---------------------------------------------------------*/
int pcr_init_parallel( /* returns: >=0 - success code, <0 - error code */
  int* argc,       /* in: */
  char** argv,     /* in: */
  char* Work_dir,  /* in: */
  char *interactive_output_name, /* in/out: */
  FILE **interactive_output_p, /* out: reset pointer to interactive output */
  int* Nr_pr,      /* out: number of processors/processes */
  int* My_id       /* out: local process ID */
  )
{
  

  int i, rc;
  FILE *interactive_output;

  MPI_Init(argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&pcv_my_rank);
  /*!!!!!!!!!!!!!!!!*/
  pcv_my_proc_id = pcv_my_rank+1;
  /*!!!!!!!!!!!!!!!!*/
  MPI_Comm_size(MPI_COMM_WORLD,&pcv_nr_proc);

  MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
  
  // set interactive output for parallel execution
  if(*interactive_output_p != stdout){
    sprintf(interactive_output_name,"%s_%d",
        interactive_output_name,pcv_my_proc_id);

    interactive_output = fopen(interactive_output_name,"w");
  }
  else interactive_output = *interactive_output_p;

  pcv_output_stream = interactive_output;

  mf_check( (interactive_output != NULL), "Cannot establish interactive output. Exiting\n" );

  *Nr_pr = pcv_nr_proc;
  *My_id = pcv_my_proc_id;

  //pcv_my_proc_id = pcv_my_proc_id;
  //pcv_nr_proc = pcv_nr_proc;

  // initialize internal MPI buffer
  // it is assumed that at most ten LARGE outgoing messages are buffered
  // i.e. each subdomain exchanges data with maximum 10 neighbouring subdomains
  pcv_MPI_buffer.resize( PCC_MPI_ATTACHED_BUFFER_SIZE );
  PCR_HANDLE_MPI( MPI_Buffer_attach( pcv_MPI_buffer.data(), pcv_MPI_buffer.size() ), "Buffer attach failed." );

  *interactive_output_p = interactive_output;

  mf_check_debug( pcr_test() == PCC_OK, "pcd_mpi_safe module autotest failed!" );

  return(0);
}


/*---------------------------------------------------------
pcr_print_master - to return the id of read/write master processor
---------------------------------------------------------*/
int pcr_print_master(
  /* returns the id of read/write master processor or error code <0 */
){

  return(PCC_MASTER_PROC_ID);
}


/*---------------------------------------------------------
  pcr_send_buffer_open - to open a buffer for sending data
---------------------------------------------------------*/
int pcr_send_buffer_open( 
  /* returns: >=0 - message buffer ID = success code, <0 - error code */
  const int Message_id,  /* in: message ID */
  int Buffer_size  /* in: message buffer size (0 for DEFAULT_BUFFER_SIZE) */
  )
{
    if(Buffer_size == 0) {
        Buffer_size = PCC_DEFAULT_BUFFER_SIZE;
    }

    int buffer_id=pcr_get_free_buffer();

    pct_buffer & buf = pcv_buffers.at(buffer_id);

    /* initialize communication buffer data structure */
  if(buf.storage.size() < Buffer_size ) {
       buf.storage.resize(Buffer_size);
  }
  buf.message_id = Message_id;
  buf.sender_rank = pcv_my_proc_id-1;
  buf.receiver_rank = -1;
  buf.message_id = Message_id;
  buf.buffer_id = buffer_id;

  // default header for whole module
  buf.control_data.resize(2);
  buf.control_data[buf.control_data_n_parts_pos]=0;
  buf.control_data[buf.control_data_total_size_pos]=0;

  return buffer_id;
}


/*---------------------------------------------------------
  pcr_buffer_pack_int - to pack an array of integers to a buffer
---------------------------------------------------------*/
int pcr_buffer_pack_int( /* returns: >=0 - success code, <0 - error code */
  const int Message_id,  /* in: message ID */
  const int Buffer_id,  /* in: buffer ID */
  const int Nr_num,     /* in: number of values to pack to the buffer */
  const int* Numbers    /* in: array of numbers to pack */
)
{
    return pcr_buffer_pack_type(Message_id,Buffer_id,Nr_num,reinterpret_cast<const PCT_BYTE*>(Numbers),PCE_INT,MPI_INT);
}

/*---------------------------------------------------------
  pcr_buffer_pack_double - to pack an array of doubles to a buffer
---------------------------------------------------------*/
int pcr_buffer_pack_double( /* returns: >=0 - success code, <0 - error code */
  const int Message_id,  /* in: message ID */
  const int Buffer_id,  /* in: message ID */
  const int Nr_num,     /* in: number of values to pack to the buffer */
  const double* Numbers /* in: array of numbers to pack */
)
{
    return pcr_buffer_pack_type(Message_id,Buffer_id,Nr_num,reinterpret_cast<const PCT_BYTE*>(Numbers),PCE_DOUBLE,MPI_DOUBLE);
}

/*---------------------------------------------------------
  pcr_buffer_pack_char - to pack an array of chars to a buffer
---------------------------------------------------------*/
int pcr_buffer_pack_char( /* returns: >=0 - success code, <0 - error code */
  const int Message_id,  /* in: message ID */
  const int Buffer_id,  /* in: message ID */
  const int Nr_num,     /* in: number of values to pack to the buffer */
  const char* Numbers /* in: array of numbers to pack */
)
{
    return pcr_buffer_pack_type(Message_id,Buffer_id,Nr_num,Numbers,PCE_CHAR,MPI_CHAR);
}

/*---------------------------------------------------------
  pcr_buffer_pack_long_int - to pack an array of long int to a buffer
---------------------------------------------------------*/
int pcr_buffer_pack_long_int( /* returns: >=0 - success code, <0 - error code */
  const int Message_id,  /* in: message ID */
  const int Buffer_id,  /* in: message ID */
  const int Nr_num,     /* in: number of values to pack to the buffer */
  const long int* Numbers /* in: array of numbers to pack */
)
{
    return pcr_buffer_pack_type(Message_id,Buffer_id,Nr_num,reinterpret_cast<const PCT_BYTE*>(Numbers),PCE_LONG_INT,MPI_LONG_INT);
}


/*---------------------------------------------------------
  pcr_buffer_send - to send a buffer to destination processor
---------------------------------------------------------*/
///
/// \brief pcr_buffer_send
/// \param Message_id
/// \param Buffer_id
/// \param Dest_proc
/// \return
///
/// A quick overview of MPI's send modes
// MPI has a number of different "send modes."
// These represent different choices of buffering (where is the data kept until it is received)
// and synchronization (when does a send complete).
// In the following, I use "send buffer" for the user-provided buffer to send.
// MPI_Send
// MPI_Send will not return until you can use the send buffer.
// It may or may not block (it is allowed to buffer, either on the sender or receiver side,
// or to wait for the matching receive).
// MPI_Bsend
// May buffer; returns immediately and you can use the send buffer. A late add-on to the MPI specification.
// Should be used only when absolutely necessary.
// MPI_Ssend
// will not return until matching receive posted
// MPI_Rsend
// May be used ONLY if matching receive already posted. User responsible for writing a correct program.
// MPI_Isend
// Nonblocking send. But not necessarily asynchronous.
// You can NOT reuse the send buffer until either a successful,
// wait/test or you KNOW that the message has been received (see MPI_Request_free).
// Note also that while the I refers to immediate, there is no performance requirement on MPI_Isend.
// An immediate send must return to the user without requiring a matching receive at the destination.
// An implementation is free to send the data to the destination before returning,
// as long as the send call does not block waiting for a matching receive.
// Different strategies of when to send the data offer different performance advantages
// and disadvantages that will depend on the application.
// MPI_Ibsend
// buffered nonblocking
// MPI_Issend
// Synchronous nonblocking. Note that a Wait/Test will complete only when the matching receive is posted.
// MPI_Irsend
// As with MPI_Rsend, but nonblocking.
// Note that "nonblocking" refers ONLY to whether the data buffer is available for reuse after the call. No part of the MPI specification,
// for example, mandates concurent operation of data transfers and computation.
// Some people have expressed concern about not having a single "perfect" send routine.
// But note that in general you can't write code in Fortran that will run at optimum speed
// on both Vector and RICS/Cache machines without picking different code for the different architectures.
// MPI at least lets you express the different algorithms, just like C or Fortran.

//Recommendations

//The best performance is likely if you can write your program so that you could use just MPI_Ssend;
// in that case, an MPI implementation can completely avoid buffering data.
// Use MPI_Send instead; this allows the MPI implementation the maximum flexibility in choosing how to deliver your data.
// (Unfortunately, one vendor has chosen to have MPI_Send emphasize buffering over performance; on that system, MPI_Ssend may perform better.)
// If nonblocking routines are necessary, then try to use MPI_Isend or MPI_Irecv.
// Use MPI_Bsend only when it is too inconvienent to use MPI_Isend.
// The remaining routines, MPI_Rsend, MPI_Issend, etc., are rarely used but may be of value in writing system-dependent message-passing code entirely within MPI.
///
int pcr_buffer_send( /* returns: >=0 - success code, <0 - error code */
  int Message_id,     /* in: message ID */
  int Buffer_id,  /* in: buffer ID */
  int Dest_proc   /* in : destination processor ID */
  )
{   
  mf_check_debug(Dest_proc >= 1, "pcr_send_int: Destination proc id out of range!");
  pct_buffer& buf= pcv_buffers.at(Buffer_id);
  assert(Message_id == buf.message_id);

  buf.receiver_rank = Dest_proc-1;

  mf_check(PCC_MPI_ATTACHED_BUFFER_SIZE > buf.control_data.size(),"Requesting to send buffer bigger( %dMB ) than MPI attached buffer( %dMB )",
           buf.control_data.size()/1000000,PCC_MPI_ATTACHED_BUFFER_SIZE/1000000);


  /* buffered blocking send */
  PCR_HANDLE_MPI( MPI_Send(buf.control_data.data(), buf.control_data.size(), MPI_INT, buf.receiver_rank, PCC_MSG_ID_CONTROL_DATA, MPI_COMM_WORLD),
                  "Failed to send control data!");


  mf_check(PCC_MPI_ATTACHED_BUFFER_SIZE > buf.position,"Requesting to send buffer bigger( %dMB ) than MPI attached buffer( %dMB )",
           buf.position/1000000,PCC_MPI_ATTACHED_BUFFER_SIZE/1000000);

  PCR_HANDLE_MPI( MPI_Send(buf.storage.data(), buf.position, MPI_PACKED, buf.receiver_rank, buf.message_id, MPI_COMM_WORLD),
                  "Failed to send buffer!");

  /* prepare buffer for new sends or receives */
  pcv_empty_buffers.push(buf.buffer_id);
  buf.clear();

  return(0);
}

/*---------------------------------------------------------
  pcr_buffer_source - to get the source of message in buffer
---------------------------------------------------------*/
int pcr_buffer_source_id(int Buffer)
{
    return pcv_buffers.at(Buffer).status.MPI_SOURCE+1;
}

/*---------------------------------------------------------
  pcr_buffer_receive - to receive a buffer from a particular processor
---------------------------------------------------------*/
int pcr_buffer_receive(
           /* returns: >= 0 - Buffer_id, <0 - error code */
  int Message_id,   /* in: id of the message containing the data */
  int Sender_proc,  /* in : sender processor ID */
  int Buffer_size /* in: message buffer size (0 for DEFAULT_BUFFER_SIZE) */
  )
{

    const int buffer_id = pcr_get_free_buffer();
    pct_buffer& buf = pcv_buffers.at(buffer_id);
    buf.buffer_id = buffer_id;
  buf.message_id = Message_id;

  if(Sender_proc == PCC_ANY_PROC) {
      buf.sender_rank = MPI_ANY_SOURCE;
  }
  else {
      mf_check_debug(Sender_proc >= 1, "pcr_send_int: Sender proc id out of range!");
      buf.sender_rank = Sender_proc-1;
  }
  buf.receiver_rank = pcv_my_proc_id-1;


  // Recving control data.
  int count=0;

  PCR_HANDLE_MPI( MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,& buf.status),
                  "Unable to wait for control data!");

  mf_check(buf.status.MPI_SOURCE == buf.sender_rank || buf.sender_rank == MPI_ANY_SOURCE,
           "Other sender (%d) then expected (%d)!", buf.status.MPI_SOURCE, buf.sender_rank);
  mf_check(buf.status.MPI_TAG == PCC_MSG_ID_CONTROL_DATA,
           "Other tag (%d) then expected (%d)",buf.status.MPI_TAG, PCC_MSG_ID_CONTROL_DATA);

  PCR_HANDLE_MPI( MPI_Get_count(&buf.status, MPI_INT, &count),
                  "Failed to recv control data size!");
  mf_check((count > 0), "Recived 0-size message!");

  buf.control_data.resize(count,0);
  PCR_HANDLE_MPI( MPI_Recv(buf.control_data.data(), buf.control_data.size(), MPI_INT, buf.sender_rank, PCC_MSG_ID_CONTROL_DATA, MPI_COMM_WORLD,& buf.status),
                  "Failed to recv control data contents!");

  // Recving buffer.
  PCR_HANDLE_MPI( MPI_Probe(buf.sender_rank, buf.message_id,MPI_COMM_WORLD, &buf.status),
                  "Failed to recv buffer!\n");
  PCR_HANDLE_MPI( MPI_Get_count(&buf.status, MPI_PACKED, &count), "Unable to get recv buffer size!");
  //mf_check(count == buf.control_data[pct_buffer::control_data_total_size_pos], "Incosistency while reciving buffer size!");

  buf.storage.resize(count);

  PCR_HANDLE_MPI( MPI_Recv(buf.storage.data(), buf.storage.size(), MPI_PACKED,
       buf.sender_rank, buf.message_id, MPI_COMM_WORLD, &buf.status),
                  "Failed to recv buffer contents!");

  return buf.buffer_id;
}

/*--------------------------------------------------------
  pcr_buffer_bcast - to broadcast a buffer to all processes
---------------------------------------------------------*/
int pcr_buffer_bcast( /* returns: >=0 - buffer_id, <0 - error code*/
                      int Message_id, /* in: message ID */
                      int Buffer_id,  /* in: buffer ID */
                      int Sender_proc /* in: sender processor ID */
                      )
{
    mf_check_debug(Sender_proc >= 1, "pcr_send_int: Sender proc id out of range!");

    const int buffer_id = (pcv_my_proc_id==Sender_proc) ? Buffer_id : pcr_get_free_buffer() ;

    pct_buffer& buf = pcv_buffers.at(buffer_id);

    if(pcv_my_proc_id!=Sender_proc){
        /* initialize communication buffer data structure */
        buf.message_id = Message_id;
        buf.sender_rank = Sender_proc-1;
        buf.receiver_rank = pcv_my_proc_id-1;
        buf.position = 0;
    }
    else {
        mf_check(Buffer_id > -1, "Wrong buffer id(%d)!", Buffer_id);
    }

    int control_data_size = buf.control_data.size();
    PCR_HANDLE_MPI( MPI_Bcast( &control_data_size, 1, MPI_INT, buf.sender_rank, MPI_COMM_WORLD),
                    "Failed to Bcast control data size!");

    if(pcv_my_proc_id != Sender_proc) {
        buf.control_data.resize(control_data_size);
    }

    PCR_HANDLE_MPI( MPI_Bcast(buf.control_data.data(), buf.control_data.size(),
                              MPI_INT,buf.sender_rank,MPI_COMM_WORLD),
                    "Failed to send control data using buffered Bcast!");

    if(pcv_my_proc_id != Sender_proc) {
        buf.storage.resize(buf.total_size());
    }

    PCR_HANDLE_MPI( MPI_Bcast(buf.storage.data(), buf.total_size(), MPI_PACKED,
                              buf.sender_rank, MPI_COMM_WORLD ),
                    "Bcast data transfer error!");


    if(pcv_my_proc_id==Sender_proc){
        buf.clear();
        pcv_empty_buffers.push(buf.buffer_id);
    }
    else {
        /* prepare buffer for unpacking */
        buf.position = 0;
    }

    return (buffer_id);
}


/*---------------------------------------------------------
  pcr_buffer_unpack_int - to unpack an array of integers from a buffer
---------------------------------------------------------*/
int pcr_buffer_unpack_int( /* returns: >=0 - success code, <0 - error code */
  int Message_id,  /* in: message ID */
  int Buffer_id,  /* in: buffer ID */
  int Nr_num,     /* in: number of values to pack to the buffer */
  int* Numbers    /* in: array of numbers to pack */
)
{
    return pcr_buffer_unpack_type(Message_id,Buffer_id,Nr_num,reinterpret_cast<PCT_BYTE*>(Numbers),PCE_INT,MPI_INT);
}

/*---------------------------------------------------------
  pcr_buffer_unpack_double - to unpack an array of integers from a buffer
---------------------------------------------------------*/
int pcr_buffer_unpack_double( /* returns: >=0 - success code, <0 - error code */
  int Message_id,  /* in: message ID */
  int Buffer_id,  /* in: buffer ID */
  int Nr_num,     /* in: number of values to unpack from the buffer */
  double* Numbers /* in: array of numbers to unpack */
)
{
    return pcr_buffer_unpack_type(Message_id,Buffer_id,Nr_num,reinterpret_cast<PCT_BYTE*>(Numbers),PCE_DOUBLE,MPI_DOUBLE);
}

/*---------------------------------------------------------
  pcr_buffer_unpack_char - to unpack an array of integers from a buffer
---------------------------------------------------------*/
int pcr_buffer_unpack_char( /* returns: >=0 - success code, <0 - error code */
  int Message_id,  /* in: message ID */
  int Buffer_id,  /* in: buffer ID */
  int Nr_num,     /* in: number of values to unpack from the buffer */
  char* Numbers /* in: array of numbers to unpack */
)
{

    return pcr_buffer_unpack_type(Message_id,Buffer_id,Nr_num,Numbers,PCE_CHAR,MPI_CHAR);
}

/*---------------------------------------------------------
  pcr_buffer_unpack_long_int - to unpack an array of integers from a buffer
---------------------------------------------------------*/
int pcr_buffer_unpack_long_int( /* returns: >=0 - success code, <0 - error code */
  int Message_id,  /* in: message ID */
  int Buffer_id,  /* in: buffer ID */
  int Nr_num,     /* in: number of values to unpack from the buffer */
  long int* Numbers /* in: array of numbers to unpack */
)
{

    return pcr_buffer_unpack_type(Message_id,Buffer_id,Nr_num,reinterpret_cast<PCT_BYTE*>(Numbers),PCE_LONG_INT,MPI_LONG_INT);
}

/*---------------------------------------------------------
  pcr_recv_buffer_close - to close a buffer for receiveing data
---------------------------------------------------------*/
int pcr_recv_buffer_close( /* returns: >=0 - success code, <0 - error code */
  int Message_id,  /* in: message ID */
  int Buffer_id  /* in: buffer ID */
  )
{

  pct_buffer& buf =  pcv_buffers.at(Buffer_id);

  mf_check(Message_id == buf.message_id,
           "Mismatched message id (%d) while closing buffer %d",Message_id,Buffer_id);

  buf.clear();
  pcv_empty_buffers.push(Buffer_id);

  return(PCC_OK);

}



/*---------------------------------------------------------
  pcr_send_int - to send an array of integers
---------------------------------------------------------*/
extern int pcr_send_int( 
                    /* returns: >=0 - success code, <0 - error code */
  const int Dest_proc_id, /* in : destination processor ID */
  const int Message_id,   /* in: id of the message containing the data */
  const int Nr_num,       /* in: number of values to pack to the buffer */
  const int* Numbers      /* in: array of numbers to pack */
)
{
    assert(Numbers != NULL);
    mf_check_debug(Dest_proc_id >= 1, "pcr_send_int: Destination proc id out of range!");
  return PCR_HANDLE_MPI( MPI_Send(const_cast<int*>(Numbers), Nr_num, MPI_INT,
       Dest_proc_id-1, Message_id, MPI_COMM_WORLD),
                         "pcr_send_int error");
}

/*---------------------------------------------------------
  pcr_send_double - to send an array of doubles 
---------------------------------------------------------*/
extern int pcr_send_double( 
                    /* returns: >=0 - success code, <0 - error code */
  const int Dest_proc_id, /* in : destination processor ID */
  const int Message_id,   /* in: id of the message containing the data */
  const int Nr_num,       /* in: number of values to send */
  const double* Numbers   /* in: array of numbers to send */
  )
{
    mf_check_debug(Dest_proc_id >= 1, "pcr_send_int: Destination proc id out of range!");
    assert(Numbers != NULL);
  return PCR_HANDLE_MPI( MPI_Send(const_cast<double*>(Numbers), Nr_num, MPI_DOUBLE,
       Dest_proc_id-1, Message_id, MPI_COMM_WORLD),
                         "pcr_send_double error");
}

/*---------------------------------------------------------
  pcr_receive_int - to receive an array of integers
---------------------------------------------------------*/
extern int pcr_receive_int( 
                    /* returns: >=0 - success code, <0 - error code */
  const int Sender_proc_id, /* in : destination processor ID */
  const int Message_id,   /* in: id of the message containing the data */
  const int Nr_num,       /* in: number of values to pack to the buffer */
  int* Numbers      /* in: array of numbers to pack */
  )
{
    mf_check_debug(Sender_proc_id >= 1, "pcr_send_int: Sender proc id out of range!");
    assert(Numbers != NULL);
  MPI_Status status;
  return PCR_HANDLE_MPI( MPI_Recv(const_cast<int*>(Numbers), Nr_num, MPI_INT, Sender_proc_id-1, Message_id,
                                  MPI_COMM_WORLD, &status),
                         "pcr_receive_int error");
}

/*---------------------------------------------------------
  pcr_receive_double - to receive an array of doubles 
---------------------------------------------------------*/
extern int pcr_receive_double( 
                    /* returns: >=0 - success code, <0 - error code */
  const int Sender_proc_id, /* in : destination processor ID */
  const int Message_id,   /* in: id of the message containing the data */
  const int Nr_num,       /* in: number of values to pack to the buffer */
  double* Numbers   /* in: array of numbers to pack */
  )
{
    mf_check_debug(Sender_proc_id >= 1, "pcr_send_int: Sender proc id out of range!");
    assert(Numbers != NULL);
  MPI_Status status;
  return PCR_HANDLE_MPI( MPI_Recv(Numbers, Nr_num, MPI_DOUBLE, Sender_proc_id-1, Message_id,
       MPI_COMM_WORLD, &status),
                         "pcr_receive_double error");
}

/*---------------------------------------------------------
  pcr_send_long - to send an array of integers
---------------------------------------------------------*/
extern int pcr_send_long(
                    /* returns: >=0 - success code, <0 - error code */
  const int Dest_proc_id, /* in : destination processor ID */
  const int Message_id,   /* in: id of the message containing the data */
  const int Nr_num,       /* in: number of values to send */
  const long int* Numbers   /* in: array of numbers to send */
  )
{
    mf_check_debug(Dest_proc_id >= 1, "pcr_send_int: Destination proc id out of range!");
    assert(Numbers != NULL);
  return PCR_HANDLE_MPI( MPI_Send(const_cast<long int*>(Numbers), Nr_num, MPI_LONG,
       Dest_proc_id-1, Message_id, MPI_COMM_WORLD),
                         "pcr_send_long error");
}

/*---------------------------------------------------------
  pcr_receive_long - to receive an array of integers
---------------------------------------------------------*/
extern int pcr_receive_long(
                    /* returns: >=0 - success code, <0 - error code */
  const int Sender_proc_id, /* in : destination processor ID */
  const int Message_id,   /* in: id of the message containing the data */
  const int Nr_num,       /* in: number of values to pack to the buffer */
  long int* Numbers      /* in: array of numbers to pack */
  )
{
    mf_check_debug(Sender_proc_id >= 1, "pcr_send_int: Sender proc id out of range!");
    assert(Numbers != NULL);
  MPI_Status status;
  return PCR_HANDLE_MPI( MPI_Recv(Numbers, Nr_num, MPI_LONG, Sender_proc_id-1, Message_id,
       MPI_COMM_WORLD, &status),
                         "pcr_receive_long error!");
}

/*---------------------------------------------------------
  pcr_send_bytes - to send an array of integers
---------------------------------------------------------*/
extern int pcr_send_bytes(
                    /* returns: >=0 - success code, <0 - error code */
  const int Dest_proc_id, /* in : destination processor ID */
  const int Message_id,   /* in: id of the message containing the data */
  const int Nr_bytes,       /* in: number of values to send */
  const uint8_t* Bytes   /* in: array of numbers to send */
  )
{
    mf_check_debug(Dest_proc_id >= 1, "pcr_send_int: Destination proc id out of range!");
    assert(Bytes != NULL);
  return PCR_HANDLE_MPI( MPI_Send(const_cast<uint8_t*>(Bytes), Nr_bytes, MPI_BYTE,
       Dest_proc_id-1, Message_id, MPI_COMM_WORLD),
                         "pcr_send_bytes error!");
}

/*---------------------------------------------------------
  pcr_receive_bytes - to receive an array of integers
---------------------------------------------------------*/
extern int pcr_receive_bytes(
                    /* returns: >=0 - success code, <0 - error code */
  const int Sender_proc_id, /* in : destination processor ID */
  const int Message_id,   /* in: id of the message containing the data */
  const int Nr_bytes,       /* in: number of values to pack to the buffer */
  uint8_t* Bytes      /* in: array of numbers to pack */
  )
{
    mf_check_debug(Sender_proc_id >= 1, "pcr_send_int: Sender proc id out of range!");
    assert(Bytes != NULL);
  MPI_Status status;
  return PCR_HANDLE_MPI( MPI_Recv(Bytes, Nr_bytes, MPI_BYTE, Sender_proc_id-1, Message_id,
       MPI_COMM_WORLD, &status),
                         "pcr_receive_bytes error");
}


/*--------------------------------------------------------
  pcr_bcast_double - to broadcast an array of doubles
---------------------------------------------------------*/
int pcr_bcast_double( /* returns: >=0 - success code, <0 - error code*/
  const int Sender_proc_id, /* in : source processor ID */
  const int Nr_num,       /* in: number of values to pack to the buffer */
  double* Numbers   /* in: array of numbers to pack */
  )    
{
    mf_check_debug(Sender_proc_id >= 1, "pcr_send_int: Sender proc id out of range!");
    return PCR_HANDLE_MPI( MPI_Bcast(Numbers,Nr_num,MPI_DOUBLE,Sender_proc_id-1,MPI_COMM_WORLD),
                    "pcr_bcast_double error");

}


/*--------------------------------------------------------
  pcr_bcast_int - to broadcast an array of integers
---------------------------------------------------------*/
int pcr_bcast_int( /* returns: >=0 - success code, <0 - error code*/
  int Sender_proc_id, /* in : source processor ID */
  int Nr_num,       /* in: number of values to pack to the buffer */
  int* Numbers      /* in: array of numbers to pack */
  )    
{
     mf_check_debug(Sender_proc_id >= 1, "pcr_send_int: Sender proc id out of range!");
    return PCR_HANDLE_MPI( MPI_Bcast(Numbers,Nr_num,MPI_INT,Sender_proc_id-1,MPI_COMM_WORLD),
                    "pcr_bcast_int error");

}


/*--------------------------------------------------------
  pcr_bcast_char - to broadcast an array of charegers
---------------------------------------------------------*/
int pcr_bcast_char( /* returns: >=0 - success code, <0 - error code*/
  int Sender_proc_id, /* in : source processor ID */
  int Nr_num,       /* in: number of values to pack to the buffer */
  char* Numbers      /* in: array of numbers to pack */
  )    
{
     mf_check_debug(Sender_proc_id >= 1, "pcr_send_int: Sender proc id out of range!");
    return PCR_HANDLE_MPI( MPI_Bcast(Numbers,Nr_num,MPI_CHAR,Sender_proc_id-1,MPI_COMM_WORLD),
                           "pcr_bcast_char error!");
}


/*---------------------------------------------------------
pcr_allreduce_sum_int - to reduce by summing and broadcast an array of integers 
---------------------------------------------------------*/
int pcr_allreduce_sum_int( /* returns: >=0 - success code, <0 - error code*/
  const int Nr_num,          /* in: number of values to reduce */
  const int* Numbers,        /* in: array of numbers to sum */
   int* Numbers_reduced /* in: array of summed numbers */
  )    
{

    assert(Numbers != Numbers_reduced);
  return PCR_HANDLE_MPI( MPI_Allreduce(const_cast<int*>(Numbers),Numbers_reduced,Nr_num,MPI_INT,MPI_SUM,MPI_COMM_WORLD),
                  "Allreduce error");
}


/*--------------------------------------------------------
pcr_allreduce_sum_double - to reduce by summing and broadcast an array of doubles
---------------------------------------------------------*/
int pcr_allreduce_sum_double(/* returns: >=0 - success code, <0 - error code*/
  const int Nr_num,          /* in: number of values to reduce */
  const double* Numbers,        /* in: array of numbers to sum */
  double *Numbers_reduced /* in: array of summed numbers */
  )
{
    assert(Numbers != Numbers_reduced);
 return PCR_HANDLE_MPI( MPI_Allreduce(const_cast<double*>(Numbers),Numbers_reduced,Nr_num,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD),
                 "Allreduce error");
}

/*---------------------------------------------------------
pcr_allreduce_max_int - to reduce by maxming and broadcast an array of integers 
---------------------------------------------------------*/
int pcr_allreduce_max_int( /* returns: >=0 - success code, <0 - error code*/
  const int Nr_num,          /* in: number of values to reduce */
  const int* Numbers,  /* in: array of numbers to max */
  int* Numbers_reduced /* in: array of maxmed numbers */
  )    
{
    assert(Numbers != Numbers_reduced);
  return PCR_HANDLE_MPI( MPI_Allreduce(const_cast<int*>(Numbers),Numbers_reduced,Nr_num,MPI_INT,MPI_MAX,MPI_COMM_WORLD),
                         "Allreduce error");

}


/*--------------------------------------------------------
pcr_allreduce_max_double - to reduce by maxming and broadcast an array of doubles
---------------------------------------------------------*/
int pcr_allreduce_max_double( /* returns: >=0 - success code, <0 - error code*/
  int Nr_num,          /* in: number of values to reduce */
  double* Numbers,        /* in: array of numbers to max */
  double* Numbers_reduced /* in: array of maxmed numbers */
  )    
{
    assert(Numbers != Numbers_reduced);
 return PCR_HANDLE_MPI( MPI_Allreduce(Numbers,Numbers_reduced,Nr_num,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD),
                 "Allreduce error");
}

/*---------------------------------------------------------
  pcr_exit_parallel - to exit parallel communication library
---------------------------------------------------------*/
int pcr_exit_parallel( /* returns: >=0 - success code, <0 - error code */
  )
{
    
 pcv_buffers.clear();
 // idiom for clean
 while(!pcv_empty_buffers.empty()) {
    pcv_empty_buffers.pop();
 }

 int size = pcv_MPI_buffer.size();
 PCR_HANDLE_MPI( MPI_Buffer_detach(&pcv_MPI_buffer, &size ),
                 "Buffer detach error!");
 pcv_MPI_buffer.clear();

  return ( MPI_Finalize() );
}

//------------------------------------------------------//
// pcr_barrier - to synchronise parallel execution
//------------------------------------------------------//
int pcr_barrier( // returns >=0 - success, <0- error code
						 void
						 )
{
  return PCR_HANDLE_MPI( MPI_Barrier(MPI_COMM_WORLD),
                         "Barrier error");
}

//------------------------------------------------------//
// pcr_allgather_int - gathers together values from a group of processors and distributes to all
//------------------------------------------------------//
int pcr_allgather_int( //returns success code
                              int const send_values[],
                              int const n_send_values,
                              int gathered_values[],
                              int const n_gathered_values

)
{
    return PCR_HANDLE_MPI( MPI_Allgather(const_cast<int*>(send_values),n_send_values,MPI_INT,
                         gathered_values,n_gathered_values,MPI_INT,MPI_COMM_WORLD),
            "Allgather error!");
}



#ifdef __cplusplus
}
#endif
