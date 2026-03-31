#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include <petscksp.h>
#include "../include/lah_petsc.h"
#include "../include/lah_block.h"
#include "../amg/JacobiMethod.hpp"
#include "petscsys.h"
#include "AMGSolverStructure.hpp"

Mat A;
Vec b;

AMGSolverStructure* s;

void fill_crs_matrix(itt_matrices *matrix)
{
	//itt_matrices *matrix = &itv_matrices[matrix_id];
	//printf("TOTAL COUNT %d\n", count_entries(matrix));
	int block_index;
	for(block_index = 1; block_index <= matrix->Nrblocks; block_index++)
	{
		itt_blocks* current_block = matrix->Block[block_index];


		if(current_block->Lngb != NULL)
		{
			int block_row_number, block_column_number;
			block_row_number = current_block->Posg;
			int i,j;
			//diagonal block
			for(i = 0; i<current_block->Ndof; i++)
			{
				block_column_number = current_block->Posg;
				for(j = 0; j<current_block->Ndof; j++)
				{
					PetscErrorCode ierr;
					ierr = MatSetValues(A,1,&block_row_number,1,&block_column_number,
							&(current_block->Dia[i + current_block->Ndof*j]),INSERT_VALUES);CHKERRABORT(PETSC_COMM_WORLD, ierr);

					//printf("row %d column %d, value %12.3le\n", block_row_number, block_column_number,
							//current_block->Dia[i + current_block->Ndof*j]);
					block_column_number++;
				}
				block_row_number++;
			}
			//printf("\n");
			//aux block
			int current_aux_block_number;
			for(current_aux_block_number = 1; current_aux_block_number <= current_block->Lngb[0];
					current_aux_block_number++)
			{
				itt_blocks* current_aux_block = matrix->Block[current_block->Lngb[current_aux_block_number]];
				block_row_number = current_block->Posg;
				for(i = 0; i<current_block->Ndof; i++)
				{
					block_column_number = current_aux_block->Posg;
					for(j = 0; j<current_aux_block->Ndof; j++)
					{
						PetscErrorCode ierr;
						ierr = MatSetValues(A,1,&block_row_number,1,&block_column_number,
								&(current_block->Aux[current_aux_block_number - 1][i + current_block->Ndof*j]),
								INSERT_VALUES);CHKERRABORT(PETSC_COMM_WORLD, ierr);
						//printf("row %d column %d, value %12.3le\n", block_row_number, block_column_number,
								//current_block->Aux[current_aux_block_number - 1][i + current_block->Ndof*j]);
						block_column_number++;
					}
					block_row_number++;
				}
			}

			//rhs
			for(i = 0; i<current_block->Ndof; i++)
			{
				PetscErrorCode ierr;
				int global_colum_number = current_block->Posg + i;
				ierr = VecSetValues(b,1,&global_colum_number,&(current_block->Rhs[i]),INSERT_VALUES); CHKERRABORT(PETSC_COMM_WORLD, ierr);
				//printf("row %d value %12.3le \n", i, current_block->Rhs[i]);
			}
		}
		//printf("\n");
	}
	printf("KONIEC SUMOWANIA\n");
	PetscErrorCode ierr;
	ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY); CHKERRABORT(PETSC_COMM_WORLD, ierr);
	ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY); CHKERRABORT(PETSC_COMM_WORLD, ierr);
	ierr = MatView(A,PETSC_VIEWER_STDOUT_WORLD); CHKERRABORT(PETSC_COMM_WORLD, ierr);
	ierr = VecView(b,PETSC_VIEWER_STDOUT_WORLD); CHKERRABORT(PETSC_COMM_WORLD, ierr);

	s = new AMGSolverStructure(A,b,matrix->Nrdofgl);
}

void pets_allocate_SM_and_LV(int Nrdof_glob)
{
	  printf("GLOBALNA %d\n",Nrdof_glob);
	  PetscInitialize(NULL,NULL,"/home/damian/.petscrc",NULL);
	  PetscOptionsView(PETSC_VIEWER_STDOUT_WORLD);
	  PetscErrorCode ierr;
	  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,Nrdof_glob,Nrdof_glob);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	  ierr = MatSetFromOptions(A);CHKERRABORT(PETSC_COMM_WORLD, ierr);

	  ierr = VecCreate(PETSC_COMM_WORLD,&b);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	  ierr = VecSetSizes(b,PETSC_DECIDE,Nrdof_glob);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	  ierr = VecSetFromOptions(b);CHKERRABORT(PETSC_COMM_WORLD, ierr);
}

void petsc_initialize_SM_and_LV()
{
	  printf("INICJALIZACJA\n");
	  PetscErrorCode ierr;
	  ierr = MatMPIAIJSetPreallocation(A,100,NULL,100,NULL);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	  ierr = MatSeqAIJSetPreallocation(A,100,NULL);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	  ierr = MatSetUp(A);CHKERRABORT(PETSC_COMM_WORLD, ierr);
}

void petsc_perform_BJ_iterations(double* V, double* B, int Ndof, int Nr_prec)
{
	PetscErrorCode ierr;
	Vec v;
	ierr = VecCreate(PETSC_COMM_WORLD,&v);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	ierr = VecSetSizes(v,PETSC_DECIDE,Ndof);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	ierr = VecSetFromOptions(v);CHKERRABORT(PETSC_COMM_WORLD, ierr);

	int h;
	printf("\n\n\n\n\n PERFORMING BJ ITERATION n: %d nr p: %d rhs: %s \n\n\n\n\n",Ndof,Nr_prec, B == NULL ? "true" : "false");
	//for(h = 0; h<Ndof; h++)
	//{
		//ierr = VecSetValues(v,1,&h,&(V[h]),INSERT_VALUES);
		//printf("%le ", V[h]);
	//}
	ierr = VecSet(v,0.0);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	printf("\n");

	JacobiMethod* jacobiMethod = new JacobiMethod();
	jacobiMethod->PreSmoothing(A,b,v);
	int i;
	for(i = 0; i<1; i++)
	{
		jacobiMethod->Smooth(v);
		printf("\n Vector after petsc iteration \n");
		VecView(v,PETSC_VIEWER_STDOUT_WORLD);
		printf("\n");
	}

	/*printf("\n Vector after petsc iteration \n");
	VecView(v,PETSC_VIEWER_STDOUT_WORLD);
	printf("\n");*/
	/*
	 *  Vector after petsc iteration
Vec Object: 1 MPI processes
  type: seq
0.151728
0.439296
0.423052
0.0564497
0.439296
1.22486
0.163438
0.151728
*/

	//print_block_matrix();
	mf_log_info("%s","sample log");
}

/*void petsc_perform_BJ_iterations(double* V, double* B, int Ndof, int Nr_prec)
{
	PetscErrorCode ierr;
	Vec v;
	ierr = VecCreate(PETSC_COMM_WORLD,&v);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	ierr = VecSetSizes(v,PETSC_DECIDE,Ndof);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	ierr = VecSetFromOptions(v);CHKERRABORT(PETSC_COMM_WORLD, ierr);

	int h;
	printf("\n\n\n\n\n PERFORMING BJ ITERATION n: %d nr p: %d rhs: %s \n\n\n\n\n",Ndof,Nr_prec, B == NULL ? "true" : "false");
	//for(h = 0; h<Ndof; h++)
	//{
		//ierr = VecSetValues(v,1,&h,&(V[h]),INSERT_VALUES);
		//printf("%le ", V[h]);
	//}
	ierr = VecSet(v,0.0);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	printf("\n");

	KSP ksp;
	PC pc;

	KSPCreate(PETSC_COMM_WORLD,&ksp);
	KSPSetOperators(ksp,A,A);

	KSPGetPC(ksp,&pc);
	PCSetType(pc,PCJACOBI);

	ierr = KSPSetFromOptions(ksp);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	KSPSetUp(ksp);CHKERRABORT(PETSC_COMM_WORLD, ierr);
	KSPSolve(ksp,b,v);
	KSPView(ksp,PETSC_VIEWER_STDOUT_WORLD);

	PetscInt iterations_number;
	KSPGetIterationNumber(ksp,&iterations_number);

	PetscPrintf(PETSC_COMM_WORLD,"iterations %D\n", iterations_number);
	KSPDestroy(&ksp);

	printf("\n Vector after petsc iteration \n");
	VecView(v,PETSC_VIEWER_STDOUT_WORLD);
	printf("\n");


	//print_block_matrix();
	mf_log_info("%s","sample log");
}*/
/*
 *  Vector after petsc iteration
Vec Object: 1 MPI processes
type: seq
0.151728
0.439296
0.423052
0.0564497
0.439296
1.22486
0.163438
0.151728
*/


void create_amg_solver_levels()
{

}

void run_v_cycle()
{

}
