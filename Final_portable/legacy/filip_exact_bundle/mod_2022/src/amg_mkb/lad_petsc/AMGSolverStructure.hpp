
#ifndef AMGSOLVERSTRUCTURE_H
#define AMGSOLVERSTRUCTURE_H

#include <petscksp.h>
#include "uth_log.h"
#include "../amg/RSCFSplitter.hpp"

class AMGSolverStructure
{
	public:

		AMGSolverStructure(Mat matrix, Vec rhs, int size);
		void CreateAMGSolverLevels();
		void RunVCycle();

	private:

		Mat matrix;
		Vec rhs;
		int size;

		Mat* amg_level_matrices;
		Vec* amg_level_rhs;
		int* amg_level_sizes;
		int levelsNumber;

		int GetLevelsNumber();
		void InitLevels();
		void CreateNextLevel(int levelNumber);

};

#endif
