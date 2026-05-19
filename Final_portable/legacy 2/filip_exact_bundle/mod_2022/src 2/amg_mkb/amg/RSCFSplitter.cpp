/*
 * RSCFSplitter.cpp
 *
 *  Created on: Jun 6, 2015
 *      Author: damian
 */

#include "RSCFSplitter.hpp"

RSCFSplitter::RSCFSplitter(Mat mat, double strength_threshold) : CFSplitter(mat)
{
	this->strength_threshold = strength_threshold;
}

RSCFSplitter::~RSCFSplitter()
{

}

void RSCFSplitter::MakeCFSplitting()
{
	PetscErrorCode ierr;
	PetscInt first_row_in_range;
	PetscInt range_end;
	ierr = MatGetOwnershipRange(mat,&first_row_in_range,&range_end); CHKERRABORT(PETSC_COMM_WORLD, ierr);

	PetscInt ownership_range_size = range_end - first_row_in_range;

	/*
	for(PetscInt i = 0; PetscInt < ownership_range_size - 1; i++)
	{
		row_info_array[i].next_row_info = &(row_info_array[i+1])
	}
	row_info_array[range_end - 1] = NULL;
	*/

	const PetscInt* columns;
	const PetscScalar* values;
	PetscInt columns_number;


	row_info_array = new struct row_info[ownership_range_size];
	influenced_info_array = new struct influenced_info[ownership_range_size];

	//compute initial influence
	for(PetscInt i = first_row_in_range; i<range_end; i++)
	{
		row_info_array[i - first_row_in_range].local_row_number = i - first_row_in_range;
		influenced_info_array[i - first_row_in_range].row_info = &row_info_array[i - first_row_in_range];

		MatGetRow(mat,i,&columns_number,&columns,&values);

		PetscScalar row_min = *std::min_element(values,values+columns_number);
		for(PetscInt j = 0; j<columns_number; j++)
		{
			PetscScalar value = values[j];
			if( (-value >= strength_threshold*row_min) && (columns[j] != i) && (columns[j] >= first_row_in_range && columns[j] < range_end) )
			{
				influenced_info_array[columns[j] - first_row_in_range].influenced_number++;
				ReplaceAfterInfluencedNumberChange(influenced_info_array[columns[j] - first_row_in_range].row_info);
			}
		}

		MatRestoreRow(mat,i,&columns_number,&columns,&values);
	}

	//start c-f splitting

	int rows_on_heap = ownership_range_size;
	while(rows_on_heap)
	{
		row_info* highest_influence_row_info = &row_info_array[0];
		row_info* last_heap_row_info = &row_info_array[ownership_range_size - 1];

		PetscInt row_number = highest_influence_row_info->local_row_number + first_row_in_range;
		highest_influence_row_info->set = CSET;
		if(highest_influence_row_info->set == DEFAULT)
		{
			MatGetRow(mat,highest_influence_row_info->local_row_number + first_row_in_range,&columns_number,&columns,NULL);


			PetscInt neighboring[columns_number];
			//set to F all neighbors that are not C
			PetscInt column_index = 0;
			for(PetscInt j = 0; j<columns_number; j++)
			{
				if((columns[j] >= first_row_in_range && columns[j] < range_end) && (row_number != columns[j]))
				{
					neighboring[column_index++] = columns[j];
					row_info* neighboring_row_info = influenced_info_array[columns[j] - first_row_in_range].row_info;
					if(neighboring_row_info->set == DEFAULT)
					{
						neighboring_row_info->set = FSET;
					}
				}
			}
			PetscInt neighboring_number = column_index;

			MatRestoreRow(mat,highest_influence_row_info->local_row_number + first_row_in_range,&columns_number,&columns,NULL);

			for(PetscInt j = 0; j<neighboring_number; j++)
			{
				MatGetRow(mat,neighboring[j],&columns_number,&columns,NULL);

				for(PetscInt k = 0; k<columns_number; k++)
				{
					if((columns[k] >= first_row_in_range && columns[k] < range_end) && (neighboring[j] != columns[k]))
					{
						row_info* neighbor_neighboring_row_info = influenced_info_array[columns[k] - first_row_in_range].row_info;
						if(neighbor_neighboring_row_info->set == DEFAULT)
						{
							influenced_info_array[columns[k] - first_row_in_range].influenced_number++;
						}
					}
				}
				MatRestoreRow(mat,neighboring[j],&columns_number,&columns,NULL);
			}

			replaceRowInfos(highest_influence_row_info, last_heap_row_info);

		}
		else
		{
			replaceRowInfos(highest_influence_row_info, last_heap_row_info);
		}

			//funkcja do kopcowania
		rows_on_heap--;
	}
}

struct row_info* RSCFSplitter::getHeapParent(int index)
{
	return &row_info_array[(index+1)/2 - 1];
}

void RSCFSplitter::replaceRowInfos(struct row_info* row_info1, struct row_info* row_info2)
{
	influenced_info_array[row_info1->local_row_number].row_info = row_info2;
	influenced_info_array[row_info2->local_row_number].row_info = row_info1;

	Set set = row_info1->set;
	PetscInt tmp_local_row_number = row_info1->local_row_number;

	row_info1->set = row_info2->set;
	row_info1->local_row_number = row_info2->local_row_number;

	row_info2->set = set;
	row_info2->local_row_number = tmp_local_row_number;
}

void RSCFSplitter::ReplaceAfterInfluencedNumberChange(struct row_info* row_info)
{
	while(true)
	{
		int index_in_heap = row_info_array - row_info;
		if(index_in_heap == 0)
			return;

		struct row_info* parent_row_info = getHeapParent(index_in_heap);
		if(influenced_info_array[row_info->local_row_number].influenced_number >
					influenced_info_array[parent_row_info->local_row_number].influenced_number)
		{
			replaceRowInfos(row_info, parent_row_info);
			row_info = parent_row_info;
		}
		else
		{
			return;
		}
	}
}


