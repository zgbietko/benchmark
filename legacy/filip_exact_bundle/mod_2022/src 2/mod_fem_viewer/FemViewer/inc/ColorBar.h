/*
 * ColorBar.h
 *
 *  Created on: 17-09-2013
 *      Author: Paweł Macioł
 */

#ifndef COLORBAR_H_
#define COLORBAR_H_

#include "uth_log.h"
#include "fv_inc.h"
#include "fv_txt_utls.h"
#include "Log.h"
#include "LegendValue.h"
#include <vector>
#include <cassert>
#include <iostream>
#include <GL/gl.h>

namespace FemViewer {

class Legend;

enum { left, up, };

class ColorBar {
  protected:
	static GLuint idList;
	static bool   isInit;
	static bool Init (void);
	static void Destroy (void);
	static void FillFlat (const ColorBar& rhs,
						   float posX,
						   float posY,
						   float width,
						   float height,
						   int direction);
	static void FillGradient (const ColorBar& clbar,
							   float posX,
							   float posY,
							   float width,
							   float height,
							   const int direction = (int)left);

  protected:
	const Legend & parent;

  public:
	// Ctr & Dsr
	explicit ColorBar (const Legend& parent_) : parent(parent_) {
		//mfp_log_debug("ColrBar ctr const Legend&");
		//if (!isInit) Init();
	}

	explicit ColorBar (const ColorBar& rhs) : parent(rhs.parent) {
		//mfp_log_debug("ColorBar ctr copy)");
		//if (!isInit) Init();
	}

	virtual ~ColorBar (void) {
		//mfp_log_debug("ColorBar dtr");
		Destroy();
	}
	virtual void Draw (void) {
		glCallList(idList);
	}
	virtual void Reset (void) { Destroy(); Init(); }
	//virtual void Update (void) {}
	virtual void Create (void) { ; };

};

class HorizontalColorBar : public ColorBar {

private:
	// Start position of color bar
	// (lower-left corner
	float xx, yy;
	// Width and height coefficients of current screen size
	float wCoef, hCoef;

public:
	// Ctr & Dsr
	explicit HorizontalColorBar (const Legend& parent,float x=0.05f, float y=0.05f, float wcoef=0.9, float hcoef=0.03f);
	HorizontalColorBar (const HorizontalColorBar& rhs);
	HorizontalColorBar& operator= (const HorizontalColorBar& rhs);
	virtual ~HorizontalColorBar (void);
	void Create (void);
};

}


#endif /* COLORBAR_H_ */
