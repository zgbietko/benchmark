/*
 * Light.h
 *
 *  Created on: 14 sie 2014
 *      Author: dwg
 */

#ifndef LIGHT_H_
#define LIGHT_H_

#include "Color.h"
#include "MathHelper.h"

namespace FemViewer {

using namespace fvmath;

class Light {

public:
	Light();
    Light(const Light& rhs);
	Light& operator=(const Light& rhs);
	~Light(){}

	      ColorRGBA& Color()       { return _color; }
	const ColorRGBA& Color() const { return _color; }
		  CVec3f& AmbientIntensity()       { return _ambientIntensity; }
	const CVec3f& AmbientIntensity() const { return _ambientIntensity; }
		  CVec3f& DiffuseIntensity()       { return _diffuseIntensity; }
	const CVec3f& DiffuseIntensity() const { return _diffuseIntensity; }
		  CVec3f& Position()       { return _position; }
    const CVec3f& Position() const { return _position; }
private:

	ColorRGBA _color;
	CVec3f _ambientIntensity;
	CVec3f _diffuseIntensity;
	CVec3f _position;
};


} //

#endif /* LIGHT_H_ */
