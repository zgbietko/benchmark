#include "hParent.hpp"
#include "hHybridMesh.h"

/*template<int I, int nTVerts, int nTComponents, typename TComponents,
int nTFlags, int nTSons, int nTCoords, int nTNeighs>
void	hParent<I,nTVerts,nTComponents,TComponents,nTFlags,nTSons,nTCoords,nTNeighs>::updateHash(const uTind vertices[])
{
    switch(type_)
    {
	case 1:
	case 2: myMesh->registerEdge(vertices,pos_); break;
	case 3:
	case 4: myMesh->registerFace(vertices,pos_); break;
    }
}*/

void EmptyHParentSpace::mark2Ref(hObj* ,const int ){ throw "Not implemented";};
void EmptyHParentSpace::mark2Deref(hObj* ){ throw "Not implemented";};
void EmptyHParentSpace::mark2Delete(hObj* ){ throw "Not implemented";};
int  EmptyHParentSpace::refine(hObj* ,const int ){ throw "Not implemented";return 0;};
void EmptyHParentSpace::derefine(hObj* ){ throw "Not implemented";};
bool EmptyHParentSpace::test(const hObj* ){ throw "Not implemented";return true;};
ID	 EmptyHParentSpace::components(const hObj* obj,const int i) { throw "Not implemented";return 0;};
