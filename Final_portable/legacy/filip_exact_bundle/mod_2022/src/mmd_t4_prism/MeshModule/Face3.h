#ifndef _FACE3_H_
#define _FACE3_H_

#include "Edge.h"

/** \addtogroup MMM_HYBRID Hybrid Mesh Module
 *  \ingroup MM
 *  @{
 */


/// faces_
/// components = edges_
/// params_[0-1]	= neighbours IDs
/// params2_[B_COND] = BC
namespace Face3Space{
	void mark2Ref(hObj& obj,const int i) ;
	void mark2Deref(hObj& obj);
	void mark2Delete(hObj& obj);
	int  refine(hObj& obj,const int i);
	void derefine(hObj& obj);
	bool test(const hObj& obj);
		
	MMT_H_MESH_TYPE(Face3,3,3,Edge,3,2,2,3,3,0,0)

	//const EntityAttributes shared(3,3,3,2,2,3,3,0,0,
	//		&mark2Ref,&mark2Deref,&mark2Delete,&refine,&derefine,&test,&uniqueId);
};
typedef Face3Space::Face3     Face3;
typedef Face3Space::Face3D     Face3D;

/**  @} */
#endif // _FACE3_H_
