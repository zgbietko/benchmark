#ifndef _CUT_PLANE_H_
#define _CUT_PLANE_H_

#include "Plane.h"
#include "Vec3D.h"
#include "Point3D.h"
#include "GraphicElem.hpp"
#include "MathHelper.h"
#include "uth_log.h"
#include <vector>
//template <typename T> 
//union TParams {
//	T v[4];
//	struct {T nx, ny, nz, d; };
//};
//
//typedef TParams<float> CutParamsf;
//typedef TParams<double> CutParamsd;



namespace FemViewer {

	// Forward class declaration
	class BBox3D;

	class CutPlane : public Plane {
		bool _isPlaneChanged;
	public:
		CutPlane() : Plane(), _isPlaneChanged(false) {
			//mfp_debug("CutPlane ctr\n");
		}
		
		CutPlane(double nx,double ny, double nz, double d) {
			Set(nx,ny,nz,d);
		}

		CutPlane(double p[]) : Plane()
		{ Set(p[0],p[1],p[2],p[3]); }
		
		~CutPlane(){}
		bool IsPlaneChanged() const { return _isPlaneChanged; }
		void Set(double a, double b, double c, double d = 0.0)
		{
			fvmath::CVec3d v(a,b,c);
			Normalize(v);
			p.a = v.x; p.b = v.y; p.c = v.z;
			p.d = d;
		}

		void Set(double a, double b, double c, Point3D<double> pt)
		{
			Set(a,b,c);
			p.d = -(p.a*pt.x + p.b*pt.y + p.c*pt.z);
		}

		Vec3D GetNormalVector()
		{
			Vec3D tmp = ToVec3D(p.a,p.b,p.c);
			return tmp;
		}

		bool operator () (const Plane& rhPl)
		{
			bool res = false;
			if(p.n[0] != rhPl.GetParams()[0] || 
			   p.n[1] != rhPl.GetParams()[1] ||
			   p.n[2] != rhPl.GetParams()[2] ||
			   p.n[3] != rhPl.GetParams()[3] ) res = true;

			if(res) { 
				p.a = GetParams()[0];
				p.b = GetParams()[1];
				p.c = GetParams()[2];
				p.d = GetParams()[3];
			}

			return res;
		}


		// -3 all in opozite side
		// -2 to 2  plane cuts the triangle
		//  3 all in the same saide
		/*int CheckWithTraingle(const double triang[], int type=3) const
		{
			int ret(0);
			for (int i(0),j(0);i<9;i+=3)
			{
				ret += CheckLocation(triang[i],triang[i+1],triang[i+2]);
			}
			return( ret);
		}*/
		
		// Check if quad is intersected by plane
		/*inline int CheckWithQuad(const double quad[]) const
		{
			int ret = CheckWithTraingle(quad);
			ret += CheckLocation(quad[9],quad[10],quad[11]);
			return ret;
		}*/

		/*inline bool CheckWithFigure(const double fig[], int& type) const
		{
			int ret = (type == 3) ? CheckWithTraingle(fig) :  CheckWithQuad(fig) ;
			return (type == 3) ? (type=ret, !(ret == 3 || ret == -3)) : (type=ret, !(ret == 4 || ret == -4));
		}*/


		inline int IntersectWithLine(double x2[],double x1[],double& u) const {
			return Plane::IntersectWithLine(this->p.n,x2,x1,u);
		}


		int CheckElementLocation(const double x[], std::vector<GraphElement2<double> >& vels, int size);



		int CheckOrientation(const double v[]);

		int InversPlane(const double v[]);

		int IntersectBBox3D(const BBox3D& bbox) const;

	private:

		inline Vec3D ToVec3D(const double& a_, 
							 const double& b_, 
							 const double& c_) const;

		CutPlane(const CutPlane&); // Not implemented
		CutPlane& operator=(const CutPlane&); // Not implemented
	};



	inline Vec3D CutPlane::ToVec3D(const double& a, 
							       const double& b, 
							       const double& c) const
	{
		float _a = static_cast<float>(a);
		float _b = static_cast<float>(b);
		float _c = static_cast<float>(c);
		return Vec3D(_a, _b, _c);
	}




	
} // end namespace
#endif /* _CUT_PLANE_H_
*/
