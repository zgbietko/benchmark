/*
 * Light.cpp
 *
 *  Created on: 14 sie 2014
 *      Author: dwg
 */

#include "Light.h"
namespace FemViewer {

Light::Light()
: _color(1.0f, 1.0f, 1.0f, 1.0f)
, _ambientIntensity(0.3f, 0.3f, 0.3f)
, _diffuseIntensity(0.7f, 0.7f, 0.7f)
, _position(-3.f, -3.f, -3.f)
{
}

Light::Light(const Light& rhs)
: _color(rhs._color)
, _ambientIntensity(rhs._ambientIntensity)
, _diffuseIntensity(rhs._diffuseIntensity)
, _position(rhs._position)
{
}

Light& Light::operator=(const Light& rhs)
{
	_color = rhs._color;
	_ambientIntensity = rhs._ambientIntensity;
	_diffuseIntensity = rhs._diffuseIntensity;
	_position = rhs._position;
	return *this;
}

}// end namespace



