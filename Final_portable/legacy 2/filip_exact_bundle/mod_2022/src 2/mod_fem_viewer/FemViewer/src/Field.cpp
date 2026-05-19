#include "fv_config.h"
#include "Field.h"
#include "Object.h"
#include "Fire.h"

#include "aph_intf.h"
#include "mmh_intf.h"
#include "pdh_intf.h"
#include "defs.h"
#include "Enums.h"
#include "Log.h"
#include "uth_log.h"
#include "Matrix.h"
#include "fv_compiler.h"
#include "fv_float.h"
#include "../../include/Enums.h"
#include "../../utils/fv_exception.h"
#include "../../utils/fv_mathparser.h"
#include "elem_tables.h"
#include "ViewManager.h"
#include "VtxAccumulator.h"
#include<cstdio>
#include<omp.h>
#include<set>

#ifdef FV_DEBUG
#include <iostream>
using namespace std;
#endif

namespace FemViewer {

typedef struct _Node_t {
  fvmath::CVec3d position;
  int  info;
} Node_t;

// Check if a given index of node is on the list of given face list
static inline
int is_on_boundary_face(const int** Elem, const int type, const int nr,
		const int itr) {
	int result = 0;
	for (int i = 0; i < (type - 1); ++i) {
		if (Elem[nr][i] == itr) {
			result++;
			break;
		}
	}
	return result;
}

template<typename T>
T mark_edges_in_triangle(int s,int u,const int dim)
{
	// Mark corners
	if ((u + s == 0) || (s == dim) || (u == dim)) return T(1);
	// Mark top edge
	else if (u == 0) return T(2);
	// Mark skew edge
	else if (u + s == dim) return(3);
	// Mark left edge
	else if (s == 0) return T(4);
	// Mark internal vertices
	return T(0);
}

template<typename T>
T mark_edges_in_quad(int s,int u,const int s_dim,const int u_dim)
{
	// Mark corners
	if ((u + s == 0) || (u + s == u_dim + s_dim) || (s== 0 && u == u_dim) || (u == 0 && s == s_dim))
		return T(1);
	// Mark top edge
	else if (u == 0) return T(2);
	// Mark right edge
	else if (s == s_dim) return T(3);
	// Mark bottom edge
	else if (u == u_dim) return T(4);
	// Mark left edge
	else if (s == 0) return T(5);
	// Mark internal vertices
	return T(0);
}

static unsigned int make_plane_XY0(int nptsx, int nptsy, double dlx, double dly,
		std::vector<Node_t>& vertices, std::vector<ubyte_t>& indices)
{
	assert(nptsx == nptsy);
	Node_t node;
	// Generate point coords in columns
	for (int y(0); y < nptsy; y++) {
		int nx = nptsy - y;
		for (int x(0); x < nx; x++) {
			node.position = fvmath::CVec3d(x * dlx, y * dly, -1.0);
			//node.edge_flag =  ((x > 0) && (y > 0) && ((x + y) < (nptsy -1))) ? 0.0f : 1.0f;
			node.info = mark_edges_in_triangle<Vertex::info_t>(x,y,nptsy-1);
			vertices.push_back(node);
			//std::cout << "p["<<y<<"]["<<x<<"] = " << node.position << " edf = " << node.info << std::endl;
		}
	}

	int base(0);
	nptsy--;
	//i = 0;
	for (int y(0); y < nptsy; y++) {
		int n = nptsx - y;
		bool cond = y < nptsy - 1;
		for (int x = 0; x < n; x++) {
			if ((x == (n - 1)) && cond) {
				// Repeat last index
				/*indices[i++] =*///indices.push_back( (ubyte_t)(base + 2*(n - 1)) );
				/*indices[i++] = */indices.push_back((ubyte_t) (base + x));
				indices.push_back((ubyte_t) (base + x));
			} else {
				indices.push_back((ubyte_t) (base + x));
				if (x < n - 1) /*indices[i++] = */
					indices.push_back((ubyte_t) (base + n + x));
			}
		}
		// add a degenerate triangle (except in a last row)
		if (cond) {
			/*indices[i++]*/indices.push_back((ubyte_t) (base + n - 1));
			/*indices[i++]*/indices.push_back((ubyte_t) (base + n));
		}

		base += n;
	}

	return (indices.size());
}

static unsigned int make_plane_XY1(int nptsx, int nptsy, double dlx, double dly,
		std::vector<Node_t>& vertices, std::vector<ubyte_t>& indices)
{
	assert(nptsx == nptsy);
	Node_t node;
	// Generate point coords in columns
	for (int x(0); x < nptsx; x++) {
		int ny = nptsx - x;
		for (int y(0); y < ny; y++) {
			node.position = fvmath::CVec3d(x * dlx, y * dly, 1.0);
			//node.edge_flag =  ((x > 0) && (y > 0) && ((x + y + 1) < nptsx)) ? 0.0f : 1.0f;
			node.info = mark_edges_in_triangle<Vertex::info_t>(y,x,nptsx-1);
			vertices.push_back(node);
		}
	}

	int base(0);
	nptsx--;
	//i = 0;
	for (int x(0); x < nptsx; x++) {
		int n = nptsy - x;
		bool cond = x < nptsx - 1;
		for (int y = 0; y < n; y++) {
			if ((y == (n - 1)) && cond) {
				// Repeat last index
				indices.push_back((ubyte_t) (base + y));
				indices.push_back((ubyte_t) (base + y));
			} else {
				indices.push_back((ubyte_t) (base + y));
				if (y < n - 1)
					indices.push_back((ubyte_t) (base + n + y));
			}
		}
		// add a degenerate triangle (except in a last row)
		if (cond) {
			indices.push_back((ubyte_t) (base + n - 1));
			indices.push_back((ubyte_t) (base + n));
		}

		base += n;
	}

	return (indices.size());
}

static unsigned int make_plane_XZ(int nptsx, int nptsz, double dlx, double dlz,
		std::vector<Node_t>& vertices, std::vector<ubyte_t>& indices)
{
	Node_t node;
	// Generate point coords in columns
	for (int x(0); x < nptsx; x++) {

		//int base = x * nptsz;

		for (int z(0); z < nptsz; z++) {
			node.position = fvmath::CVec3d(x * dlx, 0.0, 2 * z * dlz - 1.0);
			//node.edge_flag = ((x > 0) && ((x+1) < nptsx) && (z > 0) && ((z+1) < nptsz)) ? 0.0f : 1.0f;
			node.info = mark_edges_in_quad<Vertex::info_t>(z,x,nptsz-1,nptsx-1);
			vertices.push_back(node);
		}
	}

	//unsigned int i(0);
	nptsx--;

	for (int x(0); x < nptsx; x++) {

		int base = x * nptsz;

		for (int z(0); z < nptsz; z++) {
			indices.push_back((ubyte_t) (base + z));
			indices.push_back((ubyte_t) (base + nptsz + z));
		}
		// Add a degenerate triangle (except in a last column)
		if (x < nptsx - 1) {
			indices.push_back((ubyte_t) ((x + 1) * nptsz + (nptsz - 1)));
			indices.push_back((ubyte_t) ((x + 1) * nptsz));
		}
	}

	return (indices.size());
}

static unsigned int make_plane_XYZ(int nptsxy, int nptsz, double dlxy,
		double dlz, std::vector<Node_t>& vertices,
		std::vector<ubyte_t>& indices)
{
	Node_t node;
	// Generate point coords in columns
	for (int x(0); x < nptsxy; x++) {

		//int base = x * nptsz;

		for (int z(0); z < nptsz; z++) {
			//int index = base + z;
			//fvmath::Vec3d * pv = vertices + index;
			const double val = x * dlxy;
			node.position = fvmath::CVec3d(1.0 - val, val, 2 * z * dlz - 1.0);
			//node.edge_flag = ((x > 0) && ((x + 1) < nptsxy) && (z > 0) && ((z + 1) < nptsz)) ? 0.0f : 1.0f;
			node.info = mark_edges_in_quad<Vertex::info_t>(z,x,nptsz-1,nptsxy-1);
			vertices.push_back(node);
		}
	}

	//unsigned int i(0);
	nptsxy--;

	for (int x(0); x < nptsxy; x++) {

		int base = x * nptsz;

		for (int z(0); z < nptsz; z++) {
			indices.push_back((ubyte_t) (base + z));
			indices.push_back((ubyte_t) (base + nptsz + z));
		}
		// add a degenerate triangle (except in a last row)
		if (x < nptsxy - 1) {
			indices.push_back((ubyte_t) ((x + 1) * nptsz + (nptsz - 1)));
			indices.push_back((ubyte_t) ((x + 1) * nptsz));
		}
	}

	return (indices.size());
}

static unsigned int make_plane_YZ(int nptsy, int nptsz, double dly, double dlz,
		std::vector<Node_t>& vertices, std::vector<ubyte_t>& indices)
{
	Node_t node;
	// Generate point coords in columns
	for (int y(0); y < nptsy; y++) {

		//int base = y * nptsz;

		for (int z(0); z < nptsz; z++) {
			//int index = base + z;
			//fvmath::Vec3d * pv = vertices + index;
			const double val = y * dly;
			node.position = fvmath::CVec3d(0.0, 1.0 - val, 2 * z * dlz - 1.0);
			//node.edge_flag = ((y > 0) && (y < nptsy) && (z > 0) && (z < nptsz)) ? 0.0f : 1.0f;
			node.info = mark_edges_in_quad<Vertex::info_t>(z,y,nptsz-1,nptsy-1);
			vertices.push_back(node);
		}
	}

	//int i(0);
	nptsy--;

	for (int y(0); y < nptsy; y++) {

		int base = y * nptsz;

		for (int z(0); z < nptsz; z++) {
			indices.push_back((ubyte_t) (base + z));
			indices.push_back((ubyte_t) (base + nptsz + z));
		}
		// add a degenerate triangle (except in a last row)
		if (y < nptsy - 1) {
			indices.push_back((ubyte_t) ((y + 1) * nptsz + (nptsz - 1)));
			indices.push_back((ubyte_t) ((y + 1) * nptsz));
		}
	}

	return (indices.size());
}

//static unsigned int (* make_plane[5])(int,int,double,double,fvmath::Vec3d*,ubyte_t*) = {
//  make_plane_XY0, make_plane_XY1, make_plane_XZ, make_plane_XYZ, make_plane_YZ
//};

Field::Field(Mesh& pmesh_, const std::string& name_)
: BaseField()
, _minp(FV_DEF_P)
, _maxp(FV_DEF_P)
, _pmesh(pmesh_)
, _pobject(NULL)
, _name(name_)
, _baseGTriElemsFull()
, _baseGTriElemsCut()
, _renderer(NULL)
, _minValue(std::numeric_limits<double>::max())
, _maxValue(-std::numeric_limits<double>::max())
{
	//mfp_log_debug("Field ctr\n");
	//logd("Constructor Field\n");
	_values = std::make_pair(-FV_LARGE, FV_LARGE);
	_xgrads = std::make_pair(-FV_LARGE, FV_LARGE);
	_ygrads = std::make_pair(-FV_LARGE, FV_LARGE);
	_zgrads = std::make_pair(-FV_LARGE, FV_LARGE);
	//this->Init(_name,-1);
}

Field::~Field() {
	//mfp_log_debug("Dtr\n");
	this->Free();

	_baseGTriElemsFull.clear();
	_baseGTriElemsCut.clear();
}

int Field::Init(const int parent_id_, const char* fname_) {
	//mfp_log_debug("Init\n");
	int result;
	if ((result = BaseField::Init(fname_, parent_id_)) < 0)
		return (-1);
	//mfp_debug("before of\n");
	_name = fname_ ? std::string(fname_) : "Unknown";
	if (this->type() == FieldSTD) {
		//mfp_debug("in std\n");
		// set min/max pdeg to default value
		_minp = FV_DEF_MAX_P_STD;
		_maxp = FV_DEF_MIN_P_STD;
		// set render routine
		_renderer = &Field::RenderSTD;

	} else if (this->type() == FieldDG) {
		//mfp_debug("In dg cectuion of init\n");
		// set render routine
		_renderer = &Field::RenderDG;
		_minp = FV_DEF_MAX_P_DG;
		_maxp = FV_DEF_MIN_P_DG;
		// get min/max value of pdeg
		int pdeg[PDC_MAXEQ];
		// here is the best way to do omp stuff
		Mesh::arElems::const_iterator it(_pmesh.GetElems().begin());
		Mesh::arElems::const_iterator it_e(_pmesh.GetElems().end());
		for (; it != it_e; ++it) {
			(void) get_el_pdeg(this->idx(), it->elemId.eid, pdeg);
			_minp = (_minp < pdeg[0]) ? _minp : pdeg[0];
			_maxp = (_maxp > pdeg[0]) ? _maxp : pdeg[0];
		}
		//
	} else {
		// Unknown field type
		return (-1);
	}

	GetValuesRange(1);
	//mfp_log_debug("min_pdeg = %d max_pdeg = %d\n",_minp,_maxp);

	return (result);
}

size_t Field::CountCoeffs(std::vector<int>* degree_ptr) const
{
	int pdeg;
	const int nreq = _solution.nrEquetions;
	if (degree_ptr) degree_ptr->resize(_pmesh.GetElems().size());

	size_t num_coeffs = 0;
	for (size_t i(0); i < _pmesh.GetElems().size(); ++i) {
		int el_id = _pmesh.GetElems()[i].elemId.eid;
		int num_shup = GetNumberOfShapeFunc(el_id, &pdeg);
		num_coeffs += get_el_pdeg_numshap(this->idx(), el_id, &pdeg) * nreq;
		if (degree_ptr) degree_ptr->push_back(pdeg);
	}
	return num_coeffs;
}

int Field::Free() {
	return BaseField::Free();
}

void Field::Reset(bool bClearGL) {
	this->BaseField::Reset();
	if (bClearGL)
		_pobject = NULL;
}

void Field::Clear() {
//	_pobject =
//			ViewManagerInst().GetGraphicData().GetRootObject().ResetSubObject(
//					this->GetGLId());
}

void Field::InitGeometry(void * pRootObject, char type) {
}

void Field::ResetGeometry(void * pRootObj, char type) {
}

void Field::Draw(void *ptr)
{
	fv_timer t;
	t.start();
	(this->*_renderer)(ptr);
	//printf("duration = %f ms\n",t.get_duration_ms());
}

void Field::Render() {
	//mfp_log_debug("Field: render\n");
	int pdeg[PDC_MAXEQ];
	int nr_nodes, nr_faces, nr_dofs, w = 0;
	const int iaux = 2;
	const int *elem_p(NULL);
	int Nodes[MMC_MAXELVNO + 1]; /* indices of nodes */
	double Node_coor[3 * MMC_MAXELVNO]; /* coord of nodes of El */
	double Dofs_loc[APC_MAXELSD]; /* element solution dofs */
	double Xcoor[18]; /* global coord of gauss point */
	double U_val[PDC_MAXEQ]; /* computed solution */
	double U_x[PDC_MAXEQ]; /* gradient of computed solution */
	double U_y[PDC_MAXEQ]; /* gradient of computed solution */
	double U_z[PDC_MAXEQ]; /* gradient of computed solution */
	double Base_phi[APC_MAXELVD]; /* basis functions */
	double Base_dphix[APC_MAXELVD]; /* x-derivatives of basis function */
	double Base_dphiy[APC_MAXELVD]; /* y-derivatives of basis function */
	double Base_dphiz[APC_MAXELVD]; /* y-derivatives of basis function */
	double *Xloc;
	double dTmp;
	// Vector of solutions
	std::vector<double> vSol;
	std::vector<fvmath::CVec3f> mappedColors(MMC_MAXELVNO);
	std::vector<fvmathParser::MathElement> mathElems;
	//Object*& pobject = _pmesh.GetGLHandle();
	//VtxAccumulator& vtxAccuml = _pobject->GetCurrentVtxAccumulator();
	// First update legend
	Legend& rleg = ViewManagerInst().GetLegend();
	rleg.Set(GetValues().first, GetValues().second, GetValues().first,
			GetValues().second);
	// Prepare mathematics formula
	fvmathParser::MathCalculator::ONP(_solution.sFormula, _solution.nrEquetions,
			mathElems);

	Mesh::arElConstIter it = _pmesh.GetElems().begin();
	const Mesh::arElConstIter ite = _pmesh.GetElems().end();
	std::vector<fvmath::CVec3d> nodeCoords;	// vector container for coords
	std::vector<short> indexNodes;		// vecot container for indexes of nodes
	//_pobject->SetPointSize(2.0);
	fvmath::CVec3f normal;
	unsigned int iv = 0;
	while (it != ite) {
		// Skip element if it is not a boundary
		if (!IS_BOUND(it->elemId.id)) {
			++it;
			continue;
		}
		// Retrieve element index
		int nel(it->elemId.eid);
		// Element base function type
		const int base = get_base_type(this->idx(), nel);
		// Set reference element
		if (IS_TETRA(it->elemId.id)) {
			Xloc = (double*) &XlocTetra[0];
			elem_p = Mesh::tetra[0];
			nr_faces = 4;
		} else {
			Xloc = (double*) &XlocPrizm[0];
			elem_p = Mesh::prizm[0];
			nr_faces = 5;
		}
		// Get element dofs
		nr_dofs = get_element_dofs(this->idx(), nel, _solution.currSolution,
				Dofs_loc);
		// Get its corresponding pdeg
		(void) get_el_pdeg(this->idx(), nel, pdeg);
		// Get element real coordinates
		nr_nodes = _pmesh.GetElementCoordinates(nel, Nodes, Node_coor);
		// Calculate values
		mappedColors.clear();

		for (int ino = 0; ino < nr_nodes; ++ino) {
			if (iaux == 2) {
				apr_elem_calc_3D_mod(iaux, _solution.nrEquetions, pdeg, base,
						&Xloc[3 * ino], Node_coor, Dofs_loc, Base_phi,
						Base_dphix, Base_dphiy, Base_dphiz, Xcoor, U_val, U_x,
						U_y, U_z, NULL);
			} else {
				apr_elem_calc_3D_mod(iaux, _solution.nrEquetions, pdeg, base,
						&Xloc[3 * ino], Node_coor, Dofs_loc, Base_phi,
						NULL, NULL, NULL, Xcoor, U_val,
						NULL, NULL, NULL, NULL);
			}
			//mfp_debug("U_val = %lf\n",U_val[0]);
			vSol.clear();

			for (int sol = 0; sol < _solution.nrEquetions; ++sol)
				vSol.push_back(U_val[sol]);

			fvmathParser::MathCalculator::ONPCalculate(mathElems, vSol, dTmp);
			//mfp_debug("Value: %lf\n",dTmp);
			rleg.GetColor(dTmp);
			mappedColors.push_back(rleg.color.rgb.v);
			//std::cout << "Color" << ino << " = " << fvmath::CVec3f(rleg.color.rgb.v) << "\n";

		}
//mfp_debug("After render\n");
		// Now we have to add faces only on boundary sides
		for (int nf = 0; nf < nr_faces; ++nf) {
			//mfp_debug("nf = %d elmId = %x \n",nf,it->elemId.id);
			//std::cout << "Is face: " << IS_SET_FACEN(it->elemId.id,nf) << std::endl;
			// Check if current face is boundary
			if (!IS_SET_FACEN(it->elemId.id, nf))
				continue;
			// for each face add triangle; for quads there will be 2 traingles
			int of = nf * (nr_faces - 1);
			//mfp_debug("of = %d %d\n",of,nr_faces);
			const int n0 = elem_p[of];
			const int n1 = elem_p[of + 1];
			const int n2 = elem_p[of + 2];
			//mfp_debug("nf = %d, n0 = %d n1 = %d n2 = %d\n",nf,n0,n1,n2);
			normal = fvmath::GetNormal2Plane<float, double>(&Node_coor[3 * n0],
					&Node_coor[3 * n1], &Node_coor[3 * n2]);
			// v0
			_pobject->AddMeshNode(fvmath::CVec3f(&Node_coor[3 * n0]));
			_pobject->AddNormalToNode(normal);
			//std::cout << nf << " normal1 " << normal << std::endl;
			//mfp_debug("n = %d Node = %d\n",n0,Nodes[n0+1]-1);
			_pobject->AddIndexOfNode(iv++,vtxTriangle);
			_pobject->AddColorOfNode(mappedColors[n0]);
			//std::cout << mappedColors[n0] << std::endl;
			// v1
			_pobject->AddMeshNode(fvmath::CVec3f(&Node_coor[3 * n1]));
			_pobject->AddNormalToNode(normal);
			//mfp_debug("n = %d Node = %d\n",n1,Nodes[n1+1]-1);
			_pobject->AddIndexOfNode(iv++,vtxTriangle);
			_pobject->AddColorOfNode(mappedColors[n1]);
			//std::cout << mappedColors[n1] << std::endl;
			// v2
			_pobject->AddMeshNode(fvmath::CVec3f(&Node_coor[3 * n2]));
			_pobject->AddNormalToNode(normal);
			//mfp_debug("n = %d Node = %d\n",n2,Nodes[n2+1]-1);
			_pobject->AddIndexOfNode(iv++,vtxTriangle);
			_pobject->AddColorOfNode(mappedColors[n2]);
			//std::cout << mappedColors[n2] << std::endl;
			if (nr_faces == 5 && nf >= 2) {
				// Quad face = 2 triangles to add
				const int n3 = elem_p[of + 3];
				//mfp_debug("nf = %d, n2 = %d n3 = %d n0 = %d\n",nf,n2,n3,n0);
				normal = fvmath::GetNormal2Plane<float, double>(
						&Node_coor[3 * n2], &Node_coor[3 * n3],
						&Node_coor[3 * n0]);
				//std::cout << "+1 " << nf << " normal11 " << normal << std::endl;
				// v2
				_pobject->AddMeshNode(fvmath::CVec3f(&Node_coor[3 * n2]));
				_pobject->AddNormalToNode(normal);
				//mfp_debug("n = %d Node = %d\n",n2,Nodes[n2+1]-1);
				_pobject->AddIndexOfNode(iv++,vtxTriangle);
				_pobject->AddColorOfNode(mappedColors[n2]);
				//std::cout << mappedColors[n2] << std::endl;
				// v3
				_pobject->AddMeshNode(fvmath::CVec3f(&Node_coor[3 * n3]));
				_pobject->AddNormalToNode(normal);
				//mfp_debug("n = %d Node = %d\n",n3,Nodes[n3+1]-1);
				_pobject->AddIndexOfNode(iv++,vtxTriangle);
				_pobject->AddColorOfNode(mappedColors[n3]);
				//std::cout << mappedColors[n3] << std::endl;
				// v0
				_pobject->AddMeshNode(fvmath::CVec3f(&Node_coor[3 * n0]));
				_pobject->AddNormalToNode(normal);
				//mfp_debug("n = %d Node = %d\n",n0,Nodes[n0+1]-1);
				_pobject->AddIndexOfNode(iv++,vtxTriangle);
				_pobject->AddColorOfNode(mappedColors[n0]);
				//std::cout << mappedColors[n0] << std::endl;
			}
		}
		++it;
	}
	FV_CHECK_ERROR_GL();
	// setup VBO
	VtxAccumulator& vtxAccuml = _pobject->GetCurrentVtxAccumulator();
	//vtxAccuml.UseVBO() = ViewManagerInst().IsVBOUsed();
	//vtxAccuml.createVBO();
	//vtxAccuml.createVAO();
	vtxAccuml.aRenderTraingles = true;
	FV_CHECK_ERROR_GL();
	mfp_debug("At the end of render\n");
}

bool Field::CreateGraphicElements(const int iaux,
		std::vector<GraphElement2<double> >& elemsContener_) {
	int pdeg[PDC_MAXEQ];
	// Control to elem_calc_3d
	const int *elem_p;
	int nr_nodes, nr_dofs;

	//---
	//int    El_nodes[MMC_MAXELVNO + 1];	/* list of nodes of El */
	int El_faces[MMC_MAXELFAC + 1]; /* list of faces of El */
	double Node_coor[3 * MMC_MAXELVNO]; /* coord of nodes of El */
	double Dofs_loc[APC_MAXELSD]; /* element solution dofs */
	double Xcoor[3]; /* global coord of gauss point */
	double U_val[PDC_MAXEQ]; /* computed solution */
	double U_x[PDC_MAXEQ]; /* gradient of computed solution */
	double U_y[PDC_MAXEQ]; /* gradient of computed solution */
	double U_z[PDC_MAXEQ]; /* gradient of computed solution */
	double Base_phi[APC_MAXELVD]; /* basis functions */
	double Base_dphix[APC_MAXELVD]; /* x-derivatives of basis function */
	double Base_dphiy[APC_MAXELVD]; /* y-derivatives of basis function */
	double Base_dphiz[APC_MAXELVD]; /* y-derivatives of basis function */
	double dTmp;
	double* Xloc;
	///--

	Point3D<double> p1, p2, p3, p4;
	GraphElement2<double> gEl;

	// Vector of solutions
	std::vector<double> vSol, vSolEvaluated;  // for mathparser

	// Prepare function formula
	std::vector<fvmathParser::MathElement> mathElems;
	fvmathParser::MathCalculator::ONP(_solution.sFormula, _solution.nrEquetions,
			mathElems);
	elemsContener_.clear();

	// Get the handle to boundary active faces
	const Mesh::arBndElems& vBounds = _pmesh.GetBoundaryElems();

	// Iterators over boundary active faces
	Mesh::BndElemsConstIter it = vBounds.begin();
	const Mesh::BndElemsConstIter it_e = vBounds.end();

	while (it != it_e) {

		// Element base function type
		const int base = get_base_type(this->idx(), it->id);

		// Set reference element
		if (IS_TETRA(it->id)) {
			Xloc = (double*) &XlocTetra[0];
			elem_p = Mesh::tetra[0];
		} else {
			Xloc = (double*) &XlocPrizm[0];
			elem_p = Mesh::prizm[0];
		}

		// Get its corresponding pdeg
		(void) get_el_pdeg(this->idx(), it->eid, pdeg);

		// Get element coordinates
		nr_nodes = _pmesh.GetElementCoordinates(it->eid, NULL, Node_coor);

		// Get element dofs
		nr_dofs = get_element_dofs(this->idx(), it->eid, _solution.currSolution,
				Dofs_loc);

		// Calculate values
		vSolEvaluated.clear();

		for (int ino = 0; ino < nr_nodes; ++ino) {
			if (iaux == 2) {
				apr_elem_calc_3D_mod(iaux, _solution.nrEquetions, pdeg, base,
						&Xloc[3 * ino], Node_coor, Dofs_loc, Base_phi,
						Base_dphix, Base_dphiy, Base_dphiz, Xcoor, U_val, U_x,
						U_y, U_z, NULL);
			} else {
				apr_elem_calc_3D_mod(iaux, _solution.nrEquetions, pdeg, base,
						&Xloc[3 * ino], Node_coor, Dofs_loc, Base_phi,
						NULL, NULL, NULL, Xcoor, U_val,
						NULL, NULL, NULL, NULL);
			}

			vSol.clear();

			for (int sol = 0; sol < _solution.nrEquetions; ++sol)
				vSol.push_back(U_val[sol]);

			fvmathParser::MathCalculator::ONPCalculate(mathElems, vSol, dTmp);
			vSolEvaluated.push_back(dTmp);
		}

		FV_ASSERT(it->id > 0);
		(void) _pmesh.GetElementFaces(it->eid, El_faces, NULL);

		//const int num_faces = *El_faces;
		switch (*El_faces) {
		case 4: {
			for (register int j(0); j < 4; ++j)
				if (vBounds[j].faces & (1 << j)) {
					int key = 3 * elem_p[3 * j];
					p1.x = Node_coor[key];
					p1.y = Node_coor[key + 1];
					p1.z = Node_coor[key + 2];
					key = 3 * elem_p[3 * j + 1];
					p2.x = Node_coor[key];
					p2.y = Node_coor[key + 1];
					p2.z = Node_coor[key + 2];
					key = 3 * elem_p[3 * j + 2];
					p3.x = Node_coor[key];
					p3.y = Node_coor[key + 1];
					p3.z = Node_coor[key + 2];
					gEl.Clear();
					gEl.Add(p1, vSolEvaluated[elem_p[3 * j]]);
					gEl.Add(p2, vSolEvaluated[elem_p[3 * j + 1]]);
					gEl.Add(p3, vSolEvaluated[elem_p[3 * j + 2]]);
					elemsContener_.push_back(gEl);
				}
		}
			break;
		case 5: {
			for (register int j(0); j < 5; ++j)
				if (vBounds[j].faces & (1 << j)) {
					int key = 3 * elem_p[4 * j];
					p1.x = Node_coor[key];
					p1.y = Node_coor[key + 1];
					p1.z = Node_coor[key + 2];
					key = 3 * elem_p[4 * j + 1];
					p2.x = Node_coor[key];
					p2.y = Node_coor[key + 1];
					p2.z = Node_coor[key + 2];
					key = 3 * elem_p[4 * j + 2];
					p3.x = Node_coor[key];
					p3.y = Node_coor[key + 1];
					p3.z = Node_coor[key + 2];
					gEl.Clear();
					gEl.Add(p1, vSolEvaluated[elem_p[4 * j]]);
					gEl.Add(p2, vSolEvaluated[elem_p[4 * j + 1]]);
					gEl.Add(p3, vSolEvaluated[elem_p[4 * j + 2]]);
					elemsContener_.push_back(gEl);
					if (j > 1) {
						key = 3 * elem_p[4 * j + 3];
						p4.x = Node_coor[key];
						p4.y = Node_coor[key + 1];
						p4.z = Node_coor[key + 2];
						gEl.Clear();
						gEl.Add(p1, vSolEvaluated[elem_p[4 * j]]);
						gEl.Add(p3, vSolEvaluated[elem_p[4 * j + 2]]);
						gEl.Add(p4, vSolEvaluated[elem_p[4 * j + 3]]);
						elemsContener_.push_back(gEl);
					}
				}
		}
			break;
		default:
			//_log_err("Unknown element type: %d\n",El_faces[0]);
			exit(-1);
		}
		++it;
	}

	return true;
}

bool Field::CreateGraphicCuttedElements(const CutPlane& plane_, const int iaux,
		std::vector<GraphElement2<double> >& elemsContener_) {
	int pdeg[PDC_MAXEQ];
	// Control to elem_calc_3d
	const int *elem_p;
	int nr_nodes, nr_dofs;

	//---
	//int    El_nodes[MMC_MAXELVNO + 1];	/* list of nodes of El */
	int El_faces[MMC_MAXELFAC + 1]; /* list of faces of El */
	double Node_coor[3 * MMC_MAXELVNO]; /* coord of nodes of El */
	double Dofs_loc[APC_MAXELSD]; /* element solution dofs */
	double Xcoor[3]; /* global coord of gauss point */
	double U_val[PDC_MAXEQ]; /* computed solution */
	double U_x[PDC_MAXEQ]; /* gradient of computed solution */
	double U_y[PDC_MAXEQ]; /* gradient of computed solution */
	double U_z[PDC_MAXEQ]; /* gradient of computed solution */
	double Base_phi[APC_MAXELVD]; /* basis functions */
	double Base_dphix[APC_MAXELVD]; /* x-derivatives of basis function */
	double Base_dphiy[APC_MAXELVD]; /* y-derivatives of basis function */
	double Base_dphiz[APC_MAXELVD]; /* y-derivatives of basis function */
	double dTmp;
	double* Xloc;
	double elem[24];

	Point3D<double> p1, p2, p3, p4;
	GraphElement2<double> gEl;

	// Vector of solutions
	std::vector<double> vSol, vSolEvaluated;  // for mathparser

	// Prepare function formula
	std::vector<fvmathParser::MathElement> mathElems;
	fvmathParser::MathCalculator::ONP(_solution.sFormula, _solution.nrEquetions,
			mathElems);
	elemsContener_.clear();

	// Get the handle to boundary active faces
	const Mesh::arBndElems& vBounds = _pmesh.GetBoundaryElems();

	// Iterators over boundary active faces
	Mesh::BndElemsConstIter it = vBounds.begin();
	//const Mesh::BndElemsConstIter it_e = vBounds.end();

	// Get the handle to boundary active faces
	const Mesh::arBndElems& arActs = _pmesh.GetBoundaryElems();
	Mesh::arBndElems::size_type nacts = arActs.nactive();

	int res;

	//const int nmel = pMesh->Get_nmel(pMesh->Id);
	for (Mesh::arBndElems::size_type i(0); i < nacts; ++i) {
		const int base = get_base_type(this->idx(), it->eid);

		// Get element coordinates
		nr_nodes = _pmesh.GetElementCoordinates(arActs(i).id, NULL, Node_coor);
		// Set reference element
		if (IS_TETRA(arActs(i).id)) {
			Xloc = (double*) &XlocTetra[0];
			elem_p = Mesh::tetra[0];
		} else {
			Xloc = (double*) &XlocPrizm[0];
			elem_p = Mesh::prizm[0];
		}

		// Get its corresponding pdeg
		(void) get_el_pdeg(this->idx(), it->eid, pdeg);

		// Get element dofs
		nr_dofs = get_element_dofs(this->idx(), it->eid, _solution.currSolution,
				Dofs_loc);

		// Clear vectors
		vSolEvaluated.clear();

		// Loop over nodes of the element
		for (int ino = 0; ino < nr_nodes; ++ino) {

			if (iaux == 2) {
				apr_elem_calc_3D_mod(iaux, _solution.nrEquetions, pdeg, base,
						&Xloc[3 * ino], Node_coor, Dofs_loc, Base_phi,
						Base_dphix, Base_dphiy, Base_dphiz, Xcoor, U_val, U_x,
						U_y, U_z, NULL);
			} else {
				apr_elem_calc_3D_mod(iaux, _solution.nrEquetions, pdeg, base,
						&Xloc[3 * ino], Node_coor, Dofs_loc, Base_phi,
						NULL, NULL, NULL, Xcoor, U_val,
						NULL, NULL, NULL, NULL);
			}

			vSol.clear();
			for (int sol = 0; sol < _solution.nrEquetions; ++sol)
				vSol.push_back(U_val[sol]);

			fvmathParser::MathCalculator::ONPCalculate(mathElems, vSol, dTmp);
			elem[(ino << 2)] = Node_coor[3 * ino];
			elem[(ino << 2) + 1] = Node_coor[3 * ino + 1];
			elem[(ino << 2) + 2] = Node_coor[3 * ino + 2];
			elem[(ino << 2) + 3] = dTmp;
			vSolEvaluated.push_back(dTmp);
		}

		::memset(El_faces, 0, (MMC_MAXELFAC + 1) * sizeof(int));

		const int nr_faces = (nr_nodes == 4) ? 4 : 5;

		if (nr_nodes == 4) {

			if (IS_SET_FACEN(arActs(i).id, 0)) {
				res = plane_.CheckLocation(*Node_coor, *(Node_coor + 1),
						*(Node_coor + 2));
				res += plane_.CheckLocation(*(Node_coor + 3), *(Node_coor + 4),
						*(Node_coor + 5));
				res += plane_.CheckLocation(*(Node_coor + 6), *(Node_coor + 7),
						*(Node_coor + 8));
				if (res == -3) {

					p1.x = *Node_coor;
					p1.y = *(Node_coor + 1);
					p1.z = *(Node_coor + 2);
					p2.x = *(Node_coor + 3);
					p2.y = *(Node_coor + 4);
					p2.z = *(Node_coor + 5);
					p3.x = *(Node_coor + 6);
					p3.y = *(Node_coor + 7);
					p3.z = *(Node_coor + 8);

					gEl.Clear();
					gEl.Add(p1, vSolEvaluated[0]);
					gEl.Add(p2, vSolEvaluated[1]);
					gEl.Add(p3, vSolEvaluated[2]);

					elemsContener_.push_back(gEl);
				}
			}

			if (IS_SET_FACEN(arActs(i).id, 1)) {
				res = plane_.CheckLocation(*Node_coor, *(Node_coor + 1),
						*(Node_coor + 2));
				res += plane_.CheckLocation(*(Node_coor + 3), *(Node_coor + 4),
						*(Node_coor + 5));
				res += plane_.CheckLocation(*(Node_coor + 9), *(Node_coor + 10),
						*(Node_coor + 11));
				if (res == -3) {

					p1.x = *Node_coor;
					p1.y = *(Node_coor + 1);
					p1.z = *(Node_coor + 2);
					p2.x = *(Node_coor + 3);
					p2.y = *(Node_coor + 4);
					p2.z = *(Node_coor + 5);
					p3.x = *(Node_coor + 9);
					p3.y = *(Node_coor + 10);
					p3.z = *(Node_coor + 11);

					gEl.Clear();
					gEl.Add(p1, vSolEvaluated[0]);
					gEl.Add(p2, vSolEvaluated[1]);
					gEl.Add(p3, vSolEvaluated[3]);

					elemsContener_.push_back(gEl);
				}
			}

			if (IS_SET_FACEN(arActs(i).id, 2)) {
				res = plane_.CheckLocation(*Node_coor, *(Node_coor + 1),
						*(Node_coor + 2));
				res += plane_.CheckLocation(*(Node_coor + 6), *(Node_coor + 7),
						*(Node_coor + 8));
				res += plane_.CheckLocation(*(Node_coor + 9), *(Node_coor + 10),
						*(Node_coor + 11));
				if (res == -3) {

					p1.x = *Node_coor;
					p1.y = *(Node_coor + 1);
					p1.z = *(Node_coor + 2);
					p2.x = *(Node_coor + 6);
					p2.y = *(Node_coor + 7);
					p2.z = *(Node_coor + 8);
					p3.x = *(Node_coor + 9);
					p3.y = *(Node_coor + 10);
					p3.z = *(Node_coor + 11);

					gEl.Clear();
					gEl.Add(p1, vSolEvaluated[0]);
					gEl.Add(p2, vSolEvaluated[2]);
					gEl.Add(p3, vSolEvaluated[3]);

					elemsContener_.push_back(gEl);
				}
			}

			if (IS_SET_FACEN(arActs(i).id, 3)) {
				res = plane_.CheckLocation(*(Node_coor + 3), *(Node_coor + 4),
						*(Node_coor + 5));
				res += plane_.CheckLocation(*(Node_coor + 6), *(Node_coor + 7),
						*(Node_coor + 8));
				res += plane_.CheckLocation(*(Node_coor + 9), *(Node_coor + 10),
						*(Node_coor + 11));
				if (res == -3) {

					p1.x = *(Node_coor + 3);
					p1.y = *(Node_coor + 4);
					p1.z = *(Node_coor + 5);
					p2.x = *(Node_coor + 6);
					p2.y = *(Node_coor + 7);
					p2.z = *(Node_coor + 8);
					p3.x = *(Node_coor + 9);
					p3.y = *(Node_coor + 10);
					p3.z = *(Node_coor + 11);

					gEl.Clear();
					gEl.Add(p1, vSolEvaluated[1]);
					gEl.Add(p2, vSolEvaluated[2]);
					gEl.Add(p3, vSolEvaluated[3]);

					elemsContener_.push_back(gEl);
				}
			}

			::memset(El_faces, 0, (MMC_MAXELFAC + 1) * sizeof(int));
			El_faces[0] = 0;
			//int idf, l(1);
			for (int nf(0); nf < nr_faces; ++nf) //????
					{
				if (IS_SET_FACEN(arActs(i).id, nf)) {
					El_faces[0]++;
					El_faces[nf + 1]++;
				}
			}
			//std::cout<<"przed\n";
			(void) DoSliceTerahedron(&plane_, El_faces, elem, elemsContener_,
					NULL);
		} else if (nr_nodes == 6) {
			(void) DoSlicePrizm(&plane_, El_faces, elem, elemsContener_, NULL);
		}
	}

	return true;
}

void Field::CreateDisplayListColors(Legend& legend,
		std::vector<GraphElement2<double> >& gEl, Object* obj_) {

	Point3D<double> mc;
	fvmath::CVec3f pv[3];
	//const mmt_nodes *vertexStart, *vertexEnd;
	Point3D<double> pa, pb, pc;

	std::vector<colormap_value> colorTmp;
	std::vector<Point3D<double> > coordTmp;
	std::vector<unsigned int> isStep;
	Fire fire(&_pmesh, legend);
	std::vector<unsigned int> drawPoints;

	std::vector<GraphElement2<double> > elOuts;

	if (obj_ == NULL) {
		mfp_log_warn("Error! Goemtry object fields not initialized\n");
		return;
	}

	FV_ASSERT(gEl.size() > 0);
	for (register unsigned int i = 0; i < static_cast<unsigned int>(gEl.size());
			++i) {

		fire.Exe3(gEl[i], elOuts);
		/*std::cout<<"sizeof elouts= " << elOuts.size() << std::endl;*/
		for (register int j = 0; j < static_cast<int>(elOuts.size()); ++j) {
			pv[0].x = elOuts[j].points[0].x;
			pv[0].y = elOuts[j].points[0].y;
			pv[0].z = elOuts[j].points[0].z;

			pv[1].x = elOuts[j].points[1].x;
			pv[1].y = elOuts[j].points[1].y;
			pv[1].z = elOuts[j].points[1].z;

			pv[2].x = elOuts[j].points[2].x;
			pv[2].y = elOuts[j].points[2].y;
			pv[2].z = elOuts[j].points[2].z;

			/*if (fvmath::Compare2(pv[0],0.0)) {
			 std::cout << pv[0].x << " " << pv[0].y << " " << pv[0].z << "\n";
			 }
			 if (fvmath::Compare2(pv[1],0.0)) {
			 std::cout << pv[1].x << " " << pv[1].y << " " << pv[1].z << "\n";
			 }
			 if (fvmath::Compare2(pv[2],0.0)) {
			 std::cout << pv[2].x << " " << pv[2].y << " " << pv[2].z << "\n";
			 }*/

			if (fvmath::Compare(pv[0], pv[1]) || fvmath::Compare(pv[1], pv[2])
					|| fvmath::Compare(pv[2], pv[0])) {
#ifdef FV_DEBUG
				std::cerr<< "elem " << i << " triang " << j << std::endl;
				std::cout.flush();
				//	//FV_ASSERT(false);
				//	//fprintf(fp," n\n");
#endif
				continue;
			}

			// Normalize
			CVec3f nv = fvmath::GetNormal2Plane<float, float>(pv[0].v, pv[1].v,
					pv[2].v);				//(pv[1] - pv[0]) * (pv[2] - pv[0]);
			try {
				fvmath::Normalize(nv);
			} catch (const char* ex) {
				std::cout << ex << std::endl;
				continue;
			}
			//std::cout<<i<<" "<<j<<std::endl;
			//std::cout<<"p1=("<<pv[0].x<<", "<<pv[0].y<<", "<<pv[0].z<<")\n";
			//std::cout << elOuts[j].colors[0] << std::endl;
			//std::cout<<"p2=("<<pv[1].x<<", "<<pv[1].y<<", "<<pv[1].z<<")\n";
			//std::cout << elOuts[j].colors[1] << std::endl;
			//std::cout<<"p3=("<<pv[2].x<<", "<<pv[2].y<<", "<<pv[2].z<<")\n";
			//std::cout << elOuts[j].colors[2] << std::endl;

			obj_->AddTriangleNormalColored(nv.v, pv[0].v, pv[1].v, pv[2].v,
					elOuts[j].colors[0], elOuts[j].colors[1],
					elOuts[j].colors[2]);

		}

	}
	//fclose(fp

}

void Field::CretaeContourLines(Legend& legend,
		std::vector<GraphElement2<double> >& gEl, Object* obj_,
		bool coloredLines) {

	Fire fire(&_pmesh, legend);
	std::vector<GraphElement2<double> > vLines;
	Vec3D p1, p2;

	ColorRGB col(1.0f, 1.0f, 1.0f);

	FV_ASSERT(obj_ != NULL);

	//obj_->SetDrawColor(col);
	obj_->SetLineWidth(1.5f);

	for (register size_t i = 0; i < gEl.size(); ++i) {
		fire.CreateLine(gEl[i], vLines);
		//std::cout<<"rozmiar["<<i<<"]= "<< vLines.size()<<std::endl;
		//std::cout<<"wynik: " << (vLines.size() & 1) << std::endl;
		//if( (vLines.size() & 1) != 0) continue;
		for (register size_t j = 0; j < vLines.size(); ++j) {
			p1[0] = static_cast<float>(vLines[j].points[0].x);
			p1[1] = static_cast<float>(vLines[j].points[0].y);
			p1[2] = static_cast<float>(vLines[j].points[0].z);
			p2[0] = static_cast<float>(vLines[j].points[1].x);
			p2[1] = static_cast<float>(vLines[j].points[1].y);
			p2[2] = static_cast<float>(vLines[j].points[1].z);
			ColorRGB& c1 = (coloredLines) ? vLines[j].colors[0] : col;
			ColorRGB& c2 = (coloredLines) ? vLines[j].colors[1] : col;
			//p1.out("p1"); c1.outcolor("k1");
			//p2.out("p2"); c2.outcolor("k2");

			if (p1 == p2) {
				std::cout << "kontury rowne dla i= " << i << " i j= " << j
						<< std::endl;
				continue;
			}
			obj_->AddLineColored(p1, c1, p2, c2);

		}
	}

	//std::cout<<"wyjscie\n";
	//std::cout.flush();
}

void Field::GetValuesRange(const int iaux) {
	//mfp_log_debug("getValueRange\n");
	int pdeg[PDC_MAXEQ];
	int nr_nodes;

	double Node_coor[3 * MMC_MAXELVNO]; /* coord of nodes of El */
	double Dofs_loc[APC_MAXELSD]; /* element solution dofs */
	double Xcoor[18]; /* global coord of gauss point */
	double U_val[PDC_MAXEQ]; /* computed solution */
	double U_x[PDC_MAXEQ]; /* gradient of computed solution */
	double U_y[PDC_MAXEQ]; /* gradient of computed solution */
	double U_z[PDC_MAXEQ]; /* gradient of computed solution */
	double Base_phi[APC_MAXELVD]; /* basis functions */
	double Base_dphix[APC_MAXELVD]; /* x-derivatives of basis function */
	double Base_dphiy[APC_MAXELVD]; /* y-derivatives of basis function */
	double Base_dphiz[APC_MAXELVD]; /* y-derivatives of basis function */
	//double dTmp, tmin, tmax;
	double vals[8] = { std::numeric_limits<ScalarValueType>::max(),
			-std::numeric_limits<ScalarValueType>::max(), std::numeric_limits<
					ScalarValueType>::max(), -std::numeric_limits<
					ScalarValueType>::max(),
			std::numeric_limits<ScalarValueType>::max(), -std::numeric_limits<
					ScalarValueType>::max(),
			std::numeric_limits<ScalarValueType>::max(), -std::numeric_limits<
					ScalarValueType>::max() };
	// Vector of solutions
	std::vector<double> vSol, vXgrad, vYgrad, vZgrad;
	std::vector<fvmathParser::MathElement> mathElems;
	fvmathParser::MathCalculator::ONP(_solution.sFormula, _solution.nrEquetions,
			mathElems);

	Mesh::arElConstIter it = _pmesh.GetElems().begin();
	Mesh::arElConstIter ite = _pmesh.GetElems().end();
	std::vector<fvmath::CVec3d> nodeCoords;	// vector container for coords
	std::vector<short> indexNodes;		// vecot container for indexes of nodes
	while (it != ite) {
		int nel(it->elemId.eid);
		// Element base function type
		const int base = get_base_type(this->idx(), nel);
		// Get its corresponding pdeg
		(void) get_el_pdeg(this->idx(), nel, pdeg);
		// Get element real coordinates
		nr_nodes = _pmesh.GetElementCoordinates(nel, NULL, Node_coor);
		// Get element reference coordinate and number of refinamet
		const int itr = _pmesh.GetReferenceCoordinates(nel, nr_nodes, pdeg[0],
				base, nodeCoords, indexNodes) / nr_nodes;
		// Get element dofs
		int nr_dofs = get_element_dofs(this->idx(), nel, _solution.currSolution,
				Dofs_loc);
		// Loop over refinement element
		for (int nref = 0; nref < itr; nref++) {
			// Get the correct reference element
			double * Xloc = (double *) (nodeCoords.data() + nref * nr_nodes);
			for (int ino = 0; ino < nr_nodes; ++ino) {
				//mfp_debug("Xloc[%d] = { %lf, %lf, %lf}\n",ino,Xloc[3*ino],Xloc[3*ino+1],Xloc[3*ino+2]);
				if (iaux == 2) {
					(void) apr_elem_calc_3D_mod(iaux, _solution.nrEquetions,
							pdeg, base, &Xloc[3 * ino], Node_coor, Dofs_loc,
							Base_phi, Base_dphix, Base_dphiy, Base_dphiz,
							&Xcoor[3 * ino], U_val, U_x, U_y, U_z, NULL);
				} else {
					(void) apr_elem_calc_3D_mod(iaux, _solution.nrEquetions,
							pdeg, base, &Xloc[3 * ino], Node_coor, Dofs_loc,
							Base_phi,
							NULL, NULL, NULL, &Xcoor[3 * ino], U_val,
							NULL, NULL, NULL, NULL);
				}
				//mfp_debug("U_val = %lf for %d\n",U_val[0],ino);

				vSol.clear();
				for (int sol = 0; sol < _solution.nrEquetions; ++sol) {
					vSol.push_back(U_val[sol]);
				}
				double dTmp;
				fvmathParser::MathCalculator::ONPCalculate(mathElems, vSol,
						dTmp);
				vals[0] = vals[0] < dTmp ? vals[0] : dTmp;
				vals[1] = vals[1] > dTmp ? vals[1] : dTmp;
			}			// for ino
		}			// for nref
		++it;
	}			// while it

	// Store min/max values
	_values.first = vals[0];
	_values.second = vals[1];
//		_xgrads.first  = vals[2];
//		_xgrads.second = vals[3];
//		_ygrads.first  = vals[4];
//		_ygrads.second = vals[5];
//		_zgrads.first  = vals[6];
//		_zgrads.second = vals[7];

	//mfp_debug("Minimal Value: %lf\n",vals[0]);
	//mfp_debug("Maximal Value: %lf\n",vals[1]);

}

void Field::CalculateMinMaxValues() const {
	if (_minValue != -std::numeric_limits<double>::max()
			&& _maxValue != std::numeric_limits<double>::max()) {
		return;
	}

	for (size_t i = 0; i < this->_pmesh.GetElems().size(); ++i) {
		// Create an element
	}
	/*
	 ScalarValueType tempMin = std::numeric_limits<ScalarValueType>::max();
	 ScalarValueType tempMax = -std::numeric_limits<ScalarValueType>::max();

	 const int numDivisions = 10;
	 CoordType h = 2.0/numDivisions;

	 for(unsigned int i = 0; i < numDivisions; ++i)
	 {
	 for(unsigned int j = 0; j < numDivisions; ++j)
	 {
	 for(unsigned int k = 0; k < numDivisions; ++k)
	 {
	 // ElVis::TensorPoint tp(-1.0 + i*h, -1.0 + j*h, -1.0 + k*h);
	 ScalarValueType p = f(tp);

	 if( p < tempMin )
	 {
	 tempMin = p;
	 }

	 if( p > tempMax )
	 {
	 tempMax = p;
	 }
	 }
	 }
	 }

	 m_minValue = tempMin;
	 m_maxValue = tempMax;
	 */
}

void Field::RenderDG(void * ptr)
{
	if (!ptr) return;
	VtxAccumulator* accum_ptr = reinterpret_cast<VtxAccumulator *>(ptr);
	//fp_log_debug("RenderDG\n");
	static const int FV_MAX_POINTS = 1000000;
	std::vector<double> vEvaluated;
	int pdeg[PDC_MAXEQ];
	int nr_nodes, nr_dofs;
	const int iaux = 2;
	double Node_coor[3 * MMC_MAXELVNO]; /* coord of nodes of El */
	double Dofs_loc[APC_MAXELSD]; /* element solution dofs */
	double Xcoor[3]; /* global coord of gauss point */
	double U_val[PDC_MAXEQ]; /* computed solution */
	double U_x[PDC_MAXEQ]; /* gradient of computed solution */
	double U_y[PDC_MAXEQ]; /* gradient of computed solution */
	double U_z[PDC_MAXEQ]; /* gradient of computed solution */
	double Base_phi[APC_MAXELVD]; /* basis functions */
	double Base_dphix[APC_MAXELVD]; /* x-derivatives of basis function */
	double Base_dphiy[APC_MAXELVD]; /* y-derivatives of basis function */
	double Base_dphiz[APC_MAXELVD]; /* y-derivatives of basis function */
	// Vector of solutions
	std::vector<double> vSol;
	std::vector<fvmathParser::MathElement> mathElems;

	//std::cout<<"Evaluating formula is: " << _solution.sFormula << std::endl;
	fvmathParser::MathCalculator::ONP(_solution.sFormula, _solution.nrEquetions,
			mathElems);

	//Mesh::BndElemsConstIter  it = _pmesh.GetBoundaryElems().begin();
	//Mesh::BndElemsConstIter ite = _pmesh.GetBoundaryElems().end();

	Mesh::arElConstIter it = _pmesh.GetElems().begin();
	const Mesh::arElConstIter ite = _pmesh.GetElems().end();

	// Resize containers to the larges evaliabe number according pdeg = 707
	std::vector<Node_t>  nodeCoords(121);	// vector container for coords (9+2) * (9+2) points
	std::vector<ubyte_t> indexNode(207);// vecot container for indexes of nodes 9*21 + 16

	//const fvmath::Vec3d * ploc = reinterpret_cast<const fvmath::Vec3d *>(&XlocPrizm[0]);
	//double XlocDest[18];
	// Start loop over bounday elements
	//VtxAccumulator& vtxAccuml = _pobject->GetCurrentVtxAccumulator();
	std::vector<Vertex>& vMeshVertices = accum_ptr->getVertices();

	int vertex_counter[2] = {0, 0};
	vEvaluated.reserve(FV_MAX_POINTS);
	int my_couter = 0;
	double minVal = LARGE_F;
	double maxVal = -minVal;
	Vertex vt;
	while (it != ite) {
		// Get element id form it
		int nel(it->elemId.eid);
		// Specify type of an alement
		const int nr_faces = it->elemId.is_tetra() ? 4 : 5;
		// Get element real coordinates
		nr_nodes = _pmesh.GetElementCoordinates(nel, NULL, Node_coor);
		// Get element dofs
		nr_dofs = get_element_dofs(this->idx(), nel, _solution.currSolution,
				Dofs_loc);
		// Element base function type
		const int base = get_base_type(this->idx(), nel);
		// Get its corresponding pdeg
		(void) get_el_pdeg(this->idx(), nel, pdeg);
		// Specify a pdeg and number of discretizations
		int nptsx, nptsy;
		if (base == APC_BASE_TENSOR_DG) {
			nptsx = pdeg[0] % 100 + 1;
			nptsy = pdeg[0] / 100 + 1;
		} else {	// APC_BASE_COMPLETE_DG
			nptsx = pdeg[0] + 1;
			nptsy = pdeg[0] + 1;
		}
		nptsx += dg_density;
		nptsy += dg_density;
		//printf("Pdeg od pdeg[0] = %d\n",pdeg[0]);
		// Lenghts of segments
		double dlx = 1.0 / nptsx; // in x and y directions
		double dly = 1.0 / nptsy; // in z directions

		// increase to real number of points, 
		// because counter of points is +1
		nptsx++;
		nptsy++;

		// Loop over boundary faces
		for (int nfa(0); nfa < nr_faces; nfa++) {

			// Skip a non boundary face
			if (!IS_SET_FACEN(it->elemId.id, nfa))
				continue;

			bool cond(nfa < 2);
			const int nno  = cond ? (nptsx + 1) * nptsx / 2 : nptsx * nptsy;
			const int dest = cond ? vtxTriangle : vtxQuad;
			// Clear
			nodeCoords.clear();
			indexNode.clear();

			unsigned int size;
			//CVec3f Normal;
			switch (nfa) {
			case 0:
				// Tesselate face
				size = make_plane_XY0(nptsx, nptsx, dlx, dlx, nodeCoords,
						indexNode);
				//Normal = fvmath::GetNormal2Plane<float, double>(&Node_coor[0],
				//		&Node_coor[3], &Node_coor[6]);
				break;
			case 1:
				// Tesselate face
				size = make_plane_XY1(nptsx, nptsx, dlx, dlx, nodeCoords,
						indexNode);
				//Normal = fvmath::GetNormal2Plane<float, double>(&Node_coor[9],
				//		&Node_coor[15], &Node_coor[12]);
				break;
			case 2:
				// Tesselate face
				size = make_plane_XZ(nptsx, nptsy, dlx, dly, nodeCoords,
						indexNode);
				//Normal = fvmath::GetNormal2Plane<float, double>(&Node_coor[0],
				//		&Node_coor[9], &Node_coor[3]);
				break;
			case 3:
				// Tesselate face
				size = make_plane_XYZ(nptsx, nptsy, dlx, dlx, nodeCoords,
						indexNode);
				//Normal = fvmath::GetNormal2Plane<float, double>(&Node_coor[3],
				//		&Node_coor[12], &Node_coor[6]);
				break;
			case 4:
				// Tesselate face
				size = make_plane_YZ(nptsx, nptsy, dlx, dlx, nodeCoords,
						indexNode);
				//Normal = fvmath::GetNormal2Plane<float, double>(&Node_coor[6],
				//		&Node_coor[15], &Node_coor[0]);
				break;
			}					// switch
			// Add count of primitives
			//mfp_debug("After tesselation\n");
			assert(size == indexNode.size());
			//1
			//VtxAccumulator::LocalIndices& it(accum_ptr->getLocalIndices(dest));
			VtxAccumulator::LocalIndices& it(accum_ptr->getIndices(dest));
			for (size_t i(0); i < indexNode.size(); ++i) {
				it.push_back(vertex_counter[dest] + indexNode[i]);
				//std::cout << "dest" << dest << " index" << i << " = " << it[i] << std::endl;
			}
			//mfp_debug("Adding size = %u nno = %d\n",size, nno);
			accum_ptr->addPrimitiveCount(size,dest);
			Node_t * pXloc(nodeCoords.data());
			// Loop over discretizated local points
			for (int ino(0); ino < nno; ino++) {
				if (iaux == 2) {
					(void) apr_elem_calc_3D_mod(iaux, _solution.nrEquetions,
							pdeg, base, pXloc[ino].position.v, Node_coor, Dofs_loc,
							Base_phi, Base_dphix, Base_dphiy, Base_dphiz, Xcoor,
							U_val, U_x, U_y, U_z, NULL);
				} else {
					(void) apr_elem_calc_3D_mod(iaux, _solution.nrEquetions,
							pdeg, base, pXloc[ino].position.v, Node_coor, Dofs_loc,
							Base_phi,
							NULL, NULL, NULL, Xcoor, U_val,
							NULL, NULL, NULL, NULL);
				}

				// Evaluate solution formula
				vSol.clear();

				for (int sol = 0; sol < _solution.nrEquetions; ++sol) {
					vSol.push_back(U_val[sol]);
				}

				double dTmp;
				fvmathParser::MathCalculator::ONPCalculate(mathElems, vSol,	dTmp);
				minVal = dTmp < minVal ? dTmp : minVal;
				maxVal = dTmp > maxVal ? dTmp : maxVal;
				//vEvaluated.push_back(dTmp);

				// Add vertex
				vt.position = CVec3f(Xcoor);
				vt.info = pXloc[ino].info;
				vt.value = static_cast<float>(dTmp);
				//std::cout << "vertex: " << vt.position << " info: " << vt.info << std::endl;
				accum_ptr->addVertex(vt,dest);

				//FV_ASSERT(dTmp>=_values.first && dTmp<=_values.second);
			}						// fo ino

			//accum_ptr->addBaseIndex(vertex_counter[dest],dest);
			vertex_counter[dest] += nno;
		}// for nfa
		//}// else pdeg
		++it;
	}			//while
	//mfp_debug("DG minimal: %lf maximal: %lf values\n",minVal,maxVal);
	// Specify min/max values
	_values.first = minVal;
	_values.second = maxVal;
	// Init legend
	//FV_CHECK_ERROR_GL();
	// Update the legend
	Legend& rleg = ViewManagerInst().GetLegend();
	rleg.Set(minVal, maxVal, minVal, maxVal);
	//FV_CHECK_ERROR_GL();
	// Update colors
	size_t ntri_vertices = accum_ptr->getVertices(vtxTriangle).size();
	Vertex *vt_p = accum_ptr->getVertices(vtxTriangle).data();
	for (size_t i(0); i < ntri_vertices; i++) {
		//std::cout << "\nVertex_tri: " << i << " value: " << vt_p->value << " position: " << vt_p->position;
		rleg.GetColor(double(vt_p->value));
		vt_p->color.x = rleg.color.rgb.r;
		vt_p->color.y = rleg.color.rgb.g;
		vt_p->color.z = rleg.color.rgb.b;
		++vt_p;
	}
	//mfp_debug("Number of tri vertices: %u\n",ntri_vertices);
	ntri_vertices = accum_ptr->getVertices(vtxQuad).size();
	vt_p = accum_ptr->getVertices(vtxQuad).data();
	for (size_t i(0); i < ntri_vertices; i++) {
		//std::cout << "\nVertex_qua: " << i << " value: " << vt_p->value << " position: " << vt_p->position;
		rleg.GetColor(double(vt_p->value));
		vt_p->color.x = rleg.color.rgb.r;
		vt_p->color.y = rleg.color.rgb.g;
		vt_p->color.z = rleg.color.rgb.b;
		++vt_p;
	}
	//mfp_debug("Number of qu vertices: %u\n",ntri_vertices);
	//accum_ptr->UseVBO() = ViewManagerInst().IsVBOUsed();
	accum_ptr->aRenderTraingleStrips = true;
	//FV_CHECK_ERROR_GL();
}

void Field::RenderSTD(void *ptr)
{
	if (!ptr) return;
	VtxAccumulator* accum_ptr = reinterpret_cast<VtxAccumulator *>(ptr);
	//mfp_log_debug("RenderSTD\n");
	int pdeg[PDC_MAXEQ];
	int nr_nodes, nr_faces, w = 0;
	const int iaux = 2;
	const int *elem_p(NULL);
	int Nodes[MMC_MAXELVNO + 1]; /* indices of nodes */
	double Node_coor[3 * MMC_MAXELVNO]; /* coord of nodes of El */
	double Dofs_loc[APC_MAXELSD]; /* element solution dofs */
	double Xcoor[18]; /* global coord of gauss point */
	double U_val[PDC_MAXEQ]; /* computed solution */
	double U_x[PDC_MAXEQ]; /* gradient of computed solution */
	double U_y[PDC_MAXEQ]; /* gradient of computed solution */
	double U_z[PDC_MAXEQ]; /* gradient of computed solution */
	double Base_phi[APC_MAXELVD]; /* basis functions */
	double Base_dphix[APC_MAXELVD]; /* x-derivatives of basis function */
	double Base_dphiy[APC_MAXELVD]; /* y-derivatives of basis function */
	double Base_dphiz[APC_MAXELVD]; /* y-derivatives of basis function */
	double *Xloc;
	double dTmp;
	// Vector of solutions
	std::vector<double> vSol;
	std::vector<fvmath::CVec3f> mappedColors(MMC_MAXELVNO);
	std::vector<fvmathParser::MathElement> mathElems;
	std::vector<int> vNodes(MMC_MAXELVNO + 1);
	//Object*& pobject = _pmesh.GetG
	//VtxAccumulator& vtxAccum = _pobject->GetCurrentVtxAccumulator();
	std::vector<Vertex>& vMeshVertices = accum_ptr->getVertices(vtxEdge);
	// First update legend
	Legend& rleg = ViewManagerInst().GetLegend();
	rleg.Set(GetValues().first, GetValues().second, GetValues().first,
			GetValues().second);
	// Prepare mathematics formula
	fvmathParser::MathCalculator::ONP(_solution.sFormula, _solution.nrEquetions,
			mathElems);

	Mesh::arElConstIter it = _pmesh.GetElems().begin();
	const Mesh::arElConstIter ite = _pmesh.GetElems().end();
	std::vector<fvmath::CVec3d> nodeCoords;	// vector container for coords
	//std::vector<short> indexNodes;		// vecot container for indexes of nodes
	//_pobject->SetPointSize(2.0);
	fvmath::CVec3f normal;
	unsigned int iv = 0;
	int target;
	//mfp_debug("Before while loop\n");
	while (it != ite) {
		// Retrieve element index
		int nel(it->elemId.eid);
		// Element base function type
		const int base = get_base_type(this->idx(), nel);
		// Set reference element
		if (IS_TETRA(it->elemId.id)) {
			Xloc = (double*) &XlocTetra[0];
			elem_p = Mesh::tetra[0];
			nr_faces = 4;
		} else {
			Xloc = (double*) &XlocPrizm[0];
			elem_p = Mesh::prizm[0];
			nr_faces = 5;
		}
		// Get element dofs
		int nr_dofs = get_element_dofs(this->idx(), nel, _solution.currSolution,
				Dofs_loc);
		// Get its corresponding pdeg
		(void) get_el_pdeg(this->idx(), nel, pdeg);
		// Get element real coordinates
		nr_nodes = _pmesh.GetElementCoordinates(nel, Nodes, Node_coor);
		// Calculate values

		for (int ino = 0; ino < nr_nodes; ++ino) {
			const int nno = Nodes[ino + 1] - 1;
			//mfp_debug("Calculating for node: %d\n",nno);
			if (accum_ptr->isVertexInitialized(nno)) {
				continue;
			}
//mfp_debug("In vertces %d\n",nno);
			if (iaux == 2) {
				apr_elem_calc_3D_mod(iaux, _solution.nrEquetions, pdeg, base,
						&Xloc[3 * ino], Node_coor, Dofs_loc, Base_phi,
						Base_dphix, Base_dphiy, Base_dphiz, Xcoor, U_val, U_x,
						U_y, U_z, NULL);
			} else {
				apr_elem_calc_3D_mod(iaux, _solution.nrEquetions, pdeg, base,
						&Xloc[3 * ino], Node_coor, Dofs_loc, Base_phi,
						NULL, NULL, NULL, Xcoor, U_val,
						NULL, NULL, NULL, NULL);
			}
			//mfp_debug("U_val = %lf\n",U_val[0]);
			vSol.clear();

			for (int sol = 0; sol < _solution.nrEquetions; ++sol)
				vSol.push_back(U_val[sol]);

			fvmathParser::MathCalculator::ONPCalculate(mathElems, vSol, dTmp);
			//mfp_debug("Value: %lf\n",dTmp);
			//vMeshVertices[nno].edge_flag = 1.0f;
			vMeshVertices[nno].value = static_cast<float>(dTmp);
			rleg.GetColor(dTmp);
			vMeshVertices[nno].color.x = rleg.color.rgb.r;
			vMeshVertices[nno].color.y = rleg.color.rgb.g;
			vMeshVertices[nno].color.z = rleg.color.rgb.b;
			//std::cout << "Color" << ino << " = "
				//	<< fvmath::CVec3f(rleg.color.rgb.v) << "\n";

		}
		//mfp_debug("After render\n");
		// Now we have to add indices of vertces on bounday faces
		for (int nf = 0; nf < nr_faces; ++nf) {
			//mfp_debug("nf = %d elmId = %x \n",nf,it->elemId.id);
			//std::cout << "Is face: " << IS_SET_FACEN(it->elemId.id,nf) << std::endl;
			// Check if current face is boundary
			if (!IS_SET_FACEN(it->elemId.id, nf))
				continue;
			// for each face add triangle; for quads there will be 2 triangles
			int of = nf * (nr_faces - 1);
			//mfp_debug("of = %d %d\n",of,nr_faces);
			const int v0 = elem_p[of++];
			const int v1 = elem_p[of++];
			const int v2 = elem_p[of];
			const int n0 = Nodes[v0+1]-1;
			const int n1 = Nodes[v1+1]-1;
			const int n2 = Nodes[v2+1]-1;
			assert(n0 >= 0 && n1 >= 0 && n2 >= 0);
			//normal = fvmath::GetNormal2Plane<float, double>(
				//	&Node_coor[3*v0], &Node_coor[3*v2], &Node_coor[3*v1]);
			//vMeshVertices[n0].normal = normal;
			//vMeshVertices[n1].normal = normal;
			//vMeshVertices[n2].normal = normal;
		    //mfp_debug("nf: %d vertices: %d %d %d ",nf,n0,n1,n2);
			//std::cout <<normal << std::endl;
			//mfp_debug("Adding triangle: %d %d %d\n",v0,v2,v1);
			target = (nr_faces == 5 && nf >= 2) ? vtxQuad : vtxTriangle;
			accum_ptr->addIndexOfNode(n0,target);
			accum_ptr->addIndexOfNode(n1,target);
			accum_ptr->addIndexOfNode(n2,target);
			if (nr_faces == 5 && nf >= 2) {
				// Quad face = 2 triangles to add
				const int v3 = elem_p[++of];
				const int n3 = Nodes[v3+1]-1;
				//normal = fvmath::GetNormal2Plane<float, double>(
				//			&Node_coor[3*v2], &Node_coor[3*v0], &Node_coor[3*v3]);
				//vMeshVertices[n3].normal = normal;
				/// mfp_debug("nf2: %d vertices: %d %d %d ",nf,n2,n3,n0);
				//std::cout << normal << std::endl;
				//mfp_debug("Adding second triangle: %d %d %d\n",v2,v0,v3);
				assert(n3 >= 0);
				accum_ptr->addIndexOfNode(n2,target);
				accum_ptr->addIndexOfNode(n3,target);
				accum_ptr->addIndexOfNode(n0,target);
			}
		}
		++it;
	}
	//mfp_debug("After adding indices\n");
	//FV_CHECK_ERROR_GL();
	// setup VBO
	accum_ptr->aRenderTraingles = true;
	//mfp_debug("At the end of render\n");

}

size_t Field::PackCoeffs(std::vector<mfvFloat_t>& Coeffs,std::vector<int>& Degree)
{
	size_t num_elems = _pmesh.GetElems().size();
	assert(num_elems > 0);

	// Get information about amount of all coefficients
	// and resize the buffer
	size_t total_ndofs = CountCoeffs(&Degree);
	Coeffs.resize(total_ndofs);

	// Accumulate coefficients
	size_t nr_dofs = 0;
	for (size_t nel = 0; nel < num_elems; ++nel) {
		Elem_Info *info_ptr = &_pmesh.GetElems()[nel];
		int el_id = info_ptr->elemId.eid;
		nr_dofs += GetElementDofs(el_id,&Coeffs[nr_dofs]);
	}

	return nr_dofs;
}

uint32_t Field::FillArrayWithCoeffs(const Field *pField, CoordType Coeffs[]) {
	Mesh::arElConstIter itb(pField->_pmesh.GetElems().begin());
	const Mesh::arElConstIter ite(pField->_pmesh.GetElems().end());
	uint32_t totalNumCoeffs(0);
	for (; itb != ite; ++itb) {
		int elId = itb->elemId.eid;
		totalNumCoeffs += pField->GetElementDofs(elId, &Coeffs[totalNumCoeffs]);
	}
	return totalNumCoeffs;
}

int Field::GetDegreeVector(int nel, int Pdeg[]) const {
	int pdeg = GetElementDegree(nel);
	int base = GetElementBaseType(nel);
	switch (base) {
	case APC_BASE_TENSOR_DG:
		Pdeg[0] = Pdeg[1] = pdeg % 100;
		Pdeg[2] = pdeg / 100;
		break;
	case APC_BASE_COMPLETE_DG:
	case APC_BASE_PRISM_STD:
	case APC_BASE_TETRA_STD:
		Pdeg[0] = Pdeg[1] = Pdeg[2] = pdeg;
		break;
	default:
		exit(-1);
	}

	return base;
}

int Field::CalculateSolution(int control, int pdeg, int base, double* ElCoords,
		double* Ploc, double* Coeffs, double* Sol, double* DSolx, double* DSoly,
		double* DSolz) const {
	double BasePhi[APC_MAXELSD];
	double BaseDPhix[APC_MAXELSD];
	double BaseDPhiy[APC_MAXELSD];
	double BaseDPhiz[APC_MAXELSD];

	if (control == 2) {
		return apr_elem_calc_3D_mod(control, _solution.nrEquetions, &pdeg, base,
				Ploc, ElCoords, Coeffs, BasePhi, BaseDPhix, BaseDPhiy,
				BaseDPhiz, NULL, Sol, DSolx, DSoly, DSolz, NULL);
	}
	return apr_elem_calc_3D_mod(control, _solution.nrEquetions, &pdeg, base,
			Ploc, ElCoords, Coeffs, BasePhi,
			NULL, NULL, NULL, NULL, Sol,
			NULL, NULL, NULL, NULL);

}

}				// end namespace
