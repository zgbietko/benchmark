#include "AMGSolverStructure.hpp"



AMGSolverStructure::AMGSolverStructure(Mat matrix, Vec rhs, int size)
{
	this->matrix = matrix;
	this->rhs = rhs;
	this->size = size;

	RSCFSplitter* rscfSplitter = new RSCFSplitter(matrix, 1.0);
}

void AMGSolverStructure::CreateAMGSolverLevels()
{
}

void AMGSolverStructure::RunVCycle()
{

}

int AMGSolverStructure::GetLevelsNumber()
{
	return 2;
}

void AMGSolverStructure::InitLevels()
{
	levelsNumber = GetLevelsNumber();

	amg_level_matrices = new Mat[levelsNumber];
	amg_level_rhs = new Vec[levelsNumber];
	amg_level_sizes = new int[levelsNumber];

	amg_level_matrices[0] = matrix;
	amg_level_rhs[0] = rhs;
	amg_level_sizes[0] = size;

	mf_log_info("%s %d","Numebr of AMG levels: ", levelsNumber);
}

void AMGSolverStructure::CreateNextLevel(int levelNumber)
{

}

