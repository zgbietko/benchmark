#include "Mesh.h"
#include "Vec3D.h"
#include "Ray.h"
#include "Geometry.h"
#include "RenderParams.h"
#include "Object.h"
#include "ViewManager.h"
#include "CutPlane.h"
#include "elem_tables.h"
#include "../../include/Enums.h"
#include "../../utils/fv_exception.h"
//#include "mmh_intf.h"
#include "defs.h"
#include "fv_config.h"
#include "fv_timer.h"
#include "Log.h"
#include "uth_log.h"
#ifdef _USE_FV_EXT_MOD
#include "../../include/ApproxModule.h"
#endif
#include "elem_tables.h"
#include "../../include/fv_compiler.h"
#include "../../utils/fv_assert.h"
#include "MathHelper.h"
#include "VtxAccumulator.h"
#include "VtxPrimitiveAccumulator.h"

// include system
#include <iostream>
#include <algorithm>
#include <omp.h>

#ifdef FV_DEBUG
#include<fstream>
#include<stdio.h>
#include"fv_conv_utils.h"
#endif


namespace FemViewer {

const int Mesh::tetra[4][3] = { {0,2,1},{0,1,3},{0,3,2},{1,2,3} };
const int Mesh::prizm[5][4] = { {0,2,1,-1},{3,4,5,-1},{0,1,4,3},{1,2,5,4},{2,0,3,5} };


Mesh::Mesh()
: BaseMesh("Unknown")
, _pobject(0)
, _GLid(-1)
, _arElems()
, _arAllElements()
, _bbox()
, _bbox_cut()
, _nno(0)
, _ned(0)
, _nfa(0)
, _nel(0)
, _lnel(0)
, _ianno(0)
{
	//mfp_log_debug("Mesh ctr with unknown name\n");
}


Mesh::Mesh(const std::string& name_)
: BaseMesh(name_)
, _pobject(0)
, _GLid(-1)
, _arElems()
, _arAllElements()
, _bbox()
, _bbox_cut()
, _nno(0)
, _ned(0)
, _nfa(0)
, _nel(0)
, _lnel(0)
, _ianno(0)
{
	//mfp_log_debug("Mesh ctr\n");
	if (Init(name_.c_str()) < 0) throw fv_exception("Can't init mesh data!");
}

Mesh::~Mesh()
{
	//mfp_log_debug("Mesh dtr\n");
	Reset();
}

int Mesh::Init(const char* fname_)
{
	//mfp_log_debug("Init\n");
	if (BaseMesh::Init(fname_) < 0) return(-1);
	_nno = BaseMesh::Get_nmno(this->idx());
	_ned = BaseMesh::Get_nmed(this->idx());
	_nfa = BaseMesh::Get_nmfa(this->idx()); 
	_nel = BaseMesh::Get_nmel(this->idx());
	//mfp_debug("Before extracting\n");
	if (test_plane_cut)  {
		CutPlane plane(cut_planes[current_plane_idx][0],
				cut_planes[current_plane_idx][1],
				cut_planes[current_plane_idx][2],
				cut_planes[current_plane_idx][3]);

		SetActiveBoundaryElems(*this,&plane);
	} else {
	_lnel = SetElements(*this,false);
	}
	//ExtractMeshBBoxFromElements(false);
	//mfp_debug("Before extracting\n");
	_bbox = ExtractMeshBBoxFromNodes();
	//assert(_lnel == _pgrid->numOfElements);
	//mfp_debug("Numebr of inactive nodes = %u\n",_ianno);
	return(1);

}



//int Mesh::Free()
//{
//	Reset();
//	return this->BaseMesh::Free();
//}

int Mesh::Reload()
{
	Reset();
	return Init(this->_name.c_str());
}

int Mesh::Update()
{
	return(-1);
}

void Mesh::Reset()
{
	//mfp_log_debug("Reset\n");
	BaseMesh::Reset();
	_arAllElements.clear();
	_bbox.Reset();
	_bbox_cut.Reset();
	_nno = _ned = _nfa = _nel = 0;
	_pobject = NULL;
	_lnel = 0;

}


bool Mesh::ExtractMeshBBoxFromElements(const bool section_)
{

	//rmesh_._bbox_cut.Reset();
	BBox3D lbbox;

	arElConstIter  itr_b = this->_arAllElements.begin();
	arElConstIter  itr_e = this->_arAllElements.end();
	this->_bbox.Reset();

	while (itr_e != itr_b)
	{
		lbbox = GetElemBBox3D(itr_b->nodes);
		if (IS_BOUND(itr_b->elemId.id)) {
			this->_bbox += lbbox;
		}
		++itr_b;
	}
	/*{
		float xmn(_bbox.Xmin());
		float ymn(_bbox.Ymin());
		float zmn(_bbox.Zmin());
		float xmx(_bbox.Xmax());
		float ymx(_bbox.Ymax());
		float zmx(_bbox.Zmax());
		mfp_debug("Mesh bbox: min {%f, %f %f} max {%f, %f, %f}\n",xmn,ymn,zmn,xmx,ymx,zmx);
	}*/
	return this->_bbox.isInitialized();
}

BBox3D Mesh::ExtractMeshBBoxFromNodes()
{
	//mfp_log_debug("extracting bounding box of the mesh\n");
	BBox3D lbbox;
	fvmath::CVec3d lcoord;
	_ianno = 0;
	for (int i=1; i <= _nno; ++i) {
		if ( MMC_ACTIVE == Get_node_status(_idx,i)) {
			GetNodeCoor(i,lcoord.v);
			//std::cout << lcoord << std::endl;
			lbbox += lcoord;
		} else {
			++_ianno;
		}
	}
	assert(lbbox.isInitialized());
	//std::cout << lbbox;
	return lbbox;
}

//Fill in a vector of graphic elements
// from boundary faces of elements

int Mesh::SetActiveBoundaryElems(Mesh& rmesh,const CutPlane* plane)
{
	int nel=0,cnt,act,Fa;
	int faces[MMC_MAXELFAC+1];
	int Fa_neig[2];
	double Coords[MMC_MAXELVNO*3];
	Elem_Info el_info;
	const int id(rmesh.idx());
	rmesh._arAllElements.clear();
	//rmesh._arAllElements.resize(rmesh._nel);
	fv_timer t;
	t.start();
	while ((nel=Get_next_act_el(rmesh.idx(), nel)) > 0)
	{
		int nno = Get_el_node_coor(id,nel,el_info.nodes,Coords);
		//mfp_check_debug((el_info.nodes[0]==6||el_info.nodes[0]==4), "Incorrect number of nodes (%d) in element (%d)\n",el_info.nodes[0],nel);
		// Get the ids of faces
		Get_el_faces(id,nel,faces,NULL);
		assert((*faces == 5) || (*faces == 4));
		// Resets the counter of boundary faces for the elment
		cnt = act = 0;
		el_info.elemId = 0;
		for (int j = 1; j <= *faces; ++j) {
			Fa = fvmath::abs(faces[j]);
			Get_face_neig(id,Fa,Fa_neig,NULL,NULL,NULL,NULL,NULL);
			if (Fa_neig[1] == MMC_BOUNDARY) {
				SET_STFACEN(el_info.elemId.id,(j-1));
				++cnt;
			}
			if (act > 0) continue;
			// Check for quad faces
			if (*faces == 5) {

				if (j > 3) { // Quad faces
					int ib1,ib2,ie1,ie2;
					switch(j) {
					case 4: ib1 = 0; ib2 = 1; ie1 = 4; ie2 = 3; break;
					case 5: ib1 = 1; ib2 = 2; ie1 = 5; ie2 = 4; break;
					case 6: ib1 = 2; ib2 = 0; ie1 = 3; ie2 = 5; break;
					}
					int res_v0 = plane->CheckLocation(&Coords[ib1]);
					int res_v1 = plane->CheckLocation(&Coords[ie1]);
					// If both
					if (res_v0 + res_v1 ==0) ++act;
					else {
						res_v0 = plane->CheckLocation(&Coords[ib2]);
						res_v1 = plane->CheckLocation(&Coords[ie2]);
						if (res_v0 + res_v1 == 0) ++act;
					}
					//if (act) printf("dla sciany%d\n",j-1);
				} else {
					int offset = (j == 1) ? 0 : 3;
					int res_v0 = plane->CheckLocation(&Coords[offset + 0]);
					int res_v1 = plane->CheckLocation(&Coords[offset + 1]);
					int res_v2 = plane->CheckLocation(&Coords[offset + 2]);
					if (res_v0 + res_v1 + res_v2 == 0) ++act;
					//if (act) printf("dla postawy %d\n",j-1);
				}
			}
			else {
				// Test for tetrahedrons
				assert(!"Test for tera not implemented");
			}
		}// end for
		//if (act) printf("Elemactywny %d\n",nel);
		// Set boundary flag
		if (cnt) SET_BOUND(el_info.elemId.id);
		// Set active flag in element is selected by plane
		if (act) SET_ACTIVE(el_info.elemId.id);

		if (cnt || act) {
			if (*faces == 4) SET_TETRA(el_info.elemId.id);
			el_info.elemId.id |= fvmath::abs(nel);
			rmesh._arAllElements.push_back(el_info);
		}
		// insert element into vector
	}
	double stop = t.get_duration_sec();
	mfp_log_info("selected %u elementss form %d total\nTime: %lf sec",rmesh._arAllElements.size(),rmesh._nel,stop);
	return( 1);
}


uint32_t Mesh::SetElements(Mesh & rmesh,const bool boundary)
{
	//mfp_log_debug("SetElems\n");
	int nel=0,cnt,Fa;
	int faces[MMC_MAXELFAC+1];
	int Fa_neig[2];
	double Coords[MMC_MAXELVNO*3];
	Elem_Info el_info;
	const int id(rmesh.idx());
	uint32_t totalNumOfNodes = 0;
	rmesh._arAllElements.clear();
	fv_timer t;
	t.start();
	//rmesh._arAllElements.resize(rmesh._nel);
	while ((nel=Get_next_act_el(rmesh.idx(), nel)) > 0)
	{
		// Get elemenent verices
		int nno = Get_el_node_coor(id,nel,el_info.nodes,Coords);
		//mfp_debug("id = %d nel = %d nno = %d\n",id,nel,nno);
		//assert(nno==6||nno==4);
		//mfp_check_debug((el_info.nodes[0]==6||el_info.nodes[0]==4), "Incorrect number of nodes (%d) in element (%d)\n",el_info.nodes[0],nel);

		// Get the ids of faces
		Get_el_faces(id,nel,faces,NULL);
		assert((*faces == 5) || (*faces == 4));
		// Resets the counter of boundary faces for the elment
		cnt = 0;
		el_info.elemId = 0;
		for (int j = 1; j <= *faces; ++j) {
			Fa = fvmath::abs(faces[j]);
			Get_face_neig(id,Fa,Fa_neig,NULL,NULL,NULL,NULL,NULL);
			if (Fa_neig[1] == MMC_BOUNDARY) {
				SET_STFACEN(el_info.elemId.id,(j-1));
				++cnt;
			}
		}// end for
		// Skip element if not boundary
		if (!cnt && boundary) continue;
		// Set boundary flag
		if (cnt) SET_BOUND(el_info.elemId.id);
		// set type of element
		if (*faces == 4) SET_TETRA(el_info.elemId.id);
		// insert element into vector
		el_info.elemId.id |= fvmath::abs(nel);
		//printf("%d adding elem %d eith id %d\n",rmesh.GetNumElems(),nel,el_info.elemId.eid);
		rmesh._arAllElements.push_back(el_info);
		totalNumOfNodes += nno;
	}//end while
	//exit(-1);
	//rmesh._arAllElements.segregate();
	//mfp_log_debug("Toatal number of nodes = %d\n",rmesh._arAllElements.size());
	double stop = t.get_duration_sec();
	mfp_log_info("selected %u elementss form %d total\nTime: %lf sec",rmesh._arAllElements.size(),rmesh._nel,stop);
	return totalNumOfNodes + rmesh._arAllElements.size();
}


bool Mesh::Render(const uint_t options,Object* hObj,CutPlane* plane,arBndElems* vbnd)
{
	FV_ASSERT(hObj != NULL);
	FV_ASSERT(_nno > 0);
	std::vector<CVec3f> lVertices(_nno);
	CVec3f lcoord;
	_ianno = 0;
	// Convert vertices to its floats coordinates
	for (int i=1; i <= _nno; ++i) {
		if ( MMC_ACTIVE == Get_node_status(_idx,i)) {
			GetNodeCoor(this->idx(),lcoord.v);
			lVertices.push_back(lcoord);
		} else {
			++_ianno;
		}
	}
	hObj->GetCurrentVtxAccumulator();

	return true;
}

bool Mesh::RenderWireframe(VtxAccumulator *accum) const
{
	//mfp_log_debug("RenderWireframe\n");
	assert(accum != nullptr);
	assert(_nno > 0);
	assert(_ned > 0);
	// Store Nodes
	Vertex v;
	int vertex_counter = 0;
	assert(accum->getVertices().size() == 0);
	for (int ino = 1; ino <= _nno; ino++)
	{
		if (GetNodeCoor<float>(ino,v.position.v) < 0) continue;
		v.info = Vertex::info_t(ino);
		accum->addVertex(v);
		//std::cout<< "vt[" << ino << "] = " << v.position << std::endl;
		++vertex_counter;
	}
	mfp_check_debug((_nno == vertex_counter),"Warrning: there are %d inanctive nodes!\n",_nno - vertex_counter);
	// Store a number of mesh nodes
	accum->getVertexCounter() = vertex_counter;
	//mfp_debug("%u %u\n",vertex_counter,accum->getVertices().size());
	assert(accum->getVertexCounter() == accum->getVertices().size());

	// Store barycenters of boundary faces.
	// Append vertices table in VtxAccumulator
	int nr_faces;
	const int *elem_p(nullptr);
	double inv_div;
	// Here we compute geometry center of each face for text
	fvmath::CVec3d origin, node;
	arElConstIter it = _arAllElements.begin();
	const arElConstIter it_e = _arAllElements.end();
	while (it != it_e)
	{
		if (IS_BOUND(it->elemId.id)) {
			// Check a type of an element
			if (it->elemId.is_tetra()) {
				nr_faces = 4;
				elem_p = Mesh::tetra[0];
			}
			else {
				nr_faces = 5;
				elem_p = Mesh::prizm[0];
			}
			//for (int i(0); i < it->nodes[0]; ++i) std::cout << "n" << i << " = " << it->nodes[i+1] << std::endl;
			for (int nfa(0); nfa < nr_faces; ++nfa) {
				// Skip a non boundary face
				if (!IS_SET_FACEN(it->elemId.id, nfa)) continue;
				int of = nfa*(nr_faces-1);
				fvmath::CVec3d origin, node;
				// Get offsets in table of nodes
				int ino = elem_p[of++]+1;
				GetNodeCoor(it->nodes[ino],origin.v);
				//std::cout <<"first: " << origin;
				ino = elem_p[of++]+1;
				GetNodeCoor(it->nodes[ino],node.v);
				//std::cout <<"second: " << node;
				origin += node;
				ino = elem_p[of++]+1;
				GetNodeCoor(it->nodes[ino],node.v);
				//std::cout <<"third: " << node;
				origin += node;
				double inv_div = 0.333333;
				if (nr_faces == 5 && nfa >= 2) {
					ino = elem_p[of]+1;
					GetNodeCoor(it->nodes[ino],node.v);
					//std::cout <<"forth: " << origin;
					origin += node;
					inv_div = 0.25;
				}
				assert(it->nodes[0] > 0);
				origin *= inv_div;
				v.position = origin;
				//std::cout << "adding center " << origin << std::endl;
				// Store element id
				v.info = Vertex::info_t(it->elemId.eid);
				accum->addVertex(v,vtxEdge);
			}
		}
		++it;
	}
	// Store indices of edges
	unsigned int edges[2];
	for (int ied = 1; ied <= _ned; ++ied)
	{
		if (GetEdgeStatus(ied) > 0) {
			GetEdgeNodes(ied,edges);
			--edges[0];
			--edges[1];
			accum->addEdge(edges[0],edges[1]);
		}
	}
	//accum->aRenderEdges = true;
	accum->setEdgeColor( CVec3f(1.f, 0.f, 1.f));

	return true;
}
// this is experimental for single pass wireframe rendering
bool Mesh::RenderWireframe2() const
{
	FV_ASSERT(_pobject != NULL);

	VtxAccumulator& vtxacc(_pobject->GetCurrentVtxAccumulator());
	//vtxacc.UseVBO() = ViewManagerInst().IsVBOUsed();
	// Store Nodes
	fvmath::CVec3f v;
	for (int ino = 1; ino <= _nno; ino++)
	{
		GetNodeCoor<float>(ino,v.v);
		_pobject->AddMeshNode(v);
	}
	// Store triangle indices
	unsigned int edges[2];
	int nfa = 0;
	int Nodes[MMC_MAXFAVNO+1];
	while((nfa = Get_next_act_fa(idx(),nfa)) > 0) {
		Get_fa_node_coor(idx(),nfa,Nodes,NULL);
		int ii = 1;
		_pobject->AddIndexOfNode(static_cast<unsigned int>(abs(Nodes[ii++]) - 1),vtxTriangle); // 0
		_pobject->AddIndexOfNode(static_cast<unsigned int>(abs(Nodes[ii++]) - 1),vtxTriangle); // 1
		_pobject->AddIndexOfNode(static_cast<unsigned int>(abs(Nodes[ii]) - 1),vtxTriangle);   // 2
		if (Nodes[0]==4) {
			_pobject->AddIndexOfNode(static_cast<unsigned int>(abs(Nodes[ii++]) - 1),vtxTriangle); // 2
			_pobject->AddIndexOfNode(static_cast<unsigned int>(abs(Nodes[ii]) - 1),vtxTriangle);   // 3
			_pobject->AddIndexOfNode(static_cast<unsigned int>(abs(Nodes[1]) - 1),vtxTriangle);    // 0
		}
	}
	//vtxacc.createEdges();
	vtxacc.aRenderEdges = true;
	vtxacc.setEdgeColor( CVec3f(1.f, 1.f, 1.f));

	return true;
}



bool Mesh::RenderWireframeCutted(Object* hObj_,CutPlane* plane_) const
{

	int Ed_nodes[2];
	double Node_coords[18],u;
	double elem[24+16];
	double *Node1 = &Node_coords[0], *Node2 = &Node_coords[3];
	Vec3D p1, p2, p3, p4;

	// Set line and width for borders
	hObj_->SetDrawColor(ColorRGB(1.0f,0.0f,1.0f));
	hObj_->SetLineWidth(1.5f);

	for(size_type i = 1; i <= _ned; i++)
	{
		if (Get_edge_status(this->idx(),i) == MMC_ACTIVE)
		{
			Get_edge_nodes(this->idx(),i,Ed_nodes);
			Get_node_coor(this->idx(),Ed_nodes[0],Node1);
			Get_node_coor(this->idx(),Ed_nodes[1],Node2);

			int res1 = plane_->CheckLocation(Node1);
			int res2 = plane_->CheckLocation(Node2);

			if ((res1 + res2) > 0) continue;
			if (res1 > 0) {
				Node2 = &Node_coords[0];
				Node1 = &Node_coords[3];
			}

			// Intersection point Node2 or Node2 is unchanged so
			// both nodes are included
			plane_->IntersectWithLine(Node2,Node1,u);

			p1[0] = static_cast<float>(Node1[0]);
			p1[1] = static_cast<float>(Node1[1]);
			p1[2] = static_cast<float>(Node1[2]);

			p2[0] = static_cast<float>(Node2[0]);
			p2[1] = static_cast<float>(Node2[1]);
			p2[2] = static_cast<float>(Node2[2]);

			FV_ASSERT(!(p1 == p2));
			hObj_->AddLine(p1,p2);

		}// if
	}// end for


	std::vector<GraphElement2<double> > vels;

	for(size_t i(0); i < _arElems.nactive(); i++)
	{
		assert(IS_ACTIVE(_arElems(i).id));

		const uint_t nno = Get_el_node_coor(this->idx(),_arElems(i).id,NULL,Node_coords);
		for(uint_t ino(0);ino<nno;ino++)
		{
			register int ino4 = (ino << 2);
			register int ino3 = ino * 3;
			elem[ino4++] = Node_coords[ino3++];
			elem[ino4++] = Node_coords[ino3++];
			elem[ino4++] = Node_coords[ino3];
			elem[ino4  ] = -1.0;
		}

		//Get_el_faces(this->_id,nel,El_faces,NULL);
		// odesparowac od boundary faces
		int res;
		if (IS_TETRA(_arElems(i).id)) {
			res = DoSliceTerahedron(plane_,NULL,elem,vels,NULL);
		} else {
			res = DoSlicePrizm(plane_,NULL,elem,vels,NULL);
		}

		if (res > 0) {
			p1[0] = static_cast<float>(vels[0].points[0].x);
			p1[1] = static_cast<float>(vels[0].points[0].y);
			p1[2] = static_cast<float>(vels[0].points[0].z);

			p2[0] = static_cast<float>(vels[0].points[1].x);
			p2[1] = static_cast<float>(vels[0].points[1].y);
			p2[2] = static_cast<float>(vels[0].points[1].z);

			p3[0] = static_cast<float>(vels[0].points[2].x);
			p3[1] = static_cast<float>(vels[0].points[2].y);
			p3[2] = static_cast<float>(vels[0].points[2].z);

			hObj_->AddLine(p1,p2);
			hObj_->AddLine(p2,p3);
			if (res == 1) {
				hObj_->AddLine(p3,p1);
			} else if (res == 2) {
				p4[0] = static_cast<float>(vels[1].points[2].x);
				p4[1] = static_cast<float>(vels[1].points[2].y);
				p4[2] = static_cast<float>(vels[1].points[2].z);

				hObj_->AddLine(p3,p4);
				hObj_->AddLine(p4,p1);
			}
		}
	}// end for


	return true;
}

//bool Mesh::RenderWireframeCutted(Object* hObj,CutPlane* plane) const
//{
//	Vec3D p1, p2, p3, p4;
//	int Nodes[7];
//	int el_struct[20],fa_struct[20];
//	int El_faces[MMC_MAXELFAC];
//	int Fa;
//	int idx;
//	double Coords[18], elem[24+16];
//
//	struct inf {
//		double p[3];
//		int state;
//	};
//
//	inf edges[3];
//	int tab[3][2] = { {0,1},{1,2},{2,0} };
//	int res,result[4];
//	std::vector<GraphElement2<double> > vels;
//
//	// Set line and width for borders
//	hObj->SetDrawColor(ColorRGB(1.0f,0.0f,1.0f));
//	hObj->SetLineWidth(1.5f);
//
//	for(size_t i(0); i < _arElems.nactive(); i++)
//	{
//		assert(IS_ACTIVE(_arElems(i).el_id));
//		//const uint_t nel = ELEM_ID(_arElems(i).el_id);
//
//		const uint_t nno = Get_el_node_coor(this->idx(),_arElems(i).id,NULL,Coords);
//		for(uint_t ino(0);ino<nno;ino++)
//		{
//			register int ino4 = (ino << 2);
//			register int ino3 = ino * 3;
//			elem[ino4++] = Coords[ino3++];
//			elem[ino4++] = Coords[ino3++];
//			elem[ino4++] = Coords[ino3];
//			elem[ino4  ] = -1.0;
//		}
//
//		Get_el_faces(this->_id,nel,El_faces,NULL);
//		// odesparowac od boundary faces
//
//		uint8_t bound_fa = 0;
//
//
////		for (uint_t ino(1);ino<=k;++ino)
////		{
////
////			const int idxFa = fvmath::abs(El_faces[ino]);
////			Get_face_struct(this->_id,idxFa,fa_struct);
////			if (fa_struct[8] != MMC_BOUNDARY) continue;
////
////			for (int j(0);j<k;++j)
////				if (el_struct[5+j] == idxFa)
////				{
////					Nodes[0]++;
////					Nodes[l++] = j;
////					break;
////				}
////
////				FV_ASSERT(Nodes[0] <= k);
//		//}
//		vels.clear();
//
//		switch (k)
//		{
//		case 4:
//		// Section through the element
//		if ((res = DoSliceTerahedron(plane,NULL,elem,vels,NULL)) > 0)
//		{
//			/*if(vels.size() == 0) continue;*/
//			p1[0] = static_cast<float>(vels[0].points[0].x);
//			p1[1] = static_cast<float>(vels[0].points[0].y);
//			p1[2] = static_cast<float>(vels[0].points[0].z);
//
//			p2[0] = static_cast<float>(vels[0].points[1].x);
//			p2[1] = static_cast<float>(vels[0].points[1].y);
//			p2[2] = static_cast<float>(vels[0].points[1].z);
//
//			p3[0] = static_cast<float>(vels[0].points[2].x);
//			p3[1] = static_cast<float>(vels[0].points[2].y);
//			p3[2] = static_cast<float>(vels[0].points[2].z);
//
//			hObj->AddLine(p1,p2);
//			hObj->AddLine(p2,p3);
//			if (res == 1) {
//				hObj->AddLine(p3,p1);
//			} else if (res == 2) {
//				p4[0] = static_cast<float>(vels[1].points[2].x);
//				p4[1] = static_cast<float>(vels[1].points[2].y);
//				p4[2] = static_cast<float>(vels[1].points[2].z);
//
//				hObj->AddLine(p3,p4);
//				hObj->AddLine(p4,p1);
//			}
//		}
//
//		//continue;
//		//plane->CheckLocation(
//		for(unsigned int j = 1; j<=k;++j)
//		{
//			Fa = El_faces[j];
//			const int nno = Get_fa_node_coor(this->_id,Fa,Nodes,Coords);
//
//			res = 0;
//			for (register int y(0);y<nno;++y) {
//				result[y] = plane->CheckLocation(&Coords[3*y]);
//				res += result[y];
//			}
//
//			if (res == nno) continue;
//
//			Get_face_struct(this->_id,(k+j),fa_struct);
//
//			if ((res == -nno && fa_struct[8] == MMC_BOUNDARY) || (result[0] == 0 && result[1] == 0 && result[2] == 0))
//			{
//				p1[0] = static_cast<float>(Coords[0]);
//				p1[1] = static_cast<float>(Coords[1]);
//				p1[2] = static_cast<float>(Coords[2]);
//
//				p2[0] = static_cast<float>(Coords[3]);
//				p2[1] = static_cast<float>(Coords[4]);
//				p2[2] = static_cast<float>(Coords[5]);
//
//				p3[0] = static_cast<float>(Coords[6]);
//				p3[1] = static_cast<float>(Coords[7]);
//				p3[2] = static_cast<float>(Coords[8]);
//
//				hObj->AddLine(p1,p2);
//				hObj->AddLine(p2,p3);
//				hObj->AddLine(p3,p1);
//			}
//
//			else if (fa_struct[8] == MMC_BOUNDARY)
//			{
//
//				edges[0].state = plane->GetCuttedLine(plane->GetParams(),&Coords[0],&Coords[3],edges[0].p);
//
//				if (edges[0].state == 2)
//				{
//					p1[0] = static_cast<float>(Coords[0]);
//					p1[1] = static_cast<float>(Coords[1]);
//					p1[2] = static_cast<float>(Coords[2]);
//
//					p2[0] = static_cast<float>(Coords[3]);
//					p2[1] = static_cast<float>(Coords[4]);
//					p2[2] = static_cast<float>(Coords[5]);
//
//					hObj->AddLine(p1,p2);
//				}
//				else if (edges[0].state == 0)
//				{
//					p1[0] = static_cast<float>(Coords[0]);
//					p1[1] = static_cast<float>(Coords[1]);
//					p1[2] = static_cast<float>(Coords[2]);
//
//					p2[0] = static_cast<float>(edges[0].p[0]);
//					p2[1] = static_cast<float>(edges[0].p[1]);
//					p2[2] = static_cast<float>(edges[0].p[2]);
//
//					hObj->AddLine(p1,p2);
//				}
//				else if (edges[0].state == 1)
//				{
//					p1[0] = static_cast<float>(Coords[3]);
//					p1[1] = static_cast<float>(Coords[4]);
//					p1[2] = static_cast<float>(Coords[5]);
//
//					p2[0] = static_cast<float>(edges[0].p[0]);
//					p2[1] = static_cast<float>(edges[0].p[1]);
//					p2[2] = static_cast<float>(edges[0].p[2]);
//
//					hObj->AddLine(p1,p2);
//				}
//
//				edges[1].state = plane->GetCuttedLine(plane->GetParams(),&Coords[3],&Coords[6],edges[1].p);
//				if (edges[1].state == 2)
//				{
//					p1[0] = static_cast<float>(Coords[3]);
//					p1[1] = static_cast<float>(Coords[4]);
//					p1[2] = static_cast<float>(Coords[5]);
//
//					p2[0] = static_cast<float>(Coords[6]);
//					p2[1] = static_cast<float>(Coords[7]);
//					p2[2] = static_cast<float>(Coords[8]);
//
//					hObj->AddLine(p1,p2);
//				}
//				else if (edges[1].state == 0)
//				{
//					p1[0] = static_cast<float>(Coords[3]);
//					p1[1] = static_cast<float>(Coords[4]);
//					p1[2] = static_cast<float>(Coords[5]);
//
//					p2[0] = static_cast<float>(edges[1].p[0]);
//					p2[1] = static_cast<float>(edges[1].p[1]);
//					p2[2] = static_cast<float>(edges[1].p[2]);
//
//					hObj->AddLine(p1,p2);
//				}
//				else if (edges[1].state == 1)
//				{
//					p1[0] = static_cast<float>(Coords[6]);
//					p1[1] = static_cast<float>(Coords[7]);
//					p1[2] = static_cast<float>(Coords[8]);
//
//					p2[0] = static_cast<float>(edges[1].p[0]);
//					p2[1] = static_cast<float>(edges[1].p[1]);
//					p2[2] = static_cast<float>(edges[1].p[2]);
//
//					hObj->AddLine(p1,p2);
//				}
//				edges[2].state = plane->GetCuttedLine(plane->GetParams(),&Coords[6],&Coords[0],edges[2].p);
//				if (edges[2].state == 2)
//				{
//					p1[0] = static_cast<float>(Coords[0]);
//					p1[1] = static_cast<float>(Coords[1]);
//					p1[2] = static_cast<float>(Coords[2]);
//
//					p2[0] = static_cast<float>(Coords[6]);
//					p2[1] = static_cast<float>(Coords[7]);
//					p2[2] = static_cast<float>(Coords[8]);
//
//					hObj->AddLine(p1,p2);
//				}
//				else if (edges[2].state == 1)
//				{
//					p1[0] = static_cast<float>(Coords[0]);
//					p1[1] = static_cast<float>(Coords[1]);
//					p1[2] = static_cast<float>(Coords[2]);
//
//					p2[0] = static_cast<float>(edges[2].p[0]);
//					p2[1] = static_cast<float>(edges[2].p[1]);
//					p2[2] = static_cast<float>(edges[2].p[2]);
//
//					hObj->AddLine(p1,p2);
//				}
//				else if (edges[2].state == 0)
//				{
//					p1[0] = static_cast<float>(Coords[6]);
//					p1[1] = static_cast<float>(Coords[7]);
//					p1[2] = static_cast<float>(Coords[8]);
//
//					p2[0] = static_cast<float>(edges[2].p[0]);
//					p2[1] = static_cast<float>(edges[2].p[1]);
//					p2[2] = static_cast<float>(edges[2].p[2]);
//
//					hObj->AddLine(p1,p2);
//				}
//
//				const bool b0 = (edges[0].state == 0 || edges[0].state == 1);
//				const bool b1 = (edges[1].state == 0 || edges[1].state == 1);
//				const bool b2 = (edges[2].state == 0 || edges[2].state == 1);
//
//				idx = -1;
//
//				if (b0 && b1) idx = 0;
//				else if(b1 && b2) idx = 1;
//				else if(b2 && b0) idx = 2;
//
//				if(idx == -1) continue;
//
//				p1[0] = static_cast<float>(edges[ tab[idx][0] ].p[0]);
//				p1[1] = static_cast<float>(edges[ tab[idx][0] ].p[1]);
//				p1[2] = static_cast<float>(edges[ tab[idx][0] ].p[2]);
//
//				p2[0] = static_cast<float>(edges[ tab[idx][1] ].p[0]);
//				p2[1] = static_cast<float>(edges[ tab[idx][1] ].p[1]);
//				p2[2] = static_cast<float>(edges[ tab[idx][1] ].p[2]);
//
//				hObj->AddLine(p1,p2);
//			}
//		}
//		break;
//		case 5:
//			if ((res = DoSlicePrizm(plane,NULL,elem,vels,NULL)) > 0) {
//				p1[0] = static_cast<float>(vels[0].points[0].x);
//				p1[1] = static_cast<float>(vels[0].points[0].y);
//				p1[2] = static_cast<float>(vels[0].points[0].z);
//
//				p2[0] = static_cast<float>(vels[0].points[1].x);
//				p2[1] = static_cast<float>(vels[0].points[1].y);
//				p2[2] = static_cast<float>(vels[0].points[1].z);
//
//				p3[0] = static_cast<float>(vels[0].points[2].x);
//				p3[1] = static_cast<float>(vels[0].points[2].y);
//				p3[2] = static_cast<float>(vels[0].points[2].z);
//
//				hObj->AddLine(p1,p2);
//				hObj->AddLine(p2,p3);
//				if (res == 1) {
//					hObj->AddLine(p3,p1);
//				} else if (res == 2) {
//					p4[0] = static_cast<float>(vels[1].points[1].x);
//					p4[1] = static_cast<float>(vels[1].points[1].y);
//					p4[2] = static_cast<float>(vels[1].points[1].z);
//
//					hObj->AddLine(p3,p4);
//					hObj->AddLine(p4,p1);
//				}
//			}
//			double pt[32],u;
//			for(unsigned int j = 1; j<=Nodes[0];++j)
//			{
//				register int pa = prizm[ Nodes[j] ][0] << 2;
//				register int pb = prizm[ Nodes[j] ][1] << 2;
//				register int pc = prizm[ Nodes[j] ][2] << 2;
//
//				if (Nodes[j] <2)
//				{
//					result[0] = plane->CheckLocation(&elem[pa]);
//					result[1] = plane->CheckLocation(&elem[pb]);
//					result[2] = plane->CheckLocation(&elem[pc]);
//
//					if (result[0] <= 0 && result[1] <= 0 && result[2] <= 0) {
//						p1[0] = static_cast<float>(elem[pa++]);
//						p1[1] = static_cast<float>(elem[pa++]);
//						p1[2] = static_cast<float>(elem[pa]);
//
//						p2[0] = static_cast<float>(elem[pb++]);
//						p2[1] = static_cast<float>(elem[pb++]);
//						p2[2] = static_cast<float>(elem[pb]);
//
//						p3[0] = static_cast<float>(elem[pc++]);
//						p3[1] = static_cast<float>(elem[pc++]);
//						p3[2] = static_cast<float>(elem[pc]);
//
//						hObj->AddLine(p1,p2);
//						hObj->AddLine(p2,p3);
//						hObj->AddLine(p3,p1);
//
//						continue;
//					}
//
//					if (result[0]*result[1]>=0) {
//						std::swap(result[2],result[0]);
//						std::swap(pc,pa);
//						std::swap(result[1],result[2]);
//						std::swap(pb,pc);
//					} else if (result[0]*result[2]>=0) {
//						std::swap(result[1],result[0]);
//						std::swap(pb,pa);
//						std::swap(result[1],result[2]);
//						std::swap(pb,pc);
//					}
//
//					plane->IntersectWithLine(&elem[pb],&elem[pa],&pt[0],u);
//					plane->IntersectWithLine(&elem[pc],&elem[pa],&pt[3],u);
//
//					p1[0] = static_cast<float>(pt[0]);
//					p1[1] = static_cast<float>(pt[1]);
//					p1[2] = static_cast<float>(pt[2]);
//
//					p2[0] = static_cast<float>(pt[3]);
//					p2[1] = static_cast<float>(pt[4]);
//					p2[2] = static_cast<float>(pt[5]);
//
//					hObj->AddLine(p1,p2);
//
//					if (result[0] < 0) {
//						p3[0] = static_cast<float>(elem[pa++]);
//						p3[1] = static_cast<float>(elem[pa++]);
//						p3[2] = static_cast<float>(elem[pa]);
//
//						hObj->AddLine(p2,p3);
//						hObj->AddLine(p3,p1);
//					} else {
//
//						p3[0] = static_cast<float>(elem[pb++]);
//						p3[1] = static_cast<float>(elem[pb++]);
//						p3[2] = static_cast<float>(elem[pb]);
//
//						p4[0] = static_cast<float>(elem[pc++]);
//						p4[1] = static_cast<float>(elem[pc++]);
//						p4[2] = static_cast<float>(elem[pc]);
//
//						hObj->AddLine(p1,p3);
//						hObj->AddLine(p3,p4);
//						hObj->AddLine(p4,p2);
//					}
//
//				} else {
//					int key_id = 0;
//					for (register int k(0); k< 4; ++k) {
//						pa = (prizm[ Nodes[j] ][k] << 2);
//						if (plane->CheckLocation(&elem[pa]) < 1)
//							key_id |= (1<<k);
//					}
//
//					if (key_id == 0) continue;
//					int ed_flg = iQuadFlags[key_id];
//
//					for (register int k(0);k<4;++k) {
//						pa = (prizm[ Nodes[j] ][k] << 2);
//						memcpy(&pt[k<<2],&elem[pa],3*sizeof(double));
//					}
//
//					for (register int k(0); k<4; ++k) {
//						if (ed_flg & (1<<k)) {
//							register int m = (k << 2) + 16;
//							pa = iQuadEdgesIds[k][0] << 2;
//							pb = iQuadEdgesIds[k][1] << 2;
//							plane->IntersectWithLine(&pt[pa],&pt[pb],&pt[m],u);
//						}
//					}
//
//					const int *i1, *i2;
//					const int *p = &iQuadEdges[key_id][0];
//					for (register int k(1); k<=*p;++k) {
//						if (k == *p) {
//							i1 = p + 1;
//							i2 = p + k;
//						} else {
//							i1 = p + k;
//							i2 = i1 + 1;
//						}
//
//						pa = *i1 << 2;
//
//						p1[0] = static_cast<float>(pt[pa++]);
//						p1[1] = static_cast<float>(pt[pa++]);
//						p1[2] = static_cast<float>(pt[pa]);
//
//						pa = *i2 << 2;
//						p2[0] = static_cast<float>(pt[pa++]);
//						p2[1] = static_cast<float>(pt[pa++]);
//						p2[2] = static_cast<float>(pt[pa]);
//
//						hObj->AddLine(p1,p2);
//					}
//
//				}
//
//			}
//			break;
//		default:
//			std::cerr<<"Error! unknown element type: " << el_struct[0] << std::endl;
//			exit(-1);
//		}
//	}
//
//	return true;
//}


uint32_t Mesh::FillArrayWithIndicesOfElemNodes(const Mesh *pMesh,int Indices[])
{
	assert(Indices != NULL);
	Mesh::arElConstIter itb(pMesh->GetElems().begin());
	Mesh::arElConstIter ite(pMesh->GetElems().end());
	uint32_t index(0);
	for (uint32_t numNodes(0);itb != ite; ++itb, index += (numNodes+1))
	{
		numNodes = static_cast<uint32_t>(itb->nodes[0]);
		Indices[0] = -numNodes;
		for (uint32_t i(1);i <= numNodes; ++i) {
			Indices[index + i] = static_cast<uint32_t>(itb->nodes[i]);
		}
	}

	return index;
}


size_t Mesh::PackElementCoord(std::vector<fvmath::Vec4<CoordType> >& ElemPrismCoords,
		bool onlyboundary)
{
	// Check if mesh data is present
	if (_arAllElements.empty()) SetElements(*this,onlyboundary);

	// Adjust size of a container to the number of elements
	fvmath::Vec4<CoordType> node_coords = {0, 0 , 0, 0};
	size_t num_elems = _arAllElements.size();
	assert(num_elems > 0);
	ElemPrismCoords.resize(6 * num_elems);

	// Start proceeding
	for (size_t nel = 0; nel < num_elems; ++nel) {
		Elem_Info* info_ptr = &_arAllElements[nel];
		assert(info_ptr->nodes[0] == 6);
		// Pack nodes one by one up to 6 of prism
		for (int i = 0; i < info_ptr->nodes[0]; ++i) {
			if (GetNodeCoor<CoordType>(info_ptr->nodes[i+1],node_coords.v) > 0)
				ElemPrismCoords.push_back(node_coords);
		}
	}

	return num_elems;
}

void Mesh::PackElementPlanes(std::vector<fvmath::Vec4<CoordType> >& ElemPlanes,
		bool onlyboundary)
{
	// Check if mesh data is present
	if (_arAllElements.empty()) SetElements(*this,onlyboundary);

	// Adjust size of a container to the number of elements
	double node_coords[18];
	size_t num_elems = _arAllElements.size();
	assert(num_elems > 0);
	ElemPlanes.resize(num_elems);

	// Start proceeding
	for (size_t nel = 0; nel < num_elems; ++nel) {
		Elem_Info* info_ptr = &_arAllElements[nel];
		assert(info_ptr->nodes[0] == 6);
		// Pack nodes one by one up to 6 of prism in local buffer
		for (int i = 0; i < info_ptr->nodes[0]; ++i) {
			GetNodeCoor<double>(info_ptr->nodes[i+1],&node_coords[3*i]);
		}
		// Extract planes for faces
		if (info_ptr->nodes[0] == 6) Plane::CreatePlanesForPrism((Plane::vec3 *)&node_coords[0],&ElemPlanes[nel]);
		else if (info_ptr->nodes[0] == 4) Plane::CreatePlanesForTetra((Plane::vec3 *)&node_coords[0],&ElemPlanes[nel]);
	}
}

void Mesh::FillArrayWithElemCoords(const Mesh* pMesh,CoordType ElemCoords[])
{
	mfp_log_debug("Fill array with coords of element nodes");
	assert(ElemCoords != NULL);
	Mesh::arElConstIter itb(pMesh->GetElems().begin());
	Mesh::arElConstIter ite(pMesh->GetElems().begin());
	for (uint32_t index(0);itb != ite; ++itb, ++index)
	{
		for (int i(0);i < itb->nodes[0]; ++i) {
			pMesh->GetNodeCoor(itb->nodes[i],&ElemCoords[3*index]);
		}
	}
}

void Mesh::ConvertNodes(const Mesh *pMesh, float Nodes[])
{
	for (size_type node(1); node < pMesh->GetNumNodes(); ++node)
		pMesh->GetNodeCoor((int)node, &Nodes[node]);
}



}// end namespace FemViewer

