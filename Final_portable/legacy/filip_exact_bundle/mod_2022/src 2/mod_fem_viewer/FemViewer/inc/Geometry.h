/*
 * Geometry.h
 *
 *  Created on: 29 sty 2014
 *      Author: dwg
 */

#ifndef GEOMETRY_H_
#define GEOMETRY_H_
#include <stdint.h>
#include <vector>
#include "types.h"
#include "Log.h"
#include "fv_config.h"
#include "ElemId.hpp"
#include "Enums.h"
#include "MathHelper.h"
#include "Matrix.h"
#include "BBox3D.h"
#include "Ray.h"

#include "Mesh.h"
#include "Field.h"


namespace FemViewer {
using namespace fvmath;

// Forward declarations
extern uint64_t numRayTrianglesTests;
extern uint64_t numRayTrianglesIsect;

template<typename TReal = CoordType>
bool intersectTriangle(
		const Ray<TReal> &r,
		const Vec3<TReal>& v0, const Vec3<TReal>& v1, const Vec3<TReal>& v2,
		TReal tuv[]);

template<typename TReal = CoordType>
bool intersectQuad(
		const Ray<TReal> &r,
		const Vec3<TReal>& v0, const Vec3<TReal>& v1, const Vec3<TReal>& v2, const Vec3<TReal>& v3,
		TReal tuv[]);

struct IsectData {
	float t;
	float u, v;
	uint32_t index;
	CVec3f N;
	IsectData() : t(0), u(0), v(0), index(0) {}
};


template<typename _Ty>
struct prism_info {
	//fvmath::Vec3<_Ty> bmin;
	//fvmath::Vec3<_Ty> bmax;
	CVec3<_Ty> vts[6];
	Vec4<_Ty> pls[5];
	_Ty min_v;
	_Ty max_v;
	//_Ty	mtx[M*N];
};

template<typename _Ty>
struct coeffs_info {
	short pdeg;
	short base;
	int   nr_shap;
	_Ty coeffs[MAX_NUM_DOFS];
};



 typedef union {
	struct { float tmin, tmax; };
	float data[2];
 } coef_t;

 typedef struct {
	 coef_t coef;
	 ElemId<id_t> id;
 } intersect_t ;

 class mfvBaseObject {
   public:
	 static const Mesh*  parentMeshPtr;
	 static const Field* parentFieldPtr;
	 virtual ~mfvBaseObject() {}
	 virtual const BBox3D& getBounds() const = 0;
	 virtual bool  intersect(const Ray<float>& ray,isect_info_t *isectData) const = 0;
	 virtual void  computeBounds(const Vec3f &planeNormal, float &dnear, float &dfar) const {};
	 virtual CVec3f getCenter() const = 0;
	 virtual void calculateNormals() = 0;
 };

template<class TReal = CoordType,
		 class TIndex = id_t,
		 int NUM_VERTICES = 3>
class mfvObject : public mfvBaseObject {
  public:
	static const size_t numVertices = NUM_VERTICES;
	typedef TReal  real_t;
	typedef ElemId<TIndex> index_t;
	typedef ScalarValueType value_type;
	typedef std::vector<value_type> vListType;
	template<typename U>
	static int convert(TReal in[],U out[]);
	mfvObject(const index_t id =0,const Matrix<ScalarValueType>& po2w = Matrix<ScalarValueType>::Identity)
	: minValue(0.0)
	, maxValue(0.0)
	, solCoeffs()
	, pdeg(101)
	, index(id)
	, objectToWorld(po2w)
	, worldToObject() {
		//mfp_debug("ctr %d\n",index.eid);
		if (parentFieldPtr != NULL) this->getCoefficients();
	}
	virtual ~mfvObject() { }
	index_t& getId() { return this->index; }
	int degree() {return pdeg;}
	double minV() { return minValue; }
	double maxV() { return maxValue; }
	const BBox3D& getBounds() const { return bbox; }
	virtual void  setBounds() {
		if (bbox.isInitialized()) return;
		for (size_t i(0); i < NUM_VERTICES; ++i) bbox += this->vertices[i];
	}
    CVec3<real_t> getCenter() const {
    	const real_t f = real_t(1.0/NUM_VERTICES);
    	CVec3<real_t> center(this->vertices[0]);
    	for (size_t i(1); i < NUM_VERTICES; ++i) center += this->vertices[i];
    	center *= f;
    	return center;
    }
    virtual void getCoefficients();
protected:
    mutable value_type minValue;
    mutable value_type maxValue;
    vListType solCoeffs;
    int pdeg;
    int base;
    int numShap;
	index_t index;
	Matrix<ScalarValueType> objectToWorld, worldToObject;
	Vec3<real_t> vertices[NUM_VERTICES];
	int idNodes[NUM_VERTICES+1];
	BBox3D bbox;
};

template<class TReal,class TIndex,int NUM_VERTICES>
template<class U>
int mfvObject<TReal,TIndex,NUM_VERTICES>::convert(TReal InV[],U OutV[])
{
	for (size_t i(0); i < numVertices; ++i) {
		OutV[i] = static_cast<U>(InV[i]);
	}

	return numVertices;
}

template<class TReal,class TIndex,int NUM_VERTICES>
void mfvObject<TReal,TIndex,NUM_VERTICES>::getCoefficients()
{
	//printf("coefficients\n");
	assert(parentFieldPtr != NULL);
	int nel = static_cast<int>(index.eid);

	int nshp =parentFieldPtr->GetNumberOfShapeFunc(nel,&pdeg);
	//printf("num_shup = %d\n",nshp);
	solCoeffs.resize(nshp);
	parentFieldPtr->GetElementDofs<ScalarValueType>(nel,solCoeffs.data());
	assert(solCoeffs.size() == nshp);
	//printf("num_shup = %d\n",nshp);
}

#define NUM_TRIANGLE_VERTICES 3
class Triangle : public mfvObject<CoordType,id_t,NUM_TRIANGLE_VERTICES>
{
public:
	Triangle(const id_t& id1,const id_t& id2,const id_t& id3);
	bool intersect(const Ray<CoordType> &r,isect_info_t* isectData) const
	{
#ifdef MOLLER_TRUMBORE
		CVec3<TReal> edge1 = this->vertices[1] - this->vertices[0];
		CVec3<TReal> edge2 = this->vertices[2] - this->vertices[0];
		CVec3<TReal> pvec = r.dir * edge2;
		CoordType det = Dot(edge1, pvec);
		if (det == CoordType(0)) return false;
		CoordType invDet = CoordType(1) / det;
		CVec3<CoordType> tvec = r.orig - this->vertices[0];
		isectData.u = dot(tvec, pvec) * invDet;
		if (isectData.u < CoordType(0) || isectData.u > CoordType(1)) return false;
		CVec3<CoordType> qvec = cross(tvec, edge1);
		isectData.v = dot(r.dir, qvec) * invDet;
		if (isectData.v < CoordType(0) || isectData.u + isectData.v > CoordType(1)) return false;
		isectData.t = dot(edge2, qvec) * invDet;
#else
		CVec3<CoordType> v0v1(this->vertices[1] - this->vertices[0]);
		CVec3<CoordType> v0v2(this->vertices[2] - this->vertices[0]);
		CVec3<CoordType> N = v0v1 * v0v2;
		CoordType nDotRay = Dot(N, r.dir);
		if (nDotRay == CoordType(0) || (nDotRay > CoordType(0) && isSingledSided)) return false; // ray parallel to triangle
		CoordType d = Dot(N, this->vertices[0]);
		CoordType t = -(Dot(N, r.orig) + d) / nDotRay;
		if (t < CoordType(0)) return false; // ray behind triangle

		// inside-out test
		CVec3<CoordType> Phit = r(t);

		// inside-out test edge0
		CVec3<CoordType> v0p = Phit - this->vertices[0];
		CoordType v = Dot(N, v0v1 * v0p);
		if (v < CoordType(0)) return false; // P outside triangle

		// inside-out test edge1
		CVec3<CoordType> v1p = Phit - this->vertices[1];
		CVec3<CoordType> v1v2 = this->vertices[2] - this->vertices[1];
		CoordType w = Dot(N, v1v2 * v1p);
		if (w < CoordType(0)) return false; // P outside triangle

		// inside-out test edge2
		CVec3<CoordType> v2p = Phit - this->vertices[2];
		CVec3<CoordType> v2v0 = this->vertices[0] - this->vertices[2];
		CoordType u = Dot(N, v2v0 * v2p);
		if (u < CoordType(0)) return false; // P outside triangle

		CoordType nlen2 = Dot(N, N);
		isectData->t = t;
		isectData->u = u / nlen2;
		isectData->v = v / nlen2;
#endif
		return true;
	}
private:
	Vec4<CoordType> normal;
	void calculateNormals();
	bool isSingledSided;
};

#define NUM_TETRA_FACES 4
#define NUM_TETRA_VERTICES 4
class Tetra : public mfvObject<CoordType,id_t,NUM_TETRA_VERTICES> {
  public:
	template<typename T>
	static BBox3D BoundingBox(const fvmath::Vec3<T>* elCoords);
	template<typename T>
	static fvmath::CVec3<T> Centrum(const fvmath::Vec3<T>* elCoords);
	Tetra(const id_t& id);
	bool intersect(const Ray<CoordType>& r,isect_info_t *isectData) const {
		isect_info_t tuv[4];
		unsigned sides = 0U;
		for (unsigned i(0); i < NUM_TETRA_FACES; ++i) {
			bool result(false);
			int i0 = Mesh::tetra[i][0];
			int i1 = Mesh::tetra[i][1];
			int i2 = Mesh::tetra[i][2];
			result = intersectTriangle(r,this->vertices[i0],this->vertices[i1],this->vertices[i2],(CoordType*)&tuv[i]);
			if (result && (r.tmin <= tuv[i].t && tuv[i].t <= r.tmax)) {
				tuv[i].side = i;
			} else tuv[i].side = -1;
		}
		unsigned mn_i(0U);
		do{
			if (tuv[mn_i].side != -1) break;
		}while(mn_i++ < 4);
		unsigned mx_i = mn_i;
		for (unsigned i = mn_i+1;i<4;++i) {
			if (tuv[mn_i].t > tuv[i].t) { mn_i = i; }
			if (tuv[mx_i].t < tuv[i].t) { mx_i = i; }
		}
		el_isect_info_t* pis = static_cast<el_isect_info_t *>(isectData);
		pis->t = tuv[mn_i].t;
		pis->u = tuv[mn_i].u;
		pis->v = tuv[mn_i].v;
		pis->out.t = tuv[mx_i].t;
		pis->out.u = tuv[mn_i].u;
		pis->out.v = tuv[mx_i].v;
		return (mn_i < mx_i);
	}
	//void computeBounds(const CVec3f &planeNormal, float &dnear, float &dfar) const {}
protected:
	Vec4<CoordType> normals[NUM_TETRA_FACES];
	void calculateNormals();
	int  setCoordinates() {
		int lindex = static_cast<int>(index.eid);
		this->parentMeshPtr->GetElementCoordinates(lindex, this->idNodes, this->vertices[0].v);
		calculateNormals();
		return lindex;
	}
	//void getCoefficients();
};

template<typename TCoord>
BBox3D Tetra::BoundingBox(const fvmath::Vec3<TCoord>* v)
{
	using namespace fvmath;
	typedef fvmath::Vec3<TCoord> vec3;
	BBox3D bb;
	bb.mn.x = fv_min(fv_min(fv_min(fv_min(v[0].x,v[1].x),v[2].x),v[3].x),v[4].x);
	bb.mn.y = fv_min(fv_min(fv_min(fv_min(v[0].y,v[1].y),v[2].y),v[3].y),v[4].y);
	bb.mn.z = fv_min(fv_min(fv_min(fv_min(v[0].z,v[1].z),v[2].z),v[3].z),v[4].z);
	bb.mx.x = fv_max(fv_max(fv_max(fv_max(v[0].x,v[1].x),v[2].x),v[3].x),v[4].x);
	bb.mx.y = fv_max(fv_max(fv_max(fv_max(v[0].y,v[1].y),v[2].y),v[3].y),v[4].y);
	bb.mx.z = fv_max(fv_max(fv_max(fv_max(v[0].z,v[1].z),v[2].z),v[3].z),v[4].z);
	bb.setInitialized(true);
	return bb;
}

template<typename T>
fvmath::CVec3<T> Tetra::Centrum(const fvmath::Vec3<T>* elCoords)
{
	CVec3<T> c;
	T inv_6 = 1. / NUM_TETRA_VERTICES;
	for (int i(0); i < NUM_TETRA_VERTICES; ++i) c += elCoords[i];
    c *= inv_6;
    return c;
}

#define NUM_PRISM_VERTICES 6
#define NUM_PRISM_FACES 5

class Prizm : public mfvObject<CoordType,id_t,NUM_PRISM_VERTICES> {
public:
	template<typename TCoord>
	static BBox3D BoundingBox(fvmath::Vec3<TCoord> *v);
	template<typename T>
	static fvmath::CVec3<T> Centrum(const fvmath::Vec3<T>* elCoords);
	template<typename T>
	static int teselateReference(const int Pdeg[3],CVec3<T> RefPoints[]);
	template<typename T>
	static int teselateWorld(const int base,const int Pdeg[3],const CVec3<T> ElCoords[],std::vector<CVec3<T> >* TesPoints);
	static int getNumberOfShapeFunctions(const int Order[],int base);
	Prizm(const index_t& id);
	virtual ~Prizm() {  }
	bool intersect(const Ray<CoordType>& r,isect_info_t *isectData) const {
		isect_info_t tuv[5];
		unsigned sides = 0U;
		for (unsigned i(0); i < NUM_PRISM_FACES; ++i) {
			bool result(false);
			int i0 = Mesh::prizm[i][0];
			int i1 = Mesh::prizm[i][1];
			int i2 = Mesh::prizm[i][2];
			int i3 = Mesh::prizm[i][3];
			if (i3 < 0)
				result = intersectTriangle(r,this->vertices[i0],this->vertices[i1],this->vertices[i2],(CoordType*)(&tuv[i]));
			else
				result = intersectQuad(r,this->vertices[i0],this->vertices[i1],this->vertices[i2],this->vertices[i3],(CoordType*)(&tuv[i]));
			if (result && (r.tmin <= tuv[i].t && tuv[i].t <= r.tmax)) {
				tuv[i].side = i;
			} else tuv[i].side = -1;
		}
		unsigned mn_i(0U);
		do{
			if (tuv[mn_i].side != -1) break;
		}while(mn_i++ < 5);
		unsigned mx_i = mn_i;
		for (unsigned i = mn_i+1;i<5;++i) {
			if (tuv[mn_i].t > tuv[i].t) { mn_i = i; }
			if (tuv[mx_i].t < tuv[i].t) { mx_i = i; }
		}
		el_isect_info_t* pis = static_cast<el_isect_info_t*>(isectData);
		pis->t = tuv[mn_i].t;
		pis->u = tuv[mn_i].u;
		pis->v = tuv[mn_i].v;
		pis->out.t = tuv[mx_i].t;
		pis->out.u = tuv[mn_i].u;
		pis->out.v = tuv[mx_i].v;
		return (mn_i < mx_i);
	}
	//void computeBounds(const CVec3f &planeNormal, float &dnear, float &dfar) const {}
private:
	Vec4<CoordType> normals[NUM_TETRA_FACES];
	bool isBaseParalled;
	//void computeBounds(const CVec3f &planeNormal, float &dnear, float &dfar) const {};
	void calculateNormals();
	int setCoordinates() {
		int lindex = static_cast<int>(this->index.eid);
		lindex = this->parentMeshPtr->GetElementCoordinates(lindex, this->idNodes, this->vertices[0].v);
		this->calculateNormals();
		return lindex;
	}
	int  setCoeffs();
	void calculateMinMaxRange();
};

template<typename TCoord>
BBox3D Prizm::BoundingBox(fvmath::Vec3<TCoord>* v)
{
	using namespace fvmath;
	typedef fvmath::Vec3<TCoord> vec3;
	BBox3D bb;
	bb.mn.x = fv_min(fv_min(fv_min(fv_min(fv_min(v[0].x,v[1].x),v[2].x),v[3].x),v[4].x),v[5].x);
	bb.mn.y = fv_min(fv_min(fv_min(fv_min(fv_min(v[0].y,v[1].y),v[2].y),v[3].y),v[4].y),v[5].y);
	bb.mn.z = fv_min(fv_min(fv_min(fv_min(fv_min(v[0].z,v[1].z),v[2].z),v[3].z),v[4].z),v[5].z);
	bb.mx.x = fv_max(fv_max(fv_max(fv_max(fv_max(v[0].x,v[1].x),v[2].x),v[3].x),v[4].x),v[5].x);
	bb.mx.y = fv_max(fv_max(fv_max(fv_max(fv_max(v[0].y,v[1].y),v[2].y),v[3].y),v[4].y),v[5].y);
	bb.mx.z = fv_max(fv_max(fv_max(fv_max(fv_max(v[0].z,v[1].z),v[2].z),v[3].z),v[4].z),v[5].z);
	bb.setInitialized(true);
	return bb;
}
template<typename T>
fvmath::CVec3<T> Prizm::Centrum(const fvmath::Vec3<T>* elCoords)
{
	CVec3<T> c;
	T inv_6 = 1. / NUM_PRISM_VERTICES;
	for (int i(0); i < NUM_PRISM_VERTICES; ++i) c += elCoords[i];
    c *= inv_6;
    return c;
}


} // end namespace FemViewer
#endif /* GEOMETRY_H_ */
