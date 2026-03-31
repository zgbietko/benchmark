/*
 * Geometry.cpp
 *
 *  Created on: 30 sty 2014
 *      Author: dwg
 */

#include "Geometry.h"
#include "Log.h"
#include<omp.h>


namespace FemViewer {
using namespace fvmath;

const Mesh* mfvBaseObject::parentMeshPtr = 0;
const Field* mfvBaseObject::parentFieldPtr = 0;
uint64_t numRayTrianglesTests = 0;
uint64_t numRayTrianglesIsect = 0;

#define EPSILON 1e-8
template<typename TReal>
bool intersectTriangle(
		const Ray<TReal> &r,
		const Vec3<TReal>& v0, const Vec3<TReal>& v1, const Vec3<TReal>& v2,
		TReal tuv[])
{
	__sync_fetch_and_add(&numRayTrianglesTests, 1);
	CVec3f edge1 = v1 - v0;
	CVec3f edge2 = v2 - v0;
	CVec3f pvec = r.dir * edge2;
	TReal det = Dot(edge1, pvec);
	if (det > TReal(-EPSILON) && det < TReal(EPSILON)) return false;
	TReal invDet = TReal(1) / det;
	CVec3f tvec = r.orig - v0;
	tuv[1] = Dot(tvec, pvec) * invDet;
	if (tuv[1] < TReal(0) || tuv[1] > 1) return false;
	CVec3<TReal> qvec = tvec * edge1;
	tuv[2] = Dot(r.dir, qvec) * invDet;
	if (tuv[2] < TReal(0) || tuv[2] + tuv[1] > TReal(1)) return false;
	tuv[0] = Dot(edge2, qvec) * invDet;
	__sync_fetch_and_add(&numRayTrianglesIsect, 1);
	return true;
}

// Fuction instantation
/*template<>
bool intersectTriangle<CoordType>(
		const Ray<CoordType> &r,
		const fvmath::Vec3<CoordType>& v0, const fvmath::Vec3<CoordType>& v1, const fvmath::Vec3<CoordType>& v2,
		CoordType tuv[]
		);
*/


template<typename TReal>
bool intersectQuad(
		const Ray<TReal> &r,
		const Vec3<TReal>& v0, const Vec3<TReal>& v1, const Vec3<TReal>& v2, const Vec3<TReal>& v3,
		TReal tuv[])
{
	// Rejects rays that are parallel to Q, and rays that intersect the plane of
	// Q either on the left of the line V00V01 or on the right of the line V00V10.
	const TReal eps(EPSILON);
	const TReal One(1);
	const TReal Zero(0);
	//CVec3<TReal> ed01 = v10 - v00;
	CVec3<TReal> ed01 = v1 - v0;
	//CVec3<TReal> ed03 = v01 - v00;
	CVec3<TReal> ed03 = v3 - v0;
	CVec3<TReal> P   = r.dir * ed03;
	TReal det  = Dot(ed01, P);
	if (std::abs(det) < eps) return false;
	TReal inv_det = TReal(1) / det;
	CVec3<TReal> T = r.orig - v0;
	TReal alfa = Dot(T, P) * inv_det;
	if (alfa < Zero) return false;
	// if (alpha > real(1.0)) return false; // Uncomment if VR is used.
	CVec3<TReal> Q = T * ed01;
	TReal beta = Dot(r.dir, Q) * inv_det;
	if (beta < Zero) return false;
	// if (beta > real(1.0)) return false; // Uncomment if VR is used.
	if (alfa + beta > One) {

	    // Rejects rays that intersect the plane of Q either on the
	    // left of the line V11V10 or on the right of the line V11V01.

	    CVec3<TReal> ed23 = v3 - v2;
	    CVec3<TReal> ed21 = v1 - v2;
	    CVec3<TReal> P_prime = r.dir * ed21;
	    TReal det_prime = Dot(ed23, P_prime);
	    if (std::abs(det_prime) < eps) return false;
	    TReal inv_det_prime = One / det_prime;
	    CVec3f T_prime = r.orig - v2;
	    TReal alfa_prime = Dot(T_prime, P_prime) * inv_det_prime;
	    if (alfa_prime < Zero) return false;
	    CVec3f Q_prime = T_prime * ed23;
	    TReal beta_prime = Dot(r.dir, Q_prime) * inv_det_prime;
	    if (beta_prime < Zero) return false;
	  }

	  // Compute the ray parameter of the intersection point, and
	  // reject the ray if it does not hit Q.

	  tuv[0] = Dot(ed03, Q) * inv_det;
	  if (tuv[0] < Zero) return false;

	  // Compute the barycentric coordinates of the fourth vertex.
	  // These do not depend on the ray, and can be precomputed
	  // and stored with the quadrilateral.

	  TReal alfa_11, beta_11;
	  CVec3<TReal> ed02 = v2 - v0;
	  CVec3<TReal> n = ed01 * ed03;

	  if ((std::abs(n.x) >= std::abs(n.y))
	    && (std::abs(n.x) >= std::abs(n.z))) {

	    alfa_11 = ((ed02.y * ed03.z) - (ed02.z * ed03.y)) / n.x;
	    beta_11  = ((ed01.y * ed02.z) - (ed01.z * ed02.y)) / n.x;
	  }
	  else if ((std::abs(n.y) >= std::abs(n.x))
	    && (std::abs(n.y) >= std::abs(n.z))) {

	    alfa_11 = ((ed02.z * ed03.x) - (ed02.x * ed03.z)) / n.y;
	    beta_11  = ((ed01.z * ed02.x) - (ed01.x * ed02.z)) / n.y;
	  }
	  else {

	    alfa_11 = ((ed02.x * ed03.y) - (ed02.y * ed03.x)) / n.z;
	    beta_11  = ((ed01.x * ed02.y) - (ed01.y * ed02.x)) / n.z;
	  }

	  // Compute the bilinear coordinates of the intersection point.

	  if (std::abs(alfa_11 - One) < eps) {

	    // Q is a trapezium.
	    tuv[1] = alfa;
	    if (std::abs(beta_11 - One) < eps) tuv[2] = beta; // Q is a parallelogram.
	    else tuv[2] = beta / ((tuv[1] * (beta_11 - One)) + One); // Q is a trapezium.
	  }
	  else if (std::abs(beta_11 - One) < eps) {

	    // Q is a trapezium.
	    tuv[2] = beta;
	    tuv[1] = alfa / ((tuv[2] * (alfa_11 - One)) + One);
	  }
	  else {

	    TReal A = One - beta_11;
	    TReal B = (alfa * (beta_11 - One))
	      - (beta * (alfa_11 - One)) - One;
	    TReal C = alfa;
	    TReal D = (B * B) - (4.0f * A * C);
	    TReal Q = TReal(-0.5) * (B + ((B < One ? -One : One)
	      * std::sqrt(D)));
	    tuv[1] = Q / A;
	    if ((tuv[1] < Zero) || (tuv[1] > One)) tuv[1] = C / Q;
	    tuv[2] = beta / ((tuv[1] * (beta_11 - One)) + One);
	  }

	  return true;

}

/*template<>
bool intersectQuad<CoordType>(
  const Ray<CoordType> &r,
  const Vec3<CoordType>& v0, const Vec3<CoordType>& v1, const Vec3<CoordType>& v2, const Vec3<CoordType>& v3,
  float tuv[]
);*/


// triangle stuff

Triangle::Triangle(const id_t& id1,const id_t& id2,const id_t& id3) : isSingledSided(true)
{
	assert(parentMeshPtr!=NULL);
	for (int i(0);i<NUM_TRIANGLE_VERTICES;++i) {
		int node = static_cast<int>(this->index.fid);
		parentMeshPtr->GetNodeCoor<CoordType>(node,this->vertices[i].v);
	}
	setBounds();
	calculateNormals();
}

void Triangle::calculateNormals()
{
	CVec3<CoordType> v1(this->vertices[2] - this->vertices[0]);
	CVec3<CoordType> v2(this->vertices[1] - this->vertices[0]);
	CVec3<CoordType> * normalPtr = reinterpret_cast<CVec3<CoordType> *>(normal.v);
	fvmath::Cross(&v1,&v2,normalPtr);
	fvmath::Normalize(*normalPtr);
	// Now calculate the D parameter for plan
	normal.w = -(normal.x*vertices[0].x + normal.y*vertices[0].y + normal.z*vertices[0].z);
}

// Tetrahedron stuff
Tetra::Tetra(const id_t& id)
: mfvObject<CoordType,id_t,NUM_TETRA_VERTICES>(id)
  {
	assert(parentMeshPtr!=NULL);
	assert(parentFieldPtr!=NULL);
	setCoordinates();
	setBounds();
	calculateNormals();
}

#define EDGE(name,begin,end)	\
		CVec3<CoordType> name(this->vertices[end] - this->vertices[begin])

void Tetra::calculateNormals()
{
	// The first face (base)
	EDGE(ed1,0,1);
	EDGE(ed2,0,2);
	CVec3<CoordType> * normalPtr = reinterpret_cast<CVec3<CoordType> *>(normals[0].v);
	fvmath::Cross(&ed1,&ed2,normalPtr);
	fvmath::Normalize(*normalPtr);
	// The second face (front)
	EDGE(ed3,0,3);
	normalPtr = reinterpret_cast<CVec3<CoordType> *>(normals[1].v);
	fvmath::Cross(&ed3,&ed1,normalPtr);
	fvmath::Normalize(*normalPtr);
	// The third face (left)
	normalPtr = reinterpret_cast<CVec3<CoordType> *>(normals[2].v);
	fvmath::Cross(&ed3,&ed1,normalPtr);
	fvmath::Normalize(*normalPtr);
	// The fourth face (behind)
	EDGE(ed4,1,2);
	ed2 = ed3 - ed1;
	normalPtr = reinterpret_cast<CVec3<CoordType> *>(normals[3].v);
	fvmath::Cross(&ed3,&ed4,normalPtr);
	fvmath::Normalize(*normalPtr);
}


// TODO add converion to
template<typename T>
int Prizm::teselateReference(const int Pdeg[], CVec3<T> RefPoints[])
{
	assert(Pdeg[0] > 0 && Pdeg[1] > 0 && Pdeg[2] > 0);
	int num = 0;
	const T dx = 1.0 / Pdeg[0];
	const T dy = 1.0 / Pdeg[1];
	const T dz = 1.0 / Pdeg[2];
	T ox(0);
	for (int i = 0; i <= Pdeg[0]; ++i) {
		T oy(0);
		for (int j = 0; j <= Pdeg[1] - i; ++j) {
			T oz(-1);
			for (int k = 0; k <= Pdeg[2]; ++k) {
				RefPoints[num++] = CVec3<T>(ox,oy,oz);
				oz += dz;
			}
			oy += dy;
		}
		ox += dx;
	}
	return num;
}

template<>
int Prizm::teselateReference(const int Pdeg[],CVec3<CoordType> RefPoints[]);

template<typename T>
int Prizm::teselateWorld(const int base,const int Pdeg[3],const CVec3<T> ElCoords[],std::vector<CVec3<T> >* TesPoints)
{
	static T GeoPhi[6];
	int size1 = Prizm::getNumberOfShapeFunctions(Pdeg,base);
	CVec3<T>* RefPoints = new CVec3<T> [size1];
	int size2 = Prizm::teselateReference(Pdeg,RefPoints);
	assert(size1 == size2);

	TesPoints->reserve(size1);
	for (int i = 0;i < size1;++i) {
		const CVec3<T>* Eta = &RefPoints[i];
		GeoPhi[0]=(1.0-Eta[0]-Eta[1])*(1.0-Eta[2])/2.0;
		GeoPhi[1]=Eta[0]*(1.0-Eta[2])/2.0;
		GeoPhi[2]=Eta[1]*(1.0-Eta[2])/2.0;
		GeoPhi[3]=(1.0-Eta[0]-Eta[1])*(1.0+Eta[2])/2.0;
		GeoPhi[4]=Eta[0]*(1.0+Eta[2])/2.0;
		GeoPhi[5]=Eta[1]*(1.0+Eta[2])/2.0;

		for (int j=0;j < 6; ++j) {
			TesPoints[i].x += GeoPhi[j]*ElCoords[j].x;
			TesPoints[i].y += GeoPhi[j]*ElCoords[j].y;
			TesPoints[i].z += GeoPhi[j]*ElCoords[j].z;
		}
	}
	// Now loop over point
	delete [] RefPoints;
	return size1;
}
int Prizm::getNumberOfShapeFunctions(const int Order[],int base)
{
	int approxType = parentFieldPtr->GetApproximationType();
	int numShapFun = 6;
	if (approxType  == FieldDG) {
		switch (base) {
		case BaseType::TENSOR:
			numShapFun = (Order[2]+1)*(Order[1]+1)*(Order[0]+2) / 2;
			break;
		case BaseType::COMPLETE:
			numShapFun = (Order[0]+1)*(Order[0]+2)*(Order[0]+3) / 6;
			break;
		}
	}

	return numShapFun;
}

Prizm::Prizm(const ElemId<id_t>& id)
: mfvObject<CoordType,id_t,NUM_PRISM_VERTICES>(id)
, isBaseParalled(false)
{
	//mfp_log_debug("ctr%d",id.eid);
	assert(parentMeshPtr!=NULL);
	assert(parentFieldPtr!=NULL);
	setCoordinates();
	setBounds();
	calculateNormals();
	calculateMinMaxRange();
}

void Prizm::calculateNormals()
{
	// First make bases
	// The first base (lower)
	EDGE(ed1,0,1);
	EDGE(ed2,0,2);
	CVec3<CoordType> * normalPtr1 = reinterpret_cast<CVec3<CoordType> *>(normals[0].v);
	fvmath::Cross(&ed1,&ed2,normalPtr1);
	fvmath::Normalize(*normalPtr1);
	// The second base (upper)
	EDGE(ed3,3,4);
	EDGE(ed4,4,5);
	CVec3<CoordType> * normalPtr2 = reinterpret_cast<CVec3<CoordType> *>(normals[1].v);
	fvmath::Cross(&ed4,&ed3,normalPtr2);
	fvmath::Normalize(*normalPtr2);
	if (*normalPtr1 == *normalPtr2) {
		this->isBaseParalled = true;
	}
	// The front face (3rd)
	EDGE(ed5,1,4);
	normalPtr1 = reinterpret_cast<CVec3<CoordType> *>(normals[2].v);
	fvmath::Cross(&ed5,&ed1,normalPtr1);
	fvmath::Normalize(*normalPtr1);
	// The behind face (4th)
	normalPtr2 = reinterpret_cast<CVec3<CoordType> *>(normals[3].v);
	fvmath::Cross(&ed5,&ed4,normalPtr2);
	fvmath::Normalize(*normalPtr2);
	// The left face (5th)
	EDGE(ed6,0,3);
	normalPtr1 = reinterpret_cast<CVec3<CoordType> *>(normals[4].v);
	fvmath::Cross(&ed2,&ed6,normalPtr1);
	fvmath::Normalize(*normalPtr1);
	// Check if bases are parallel

}



void Prizm::calculateMinMaxRange()
{
	static const double NDIV = 10;
	static const double dx = 1.0/NDIV;
	static const double dy = dx;
	static const double dz = 2.0/NDIV;
    static const int control = 1;
   	int Pdeg[3];
	int base = parentFieldPtr->GetDegreeVector(this->index.eid,Pdeg);
	double coords[3*numVertices];
	double mnval = 10e10, mxval = -10e10;
	double sol;
	(void)this->convert(this->vertices[0].v, coords);
	// Loops over 10 point in each directions
	// Start at the first vertex in prism
	CVec3d pt;
	pt.z = -1.0;
	for (pt.z = -1.0; pt.z <= 1.0; pt.z += dz) {
		for (pt.y = 0.0; pt.y <= 1.0; pt.y+= dy) {
			for (pt.x = 0.0; pt.x <= 1.0 - pt.y; pt.x += dx) {
				// vertexy trzeba zamnienic na dopuible albo dac templata na floata
				parentFieldPtr->CalculateSolution(control,pdeg,base,coords, pt.v, solCoeffs.data(), &sol, NULL, NULL);
				//printf("%d sol = %lf pt = %f %f %f\n",omp_get_thread_num(),sol,pt.x,pt.y,pt.z);
				mnval = mnval > sol ? sol : mnval;
				mxval = mxval < sol ? sol : mxval;
			}
		}
	}
	// Store in min max value
	assert(mxval >= mnval);
	minValue = mnval;
	maxValue = mxval;
}

}// end namespace



