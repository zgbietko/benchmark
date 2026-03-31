#ifndef _RENDERPARAMS_H_
#define _RENDERPARAMS_H_

#include "defs.h"
#include "Enums.h"
#include "Color.h"
#include <cstring>

namespace FemViewer {

struct BaseParams {
	/* Colors */
	float wireframe_col[4]; // Color of edges of element when transparent mode is on
	float border_col[4]; // Color of edges of element when transparent mode is off
	float iso_color[4];		// Color of iso-lines
	float iso_values[MAX_ISO_VALUES][4]; // Table of iso values data
	int   num_breaks;	// Number of break-points (isovalues)
	//float light_pos[4]; // Light position
	BaseParams() {
		memset(this,0x0,sizeof(BaseParams));
	}
};


class RenderParams //: public BaseParams
{
public:

	enum renderMode { fullRMode, bboxRMode, fastRMode };
	Render_t   eRenderType;
	renderMode eRMode;
	//Render_t   eRType;
	bool       bSmoothNormals;    // Smooth normals, where applicable, and apply them to the primitive
	bool       bFacetFrame; // Render the boundaries of a facet
	bool       bDoubleSided;      // polygons are not culled but also drawn if seen from the back side
	ColorRGB   BkgColor;
	ColorRGB   EdgeColor;
	int        iPrimitiveOptimizerValue; // Determine the maximum length of a glBegin/glEnd sequence
                                       //  default value of 100 is an acceptable performance/memory
                                       //  tradeoff.
  
	int        iRMode_Fast_Option;
	bool	   bShowNumOfVertices;
	bool       bShowNumOfElements;
	BaseParams shaderParams;

  RenderParams()
	: //on(0)
	//, start_value(0.0f)
	//, delta(0.0f)
	//, num_breaks(0)
	//, edge_color0(1.f,.0f,1.0f)
	//, edge_color1(),
	eRMode                  (fullRMode),
      eRenderType				  (RASTERIZATION_GL),
	  bSmoothNormals		  (false),
      bFacetFrame			  (false),  // to ja
      bDoubleSided			  (false),
      BkgColor(),
      iPrimitiveOptimizerValue(100),
      iRMode_Fast_Option	  (1),
      bShowNumOfVertices	  (false),
      bShowNumOfElements	  (false)
    {
	  memset(&shaderParams,0x0,sizeof(shaderParams));
    }

};

} // edn namespace FemViewer

#endif /* _RENDERPARAMS_H_
*/

