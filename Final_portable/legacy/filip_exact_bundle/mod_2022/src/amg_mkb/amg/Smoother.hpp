#ifndef SMOOTHER_H
#define SMOOTHER_H

#include <petscvec.h>
#include <petscmat.h>

class Smoother
{
	public:
		virtual ~Smoother() {}
		virtual void Smooth(Vec x) = 0;
		virtual void PreSmoothing(Mat matrix, Vec b, Vec x) = 0;

};


#endif
