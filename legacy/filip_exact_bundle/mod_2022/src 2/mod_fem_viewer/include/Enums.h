#ifndef _ENUMS_TYPE_H_
#define _ENUMS_TYPE_H_

#include "../../include/fv_config.h"


namespace FemViewer
{

	typedef enum {
		REFIN = -1,
		FREE  =  0,
		
		PRIZM =  5,
		BRICK =  6,
		TETRA =  7,
	} ElemType;

	typedef enum {
		F_TRIA = 3,
		F_QUAD = 4,
	} FaceType;

	typedef enum {
		NITM = -1,
		NODE =  0, 
		EDGE =  1,
		FACE =  2,
		ELEM =  3,
		FIELD = 4,
		FIELDN = 5,
	} ElemItem;

	typedef enum {
		NOT_REF = 0,
		REF_ISO = 1,
	} RefinementType;

	typedef enum {
		TENSOR   = 0,
		COMPLETE = 1,
	} BaseType;

	typedef enum {
		INTERNAL = 0,
		EXTERNAL,
	} ModuleType;

	typedef enum {
		LINEAR = 0,
		HIGH_ORDER,
	} ApproximationType;

	typedef enum {
		  ID_VERTEX = 0,
		  ID_ELEMENT,
		  ID_COLR_MAP,
		  ID_GRID,
		  ID_AXES,
		  ID_ALL
	 } GLListType;

	typedef enum {
		RASTERIZATION_GL = 0,
		RAYTRACE_GL_CL,
	} Render_t;

	typedef enum {
		Unknown 	= -1,
		MeshPrizm 	= 0,
		MeshHybrid 	= 1,
		MeshRemesh  = 2,
		FieldSTD 	= 0,
		FieldDG		= 1,
	} HandleType;

	/* Reference prismatic element */
	const double XlocPrizm[18] = {
				0.0, 0.0, -1.0,
				1.0, 0.0, -1.0,
				0.0, 1.0, -1.0,
				0.0, 0.0,  1.0,
				1.0, 0.0,  1.0,
				0.0, 1.0,  1.0
	};

	/* Reference tetrahedron element */
	const double XlocTetra[12] = {
				1.0, 0.0, 0.0,
				0.0, 1.0, 0.0,
		        0.0, 0.0, 1.0,
				0.0, 0.0, 0.0,
	};

	struct isect_info_t {
		float t;	  // t parameter for ray
		float u, v;   // u,v barycentric coordinates for ray/side intersection
		int   side;   // index of the side within the element
	};

    struct el_isect_info_t : isect_info_t {
		isect_info_t out;	// out info of intersection with element
	};


//	typedef enum MenuEvent
//	{
//			IDM_SEPARATOR = -1,
//		#ifndef _USE_FV_LIB
//			IDM_OPEN_MESH = 0,
//			IDM_OPEN_FIELD,
//			IDM_REFRESH,
//		#else
//			IDM_REFRESH = 0,
//		#endif
//			IDM_RELOAD,
//			IDM_RESET,
//			IDM_QUIT,
//
//			IDM_VPERSP,
//			IDM_VORTHO,
//			IDM_VTOP,
//			IDM_VBOTTOM,
//			IDM_VFRONT,
//			IDM_VBACK,
//			IDM_VLEFT,
//			IDM_VRIGHT,
//			IDM_VDEFAULT,
//			IDM_VFULL,
//			IDM_VBBOX,
//			IDM_VFAST,
//			IDM_VNEW,
//			IDM_VNEXT,
//			IDM_VPREV,
//			IDM_VDUMP_CURR,
//			IDM_VDUMP_ALL,
//
//			IDM_CAXES,
//			IDM_CGRID,
//			IDM_CBKG_COLOR,
//			IDM_CLIGHT,
//			IDM_CLEGEND,
//			IDM_CRESET,
//			IDM_CSAVE,
//			IDM_CLEG_EDIT,
//			IDM_CMOD_APR,
//
//			IDM_RSOL_SET,
//			IDM_RDRAW_WIRE,
//			IDM_RDRAW_FILL,
//			IDM_RDRAW_CONT,
//			IDM_RDRAW_FLOODED,
//			IDM_RDRAW_CUT,
//			IDM_RCUT_SETS,
//			IDM_RSCREEN_SAVE,
//
//			IDM_HELP,
//		} MenuEvent_t;

} // end namespace

#endif /* _ENUMS_TYPE_H_
*/
