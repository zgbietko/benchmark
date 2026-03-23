#ifndef _ELEMT4_H_
#define _ELEMT4_H_

#include "Face3.h"

/** \addtogroup MMM_HYBRID Hybrid Mesh Module
 *  \ingroup MM
 *  @{
 */


namespace ElemT4Space{
	void mark2Ref(hObj& obj,const int i) ;
	void mark2Deref(hObj& obj);
	void mark2Delete(hObj& obj);
	int  refine(hObj& obj,const int i);
	void derefine(hObj& obj);
	bool test(const hObj& obj);

	enum eRefKind { eRef_67=0, eRef_49=1, eRef_58=2 };

	MMT_H_MESH_TYPE(ElemT4,7,4,Face3,4,3,0,9,6,4,0)

	//const EntityAttributes shared = EntityAttributes(7,4,4,3,0,7,4,4,0,
	//		&mark2Ref,&mark2Deref,&mark2Delete,&refine,&derefine,&test,&uniqueId);

	  const int faceNeigByEdge[4][3]= {{1,2,3},{0,2,3},{0,1,3},{0,1,2}};
};
// params2 is for internal type of element
typedef ElemT4Space::ElemT4       ElemT4;
typedef ElemT4Space::ElemT4D		ElemT4D;
/**  @} */
#endif // _ELEMT4_H_
