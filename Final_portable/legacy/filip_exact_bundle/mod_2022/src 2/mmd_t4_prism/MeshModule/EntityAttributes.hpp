#ifndef _ENTITYATTRIBUTES_H_
#define _ENTITYATTRIBUTES_H_

#include "../Common.h"

/** \addtogroup MMM_HYBRID Hybrid Mesh Module
 *  \ingroup MM
 *  @{
 */


class hObj;
class hHybridMesh;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EntityAttributes
{
  EntityAttributes(//hHybridMesh & mesh,
				   const uTind myType=0,
		const uTind nComponents=0,
		const uTind nVertices=0,
		const uTind nFlags=0,
		const uTind nNeighs=0,
		const uTind nSons=0,
		const uTind nEdges=0,
		const uTind nFaces=0,
		const uTind nCoords=0,
		void (*const mark2Ref)(hObj& obj,const int i) = NULL,
		void (*const mark2Deref)(hObj& obj)= NULL,
		void (*const mark2Delete)(hObj& obj)= NULL,
		int  (*const refine)(hObj& obj,const int i)= NULL,
		void (*const derefine)(hObj& obj)= NULL,
		bool (*const test)(const hObj& obj)= NULL,
		ID	 (*const uniqueId)(void) = NULL) :   
	//	m(mesh),
	myType(myType),
		nComponents_(nComponents),
		nVerts_(nVertices),
		nFlags_(nFlags),
		nNeighs_(nNeighs),
		nSons_(nSons),	
		nEdges_(nEdges),
		nFaces_(nFaces),
	nCoords_(nCoords),
	compOffset_(nVertices),
		flagOffset_(nVertices+nComponents),
		neighOffset_(nVertices+nComponents+nFlags_),
	sonsOffset_(nVertices+nComponents+nFlags_+nNeighs_),
		mark2Ref_(mark2Ref),
		mark2Deref_(mark2Deref),
		mark2Delete_(mark2Delete),
		refine_(refine),
		derefine_(derefine),
		test_(test),
		uniqueId_(uniqueId)
		{}
  
  const uTind myType,
	nComponents_,nVerts_,nFlags_,nNeighs_,nSons_,
	nEdges_,nFaces_,nCoords_,
	compOffset_,flagOffset_,neighOffset_,sonsOffset_;
  
  //  hHybridMesh & m;
  void (*const mark2Ref_)(hObj& obj,const int i);
	void (*const mark2Deref_)(hObj& obj);
	void (*const mark2Delete_)(hObj& obj);
	int  (*const refine_)(hObj& obj,const int i);
	void (*const derefine_)(hObj& obj);
	bool (*const test_)(const hObj& obj);
  ID	 (*const uniqueId_)(void);

	
};
/**  @} */
#endif // _ENTITYATTRIBUTES_H_
