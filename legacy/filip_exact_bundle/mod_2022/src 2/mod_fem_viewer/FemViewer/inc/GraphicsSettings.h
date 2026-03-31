#ifndef _GRAPHICS_SETTINGS_H_
#define _GRAPHICS_SETTINGS_H_

#include "Enums.h"
#include "Vec3D.h"
#include "Color.h"
#include "Light.h"
#include "CubeGraphicSettings.h"

namespace FemViewer {

using namespace std;
class string;

class GraphicsSettings
{
public:

	  enum eMode 
	  { 
		  simpleMode_none,
		  simpleMode_bounding_box,
		  simpleMode_fast
	  };

	  GraphicsSettings();
	  ~GraphicsSettings();

	  void Defaults();
	  int  LoadFromFile(const std::string& sfileName, std::string& sError);
	  void SaveToFile(const std::string& pFilename, std::string& pError);

	  Render_t	eRenderType;
	  ColorRGB BkgColor;				 
	  int	iBackgroundMode;
	  bool  bIsGridOn;
	  bool  bIsAxesOn;
	  bool  bIsLegendOn;
	  bool 	bEdgeOn;
	  bool	bShadingOn;
	  bool  bIsovalueLineOn;
	  bool  bSliceModel;
	  bool  bLineCoolored;
	  bool  bIsSmothOn;
	  bool  bShowNumVertices;
	  bool  bShowNumElems;
	  float fAmbientLight;
	  int   iLightModel;
	  int   iLightPositionLocal;
	  Vec3D vLightPos;
	  eMode eSmpMode;
	  int	iSimplicationMode_param;
	  bool	bIsOrthoOn;
	  bool  bIsGridDraw;
	  bool  bIsOctreeDraw;
	  bool  bIsRayTracing;
	  Light DirectionalLight;
protected:
	// not implemented
	//GraphicsSettings(const GraphicsSettings&);
	//GraphicsSettings& operator=(const GraphicsSettings&);


};
} // end namespace FFemViewer


#endif /* _SETTINGS_H_
*/
