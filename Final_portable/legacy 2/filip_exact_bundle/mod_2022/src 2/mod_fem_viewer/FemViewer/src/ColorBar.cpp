/*
 * ColorBar.cpp
 *
 *  Created on: 17-09-2013
 *      Author: Paweł Macioł
 */

#include "Enums.h"
#include "ColorBar.h"
#include "Legend.h"
#include "fv_txt_utls.h"
#include "Log.h"
#include "VtxAccumulator.h"

namespace FemViewer {

GLuint ColorBar::idList;
bool ColorBar::isInit = false;

bool ColorBar::Init (void)
{
	//mfp_log_debug("ColorBar Init\n");
	if (! glIsList(idList)) {
		mfp_debug("Before generating glList\n");
		idList = glGenLists(1);
		FV_CHECK_ERROR_GL();
	}
	isInit = glIsList(idList) ? true : false;
	FV_CHECK_ERROR_GL();
	return isInit;
}

void ColorBar::Destroy (void)
{
	//logd("Destroy\n");
	//mfp_debug("ColorBar: Destroy\n");
	if (glIsList(idList)) glDeleteLists(idList,1);
	FV_CHECK_ERROR_GL();
	idList = 0;
	isInit = false;
}

void ColorBar::FillFlat (
		const ColorBar& colbar, float posX,
		float posY, float width, float height, int direction
		)
{
	int lGr = static_cast<int>(colbar.parent.GetColors().size()) - 1;
	lGr /= 2;
	lGr++;
	float delta;
	if (direction == left) {
		delta = width / (lGr - 1);
		float x = posX;
		glBegin(GL_QUAD_STRIP);
		for(int i(0); i<lGr; i++) {
			glColor3fv(colbar.parent.GetColors()[2*i + 1].rgb.v);
			glVertex2f(x,posY - height); // upper
			glVertex2f(x,posY); // lower
			x += delta;
		}
		glEnd();
	}
	else if (direction == up) {
		delta = height / (lGr - 1);
		float y = posY;
		glBegin(GL_QUAD_STRIP);
		for(int i(0); i<lGr; i++) {
			glColor3fv(colbar.parent.GetColors()[2*i + 1].rgb.v);
			glVertex2f(        posX,y); // upper
			glVertex2f(posX + width,y); // lower
			y += delta;
		}
		glEnd();
	}
}

void ColorBar::FillGradient (
		const ColorBar& clbar,
		float posX,
		float posY,
		float width,
		float height,
		const int direction)
{
	//mfp_log_debug("ColorBar FillGradient");
	const int lRanges = 4;
	int lGr = static_cast<int>(clbar.parent.GetColors().size());
	const float fmin = (float)clbar.parent.GetMin();
	const float fmax = (float)clbar.parent.GetMax();
	const float fstart = (float)clbar.parent.GetStart();
	const float fend = (float)clbar.parent.GetEnd();
	//printf("Min: %f\tMax: %f\n",fmin,fmax);
	//printf("Start : %f\tEnd: %f\n",fstart,fend);
	float deltaVelue = (fend - fstart) / lRanges;
	char text[12];
	sprintf(text,"%5lf", fstart);
	//std::cout<<"Width of first: " << getTextWidthOnScreen(text,GLUT_BITMAP_8_BY_13) << std::endl;
	bool isPrefix = fvmath::Less( fmin, fstart);
	bool isSuffix = fvmath::Less( fend, fmax);
	// Check if we have to draw min/max text under colorbar
	if (isPrefix || isSuffix) {
		posY += 0.05f;
	}
	//std::cout<< " isSuffix dla " << float(clbar.parent.GetEnd()) << " ma " << isSuffix <<std::endl;
	float delta;
	float ofForward  = 0.f;
	float ofBackward = 0.f;
	int mlt = 0;
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT, GL_FILL);
	glLineWidth(1.5f);

	if (direction == left)
	{
		delta = width / (lGr - 1);
		float x;
		// Prefix rectangle
		int i = 0;
		if (isPrefix ) {
			ofForward = delta;
			glBegin(GL_QUADS);
				x = posX;
				glColor3fv(clbar.parent.GetColors()[0].rgb.v);
				glVertex2f(x,posY - height);
				glVertex2f(x,posY);
				x += delta;
				glVertex2f(x,posY);
				glVertex2f(x,posY - height);
			glEnd();
			//i++;
			mlt++;
			glColor3f(0.7f,0.7f,0.7f);
			glBegin(GL_LINES);
			glVertex2f(posX,posY - height);
			glVertex2f(posX,posY - height  - 0.01f);
			glEnd();

			sprintf(text,"%5f",fmin);
			{
				float tw = getTextWidthOnScreen(text,GLUT_BITMAP_8_BY_13);
				float xts = posX - 0.5f*tw;
				float xte = xts + tw;
				float yts = posY - height - 0.03f;
				float yte = yts + 0.02f;
				drawText(text, xts, yts, xte, yte, 12, GLUT_BITMAP_8_BY_13, true, false);
			}
		}

		// Suffix rectangle
		int last(lGr);
		if (isSuffix) {
			ofBackward = delta;
			last--;
			glBegin(GL_QUADS);
				x = posX + (last-1)*delta;
				glColor3fv(clbar.parent.GetColors()[lGr - 1].rgb.v);
				glVertex2f(x,posY - height);
				glVertex2f(x,posY);
				x += delta;
				glVertex2f(x,posY);
				glVertex2f(x,posY - height);
			glEnd();
			// Draw line
			glColor3f(0.7f,0.7f,0.7f);
			glBegin(GL_LINES);
			glVertex2f(posX + width,posY - height);
			glVertex2f(posX + width,posY - height - 0.01f);
			glEnd();
			// Draw text
			sprintf(text,"%5f",fmax);
			{
				float tw = getTextWidthOnScreen(text,GLUT_BITMAP_8_BY_13);
				float xts = posX + width - 0.5f*tw;
				float xte = xts + tw;
				float yts = posY - height - 0.03f;
				float yte = yts + 0.02f;
				drawText(text, xts, yts, xte, yte, 12, GLUT_BITMAP_8_BY_13, true, false);
			}
			mlt++;
		}

		x = posX;
		glBegin(GL_QUAD_STRIP);
		for(; i<last; i++) {
			//std::cout<< " x = " << x << std::endl;
			glColor3fv(clbar.parent.GetColors()[i].rgb.v);
			glVertex2f(x,posY - height); // upper
			glVertex2f(x,posY); // lower
			x += delta;
		}
		glEnd();

		glColor3f(0.7f,0.7f,0.7f);
		// Draw colorbar rectangle edges
		glDepthFunc(GL_LEQUAL);
		glBegin(GL_LINE_LOOP);
			glVertex2f(posX,posY);
			glVertex2f(posX + width,posY);
			glVertex2f(posX + width,posY - height);
			glVertex2f(posX,posY - height);
		glEnd();

		const float dw = (width - mlt*delta)/ lRanges;
		glBegin(GL_LINES);
		float xl = posX + ofForward;
		float yl = posY;
		for(int i(0); i <= lRanges + 1; i++)
		{
			glVertex2f(xl,yl);
			glVertex2f(xl,yl + 0.01f);
			xl += dw;
		}
		glEnd();
		// Get the width of first text field

		float xts = posX + ofForward;
		float xte = 0.09f - ofBackward;
		float yts = posY + 0.01f;
		float yte = yts  + 0.03f;
		if (isPrefix) {

		}

		if (isSuffix) {

		}


		float val = static_cast<float>(clbar.parent.GetStart());
		char text[12];
		for(int i(0); i <= lRanges + 1; i++)
		{
			sprintf(text,"%.5f",val + i*deltaVelue);
			float tw = getTextWidthOnScreen(text,GLUT_BITMAP_8_BY_13);
			drawText(text, xts - 0.5f*tw, yts, xts + tw, yte, 12, GLUT_BITMAP_8_BY_13, true, false);
			xts += dw;
		}



	}
	else if (direction == up) {
		delta = height / (lGr - 1);
		float y = posY;
		glBegin(GL_QUAD_STRIP);
		for(int i(0); i<lGr; i++) {
			glColor3fv(clbar.parent.GetColors()[i].rgb.v);
			glVertex2f(        posX,y); // upper
			glVertex2f(posX + width,y); // lower
			y += delta;
		}
		glEnd();
	}


	//glPopAttrib();
}

HorizontalColorBar::HorizontalColorBar (const Legend& parent , float x, float y, float wcoef, float hcoef)
: ColorBar(parent)
, xx(x)
, yy(y)
, wCoef(wcoef)
, hCoef(hcoef)
{
	//logd("HorizColorBar: Ctr\n");
	//mfp_log_debug("HorizontalColorBar ctr");
}

HorizontalColorBar::HorizontalColorBar(const HorizontalColorBar& rhs)
: ColorBar(rhs.parent)
, xx(rhs.xx)
, yy(rhs.yy)
, wCoef(rhs.wCoef)
, hCoef(rhs.hCoef)
{
	//mfp_log_debug("HorizontalColorBar copy ctr");
}

HorizontalColorBar::~HorizontalColorBar (void)
{
	//mfp_log_debug("HorizontalColorBar dtr\n");
}

HorizontalColorBar& HorizontalColorBar::operator=(const HorizontalColorBar& rhs)
{
	xx = rhs.xx;
	yy = rhs.yy;
	wCoef = rhs.wCoef;
	hCoef= rhs.hCoef;
	return *this;
}

void HorizontalColorBar::Create (void)
{
	static int i;
	//mfp_debug("HorizontalColorBar create \n");
	idList = g_GLList+ID_COLR_MAP;

	glNewList(idList, GL_COMPILE);
	FV_CHECK_ERROR_GL();
	if (parent.GetLegendType() == SOLID)
		FillFlat(*this,xx,yy,wCoef,hCoef,int(left));
	else
		FillGradient(*this,xx,yy,wCoef,hCoef,left);
	glEndList();
	FV_CHECK_ERROR_GL();
	//mfp_debug("After Horizontal %d:\n",i++);

}


}// end namespace FemViewer


