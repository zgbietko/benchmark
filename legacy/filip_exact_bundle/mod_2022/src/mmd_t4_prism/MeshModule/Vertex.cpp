#include "Vertex.h"
#include "hHybridMesh.h"

template<>
void Vertex::init(){}

template<>
void       Vertex::print() const
{
    hObj::print();
    std::cout << "(";
       for(int i=0; i < typeSpecyfic_.nCoords_;++i) {
           std::cout << coords_[i] << ",";
       }
       std::cout << ")\n";
}


void VertexSpace::mark2Ref(hObj& ,const int ) {}
void VertexSpace::mark2Deref(hObj& ){}
void VertexSpace::mark2Delete(hObj& obj){
    if(obj.nMyClassSons_ != hObj::delMark) {
        obj.nMyClassSons_ = hObj::delMark;
        obj.myMesh->vertices_.requestChange(-1, -sizeof(Vertex));
    }
}
int  VertexSpace::refine(hObj& ,const int ){ return 0;}
void VertexSpace::derefine(hObj& ){}
bool VertexSpace::test(const hObj& ){ return false;}
//ID	 VertexSpace::components(const hObj *ptr,const int i){assert(!"Not implemented");return 0;}

