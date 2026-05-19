#ifndef _FIELD_H_
#define _FIELD_H_

#include "fv_float.h"
#include "fv_config.h"
#include "../../utils/fv_assert.h"
#include "../../include/BaseField.h"
#include "Mesh.h"
#ifdef _USE_FV_EXT_MOD
#include "../../include/ApproxModule.h"
#endif
#include "GraphicElem.hpp"
#include <utility>
#include <vector>
#include <string>


#define FV_SMALL 1e-10
#define FV_LARGE 10e10f
#define FV_DEF_P 101
#define FV_DEF_MIN_P_STD 1
#define FV_DEF_MAX_P_STD 9
#define FV_DEF_MIN_P_DG 101
#define FV_DEF_MAX_P_DG 909


namespace FemViewer {

	class ModelControler;
	class Mesh;
	class Object;
	class RenderParams;
	class CutPlane;
	class Legend;


	class Field;
	typedef void (Field::*renderMethod)(void*);

	class  Field : public BaseField
	{
		friend class ModelControler;
	public:
		typedef std::pair<double,double> range_value_t;
		typedef std::vector<GraphElement2<double> > vGraphicElems2;
	private:
		/// Min/max ranges
		range_value_t _values;
		range_value_t _xgrads;
		range_value_t _ygrads;
		range_value_t _zgrads;
		// Min/max pdeg
		int _minp;
		int _maxp;
		/// Reference to associated mesh
		Mesh& 		_pmesh;
		/// Reference to Geomtry object
		Object* 	_pobject;
		//int         _glId;
		/// Name of the field or path to field file
		std::string _name;
		/// Min/max ranges
		vGraphicElems2 _baseGTriElemsFull;
		vGraphicElems2 _baseGTriElemsCut;
		renderMethod   _renderer;
		mutable double _minValue;
		mutable double _maxValue;
	public:
		const range_value_t& GetValues() const { return _values; }
		const range_value_t& GetXGrads() const { return _xgrads; }
		const range_value_t& GetYGrads() const { return _ygrads; }
		const range_value_t& GetZGrads() const { return _zgrads; }
		//int& GetGLId() { return _glId; }
		explicit Field(Mesh& pmesh_,const std::string& name_ = "");
		~Field();

		int  Init(const int parent_id_,const char* file_name_);
		size_t CountCoeffs(std::vector<int>* degree_ptr = nullptr) const ;
		int  Free();
		void Reset(bool bClearGL = true);
		void Clear();
		//int  Reload();
		//int  Update();
		Object*& GetGLHandle() { return _pobject; }
		
		bool IsGeometryInit() const { return _pobject!=NULL; }
		void InitGeometry(void * pRootObject = NULL, char type = '\0');
		void ResetGeometry(void * pRootObj, char type = '\0');
	
		void Draw (void *ptr);
		void Render();
		void RenderDG(void *ptr);
		void RenderSTD(void *ptr);
		size_t PackCoeffs(std::vector<mfvFloat_t>& Coeffs,std::vector<int>& Degree);
		static uint32_t FillArrayWithCoeffs(const Field *pField,CoordType Coeffs[]);
	private:
		bool CreateGraphicElements(const int aux,std::vector<GraphElement2<double> >& elemsContener_);

		bool CreateGraphicCuttedElements(const CutPlane& plane_,const int iaux,std::vector<GraphElement2<double> >& elemsContener_);

		void CreateDisplayListColors(Legend& legend, std::vector<GraphElement2<double> >& gEl,Object* obj_);

		void CretaeContourLines(Legend& legend, std::vector<GraphElement2<double> >& gEl, Object* obj_,bool coloredLines = false);

		void GetValuesRange(const int iaux);

		void CalculateMinMaxValues() const;

	public:
		template<typename T>
		int GetElementDofs(int nel,T Dofs[]) const {
			return get_element_dofs(this->idx(),nel,this->_solution.currSolution,Dofs);
		}
		int GetElementDegree(int nel) const {
			return get_el_pdeg(this->idx(), nel, NULL);
		}
		int GetElementBaseType(int nel) const {
			return get_base_type(this->idx(), nel);
		}
		int GetNumberOfShapeFunc(int nel,int* degree) const {
			(void)get_el_pdeg(this->idx(),nel,degree);
			return get_el_pdeg_numshap(this->idx(), nel, degree);
		}
		int GetDegreeVector(int nel, int Pdeg[]) const;
		int CalculateSolution(int control, int pdeg, int base,
							  double* ElCoords,
							  double* Ploc,
				              double* Coeffs,
				              double* Sol,
				              double* DSolx = NULL,
				              double* DSoly = NULL,
				              double* DSolz = NULL) const;

		//static void CollectSolutionCoefs()
		#ifdef FV_DEBUG
		void dumpField () const;
		#endif
	private:

	};

template<>
inline int Field::GetElementDofs<float>(int nel,float Dofs[]) const
{
	double lDofs[1024];
	int nr_dofs = get_element_dofs(this->idx(),nel,this->_solution.currSolution,lDofs);
	for (int i(0); i<nr_dofs; ++i) Dofs[i] = static_cast<float>(lDofs[i]);
	return nr_dofs;
}

}// end namespace

#endif /* _FIELD_H_
*/
