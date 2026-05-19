#ifndef _ModelCtrl_H_
#define _ModelCtrl_H_

#include "Common.h"
#include "Geometry.h" // priomz_info, etc
#include "defs.h"
#include "ocl.h"
#include "BaseHandle.h"
#include "Mesh.h"
#include "Field.h"
#include "RContext.h"
#include "GraphicElem.hpp"
#include "CutPlane.h"
#include "BBox3D.h"
#include "../utils/fv_exception.h"

// stl, system
#include<string>
//#include<stdexcept>
#include<cassert>
#include<vector>



namespace FemViewer
{

	// Forword declarations
	class  Object;
	class  VtxAccumulator;
	class  ViewManager;
	class  BBox3D;
	struct SolutionData;
		
	class ModelCtrl //: public mfvBaseObject
	{
		//friend class mfvWindow;
		friend ModelCtrl & ModelCtrlInst(void);
		//friend int init_data();
	public:
		enum eCommnad {
			INIT 	= 0,
			CLEAR	= 1,
			UPDATE  = 2,
			INIT_MESH  = (UPDATE << 1),
			INIT_FIELD = (UPDATE << 2),
			INIT_ACCEL = (UPDATE << 3),
			INIT_GL	   = (UPDATE << 4),
			INIT_LEGEND = (UPDATE << 5),
			INIT_SOL    = (UPDATE << 6),
			INIT_PLANE = (UPDATE << 7),
	    };
		SolutionData CurrentSolution;
	public:
		// Dtr
		~ModelCtrl();
		
		bool Do(const int oper = INIT,const char* path_ = nullptr);
		void Reset(bool eraseall = false);
		void Clear();
		void Destroy();
		void Draw();
		void SetMeshChange() { _meshChange = true; }
		void SetFieldChange() { _fieldChange = true; }
		void SetLegendChange() { _legendChange = true; }
		void SetCutPlaneChange() { _solutionChange = true; }
		int InitData();
	public:
		std::vector<prism_info<mfvFloat_t> > elData;
		std::vector<coeffs_info<mfvFloat_t> > coeffsData;
		gridinfo_s gridData;
		int*       C_ptr;
		int*	   L_ptr;
	protected:
		// Ctr
		explicit ModelCtrl();
		//TArrayPtrs<BaseHandle*,10> _objArray;
		Mesh      _mesh;
		Field	  _field;
		RContext* _pRC;
		VtxAccumulator* _accum;
		bool	_meshChange;
		bool	_fieldChange;
		bool	_legendChange;
		bool	_solutionChange;
		bool    _accelChange;
		bool    _glChange;
		CutPlane  _curPlane;
		host_info_id _hosts;
#ifdef PARALLEL
		int 	  _procId;
		int		  _nrProcesses;
#endif


	public:				
		Field* GetCurrentField() { return &_field;  }
        Mesh* GetCurrentMesh() { return &_mesh; }

		bool& MeshChange() { return _meshChange; }
		const bool& MeshCnage() const { return _meshChange; }

		bool& FieldChange() { return _fieldChange; }
		const bool& FieldChange() const { return _fieldChange; }

		bool& LegendChange() { return _legendChange; }
		const bool& LegendChange() const { return _legendChange; }

		CutPlane& GetCutPlane() { return _curPlane; }
		RContext* RenderingContext()  { return _pRC; }
		const RContext* RenderingContext() const { return _pRC; }

		inline int GetMeshModuleType() const { return Mesh::GetMeshModuleType(); }
		inline int GetApproximationType() const { return Field::GetApproximationType(); }

		const BBox3D& Boundary() const { return _mesh.GetMeshBBox3D(); }

	private:

		bool InitMesh(const char* file_name_ = nullptr);
		bool InitField(const char* file_name_= nullptr,
				       const HandleType type_= Unknown);
		bool InitVertexAccumulator();
		bool InitAccelStruct();

		void EraseMesh(const int& idx_);
		void EraseField(const int& idx_);

		template<class TEntity>
	    TEntity* GetEntity(const int idx_,const char* name_);

		template<class TMap, class T>
		inline T* GetTPtr(TMap& map_, const int & id_);
		template<class T>
		int RenderColoredLines();

		void BeforeRender();
		void AfterRender();
		bool SetBoundaryElems();
		int  TestConnection() const;
		int  InitHosts(int numHosts);

	private:
		// Block use of them
		ModelCtrl(const ModelCtrl&);
		ModelCtrl& operator=(const ModelCtrl&);
	};


	extern ModelCtrl& ModelCtrlInst(void); 

	extern int init_data();



} // end namespace FemViewer
#endif /* _ModelCtrl_CONTROLER_HPP_
*/
