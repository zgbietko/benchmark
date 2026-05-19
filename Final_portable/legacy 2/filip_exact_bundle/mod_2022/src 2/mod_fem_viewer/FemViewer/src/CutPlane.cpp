#include "CutPlane.h"
#include "BBox3D.h"
//#include "MathHelper.h"
#include "elem_tables.h"
//#include "../../include/pomiar_czasu.c"


namespace FemViewer {

	int CutPlane::CheckElementLocation(const double x[], std::vector<FemViewer::GraphElement2<double> >& vels, int size) 
	{
		static /*const*/ double tetra[16] = { x[0],x[1],  x[2],-1, x[3], x[4], x[5],-1, x[6], x[7], x[8],-1,x[9],x[10],x[11],-1};
		static /*const*/ double prizm[24] = { x[0], x[1], x[2],-1, x[3], x[4], x[5],-1, x[6], x[7], x[8],-1,
											  x[9],x[10],x[11],-1,x[12],x[13],x[14],-1,x[15],x[16],x[17],-1 };
		static std::vector<FemViewer::GraphElement2<double> > _vels;
		static GraphElement2<double> el;
		if (!x) return (-1);

		int result(0);
		const double* table;
		_vels.clear();
	
		int nel(-1);
		if (size == 4)
		{
			nel = DoSliceTerahedron(this,NULL,tetra,_vels,NULL);
		} 
		else
		{
			nel = DoSlicePrizm(this,NULL,prizm,_vels,NULL);
		}

		return nel;
			
	}	

	int CutPlane::CheckOrientation(const double v[])
	{
		fvmath::CVec3d nrl(p.n);
		fvmath::CVec3d pos(v);
		fvmath::Normalize(pos);
		
		// calculate dot product
		double dot = fvmath::Dot(nrl,pos);

		if(fvmath::Compare(dot,0.0)) return(0); // perpendicular

		if(dot < 0.0f) return(-1); // the angle is > 90

		return(1); // the angle < 90

	}

	int CutPlane::InversPlane(const double v[])
	{
		fvmath::CVec3d nrl(p.n);
		fvmath::CVec3d pos(v);
		fvmath::Normalize(pos);

		// calculate dot product
		double dot = fvmath::Dot(nrl,pos);

		if(fvmath::Compare(dot,1.0)) 
			return(0); // do nothing - the same oritntation

		if(fvmath::Compare(dot,-1.0)) 
		{
			// Invers normal vector
			p.a = -p.a; p.b = -p.b; p.c = -p.c;
			return(1);
		}

		// Error
		return(-1);

	}

	// Returns:
	// -1 - box is inside
	//  0 - plane intersect box
	//  1 - box is outside the plane
	int CutPlane::IntersectBBox3D(const BBox3D& bbox) const
	{
		using namespace fvmath;
		CVec3d c = (bbox.mx + bbox.mn) * 0.5f;
		CVec3d h = (bbox.mx - bbox.mn) * 0.5f;
		double nx = abs(p.n[0]);
		double ny = abs(p.n[1]);
		double nz = abs(p.n[2]);
		double e  = h.x*nx+ h.y*ny + h.z*nz;
		double s  = c.x*p.a + c.y*p.b + c.z*p.c + p.d;
		if (s - e > 0.0) return 1;
		if (s + e < 0.0) return -1;
		return 0;
/*		unsigned short mask  = 0x00;
		unsigned short i;
		int sb, se;
		const Vec3d * rb, *re;

#define mn(a)	bbox.##a##min()
#define mx(a)	bbox.##a##max()
		
		const CVec3d bboxCorrners[] = { 
			CVec3d( mn(X), mn(Y), mn(Z) ),
			CVec3d( mx(X), mn(Y), mn(Z) ),
			CVec3d( mx(X), mx(Y), mn(Z) ),
			CVec3d( mn(X), mx(Y), mn(Z) ),
			CVec3d( mn(X), mn(Y), mx(Z) ),
			CVec3d( mx(X), mn(Y), mx(Z) ),
			CVec3d( mx(X), mx(Y), mx(Z) ),
			CVec3d( mn(X), mx(Y), mx(Z) ) 
		};

#undef mn
#undef mx

		struct Edge { int b, e; };

		const Edge bboxEdges[] = {
			{0,1},{1,2},{2,3},{3,1},
			{4,5},{5,6},{6,7},{7,4},
			{0,4},{1,5},{2,6},{3,7} 
		};

		//inicjuj_czas();

		// Parallel part
		for(i=0;i<12;i++)
		{
			rb = &bboxCorrners[ bboxEdges[i].b ];
			re = &bboxCorrners[ bboxEdges[i].e ];

			// Check if endings are at the same side of the cut plane
			sb = Check(p.n, rb->v3);
			se = Check(p.n, re->v3);

			if(sb * se <= 0.0) 
			{
				mask <<= i;
				state |= mask;
			}
		}		

		//drukuj_czas();
*/
		//return (state > 0) ? false : true;


	}

}// end namespace
