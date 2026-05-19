#ifndef CFSPLITTER_H
#define SMOOTHER_H

#include <petscmat.h>

class CFSplitter
{
	public:
		CFSplitter(Mat mat);
		virtual ~CFSplitter();
		virtual void MakeCFSplitting() = 0;

	protected:
		Mat mat;
};

#endif
