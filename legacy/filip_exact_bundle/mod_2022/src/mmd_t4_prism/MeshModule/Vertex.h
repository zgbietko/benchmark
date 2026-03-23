#ifndef _VERTEX_H_
#define _VERTEX_H_

#include "EntityAttributes.hpp"
#include "hParent.hpp"

/** \addtogroup MMM_HYBRID Hybrid Mesh Module
 *  \ingroup MM
 *  @{
 */


namespace VertexSpace{

	void mark2Ref(hObj& obj,const int i) ;
	void mark2Deref(hObj& obj);
	void mark2Delete(hObj& obj);
	int  refine(hObj& obj,const int i);
	void derefine(hObj& obj);
	bool test(const hObj& obj);

	MMT_H_MESH_TYPE(Vertex1,0,0,BYTE,0,1,0,0,0,0,3)

	//const EntityAttributes shared(0,0,0,1,0,0,0,0,3,
	//	&mark2Ref,&mark2Deref,&mark2Delete,&refine,&derefine,&test,&uniqueId);
};

//typedef hParent<0,0,0,BYTE,1,0,3>           Vertex;
typedef VertexSpace::Vertex1 Vertex;

/**  @} */
#endif // _VERTEX_H_
