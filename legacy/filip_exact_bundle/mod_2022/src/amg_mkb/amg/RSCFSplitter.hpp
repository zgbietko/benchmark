/*
 * RSCFSplitter.h
 *
 *  Created on: Jun 6, 2015
 *      Author: damian
 */

#ifndef RSCFSPLITTER_H_
#define RSCFSPLITTER_H_

#include "CFSplitter.hpp"
#include <petscmat.h>
#include <algorithm>

//Ruge Stuben CF Splitter
class RSCFSplitter : public CFSplitter
{
	public:
		RSCFSplitter(Mat mat, double strength_threshold);
		virtual ~RSCFSplitter();
		virtual void MakeCFSplitting();

	private:
		double strength_threshold;
		struct row_info* row_info_array;
		struct influenced_info* influenced_info_array;
		void ReplaceAfterInfluencedNumberChange(struct row_info* row_info);
		struct row_info* getHeapParent(int index);
		void replaceRowInfos(struct row_info* row_info1, struct row_info* row_info2);

};

enum Set
{
	DEFAULT = 0,
	CSET = 1,
	FSET = 2
};

struct row_info
{
	Set set;
	PetscInt local_row_number;

	row_info() : set(DEFAULT) {};
};

struct influenced_info
{
	int influenced_number;
	struct row_info* row_info;

	influenced_info() : row_info(NULL), influenced_number(0) {};
};

#endif /* RSCFSPLITTER_H_ */
