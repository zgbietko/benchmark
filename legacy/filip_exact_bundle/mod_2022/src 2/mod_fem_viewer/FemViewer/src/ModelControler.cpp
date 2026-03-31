#include "Enums.h"
#include "BBox3D.h"
#include "Geometry.h"
#include "Accelerators.h"
#include "ModelControler.h"
#include "ViewManager.h"
#include "BaseHandle.h"
#include "Mesh.h"
#include "Field.h"
#include "Object.h"
#include "VtxAccumulator.h"
#include "WindowFV.h"
#include "fv_compiler.h"
#include "fv_config.h"
#include "fv_exception.h"
#include "fv_assert.h"
#include "fv_txt_utls.h"
#include "fv_timer.h"
#include "defs.h"
#include "Log.h"
#include "Legend.h"
#include <sstream>
#include<iostream>
#include <algorithm>
#include <vector>
#include "mmh_intf.h"
#include "ocl/ocl.h"
#include <omp.h>

#ifdef PARALLEL
#include "mmph_intf.h"
#include "pch_intf.h"
#endif

namespace FemViewer {

	//static const int DefaultMeshType 		 = MeshPrizm;
	//static const int DefaultAppoximationType = FieldDG;

	ModelCtrl& ModelCtrlInst(void) {
		//mfp_log_debug("ModelCtrInst\n");
		static ModelCtrl modelCtrlImpl;
		return modelCtrlImpl;
	}

	ModelCtrl::ModelCtrl()
	: C_ptr(NULL)
	, L_ptr(NULL)
	, _mesh()
	, _field(_mesh)
	, _pRC(NULL)
	, _accum(NULL)
	, _meshChange(true)
	, _fieldChange(true)
	, _solutionChange(true)
	, _legendChange(true)
	, _accelChange(true)
	, _glChange(true)
	, _curPlane()
	, _hosts(NULL)
	{
		//mfp_log_debug("Ctr");
		memset(&this->gridData, 0x0, sizeof(gridinfo_s));
		const int numHosts(1);
#ifdef PARALLEL
		assert(pcr_is_parallel_initialized() == 1);
		_procId 	 = pcr_my_proc_id();
		_nrProcesses = pcr_nr_proc();
		numHosts = _nrProcesses;
#endif
		//InitHosts(numHosts);
	}

	ModelCtrl::~ModelCtrl()
	{
		//mfp_debug("dtr\n");
		FV_FREE_PTR(C_ptr);
		FV_FREE_PTR(L_ptr);
		FV_FREE_PTR(_pRC);
		FV_FREE_ARR(_hosts);
	}


	bool ModelCtrl::InitMesh(const char* file_name_)
	{
		//mfp_log_debug("ModelCtrl: InitMesh\n");
		_mesh.Reset();
		if (_mesh.Init(file_name_)<0) throw fv_exception("Can't init mesh!");
		//mfp_debug("After\n");
		_meshChange = false;
		return true;
	}

	bool ModelCtrl::InitField(const char* file_name_,const HandleType type_)
	{
		_field.Reset();
		if (!_mesh.IsInit()) throw fv_exception("Uninitialized mesh while loading field data!");
		if (_field.Init(-1,file_name_)<0) throw fv_exception("Can't init field\n");
		_fieldChange = false;
		return true;
	}

	bool ModelCtrl::InitVertexAccumulator()
	{
		//mfp_log_debug("Init vertexaccumulator\n");
		_accum = & (ViewManagerInst().GetGraphicData().GetRootObject().GetCurrentVtxAccumulator());
		//mfp_log_debug("Liczba vertekxów: %u %d\n",_accum->getVertices().size(),_mesh.GetNumNodes());
		bool result = _accum->init(_mesh.GetNumNodes(),_mesh.GetNumEdges(),_field.GetType());
		_mesh.RenderWireframe(_accum);
		_field.Draw(_accum);
		if (result) _accum->create();
		//mfp_debug("after create\n");
		return true;
	}

	bool ModelCtrl::InitAccelStruct()
	{
		//mfp_log_debug("InitAccelStruct\n");
		if (_pRC) delete _pRC;
		_pRC = new RContext(ViewManagerInst(),&_mesh,&_field);
		mfvBaseObject::parentMeshPtr = &_mesh;
		mfvBaseObject::parentFieldPtr = &_field;
		//mfp_log_info("Before building accelstruct\n");
		fv_timer t;
		t.start();
		this->_pRC->accelStruct = new Grid(_pRC);
		//this->_pRC->accelStruct = new BVH(_pRC);
		//mfp_log_debug("Building time: %lf\n",t.get_time_ms());
		//this->Do(UPDATE);
		//this->_field.Draw();
		//FV_CHECK_ERROR_GL();
		_accelChange = false;
		return true;
	}


	void ModelCtrl::Clear()
	{
		//mfp_log_debug("Clearing data\n");
		memset(&this->gridData, 0x0, sizeof(gridinfo_s));
		this->_mesh   .Reset();		// reset mesh object
		this->_field  .Reset();		// reset field object
		FV_FREE_PTR(_pRC); 			// reset rendering context
		//this->_vEGL.clear();		// clear vector of graphic elements
		ViewManagerInst().Reset();  // reset view-manger
		this->_curPlane.Reset();	// set to default cutting plane
		this->_meshChange = true;
		this->_fieldChange = true;
		this->_solutionChange = true;
		this->_legendChange = true;
		this->_accelChange = true;
		this->_glChange = true;
		// resetting vertex accumulator

		//ViewManagerInst().GetLegend().Create(); // recreate the legend
		//ViewManagerInst().GetGraphicData().GetRootObject().GetCurrentVtxAccumulator().reset(2);
		//mfp_log_debug("After reset size is %u\n",ViewManagerInst().GetGraphicData().GetRootObject().GetCurrentVtxAccumulator().getVertices().size());
	}

	void ModelCtrl::EraseMesh(const int & id_)
	{
		//BaseHandle*& hMesh = _objArray.at(id_);

		//for (size_type fid(0);fid<_objArray.Size();++fid)
		//{
		//	if (!_objArray[fid] || (_objArray[fid]->type() & FieldHandle)) continue;
		//	this->EraseField(fid);
		//}
		//FV_FREE_PTR(hMesh);
		assert(!"Not Implemented");

	}

	void ModelCtrl::EraseField(const int& id_)
	{
		assert(!"Not Implemented");
	}

	bool ModelCtrl::Do(const int oper,const char* path)
	{
		bool result = true;
		int mask = INIT | CLEAR | UPDATE;
		int mcmd = oper & mask;
		switch (mcmd)
		{
		// Do all init jobs
			case INIT:
			{
				//mfp_log_debug("Initializing data\n");
				//InitHosts(1);
				// Test conections
				if (!TestConnection()) {
					#ifdef PARALLEL
					mfp_log_debug("Exit because of wrong test connection\n");
					pcr_exit_parallel();
					#endif
				}
				try {
					if (_meshChange) {
						InitMesh(path);
					}
					//mfp_debug("After init mesh\n");
					if (_fieldChange) InitField(path);
					//mfp_debug("After init field\n");
					if (_glChange) InitVertexAccumulator();
					//mfp_debug("After init accumulator\n");
					if (_accelChange) InitAccelStruct();
					if (_legendChange) ViewManagerInst().GetLegend().Create();
					//mfp_debug("After init accel struct");
				} catch (fv_exception& ex) {
					//mfp_log_err("%s\n",ex.what());
					result = false;
				}
			} break;

				// Try init mesh data and then field data
//				if (this->Do(INITMESH))
//				{
				#ifdef PARALLEL
					IO::printSynchronizedParallel("Hello\n");
					const int l = 6;
					const int MCTRL_TAG_ID = 33456;
					BBox3D lbbox(_mesh.GetMeshBBox3D());
					float tab[l];
					if (this->_procId == pcr_print_master()) {

						for(int i = 1; i <= this->_nrProcesses; ++i){
							if (this->_procId == i)
							{
								tab[i] = true;
								continue;
							}
							pcr_receive_float(i,MCTRL_TAG_ID,l,tab);
							lbbox += BBox3D(fvmath::CVec3f(tab),fvmath::CVec3f(&tab[3]));

							mfp_log_debug("receiv bbox: min {%f, %f %f} max {%f, %f, %f}\n",tab[0],tab[1],tab[2],tab[3],tab[4],tab[5]);

						}
					}
					else {
						mfp_log_debug("Sending bbox min = {%f, %f, %f\n",lbbox.Xmin(),lbbox.Ymin(),lbbox.Zmin());
						pcr_send_float(pcr_print_master(),MCTRL_TAG_ID,l,lbbox.data());
					}

					pcr_barrier();
				#endif


		case UPDATE:
			{
				//mfp_debug("InUPDATE:\n)");
				if (oper & INIT_MESH) { _meshChange = true; mfp_log_info("Mesh has been changed\n"); }
				if (oper & INIT_FIELD) { _fieldChange = true; mfp_log_info("Approximation has been changed\n"); }
				if (oper & INIT_LEGEND) { _legendChange = true; mfp_log_info("ColorMap has been changed\n"); }
				if (oper & INIT_SOL)   {_solutionChange = true; mfp_log_info("Solution has been changed\n"); }
				if (oper & INIT_ACCEL) { _accelChange = true; mfp_log_info("Accel. structure has been changed\n"); }
				if (oper & INIT_GL) {_glChange = true; mfp_log_info("GL buffers has been changed\n"); }


				//Mesh * pm = GetCurrentMesh();
				//if(! pm->IsNotEmpty())
				//{
				//	// Do copy vertex data to GPU
				//	fvmath::Vec3f vertex;
				//	for(int i=1; i <= pm->GetNumNodes(); ++i) 
				//	{
				//		if (pm->GetNodeCoor<float>(i,vertex.v)>0)
				//			pm->GetGLHandle()->AddMeshNode(vertex);
				//	}
				//	// Get the number of vertices
				//	Object*& pobj = pm->GetGLHandle();
				//	const float * pv  = pobj->GetCurrentVtxAccumulator().getVertices().data()->coord();
				//	int numNodes = pobj->GetCurrentVtxAccumulator().getVertices().size();
				//	glGenBuffersARB(2, ViewManagerInst().vbo_id);
				//	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * 3 * numNodes, pv, GL_STATIC_DRAW_ARB);

				//}
				//if(GetCurrentField()->IsSolutionChanged());
				//if(ViewManagerInst().GetLegend().IsLegendChanged());
				// Set settings
				//mfp_debug("In UPDATE after set\n");
				VtxAccumulator *accum_ptr = &(ViewManagerInst().GetGraphicData().GetRootObject().GetCurrentVtxAccumulator());
				accum_ptr->aRenderEdges = ViewManagerInst().GetSettings()->bEdgeOn;
				accum_ptr->aRenderIsoLines = ViewManagerInst().GetSettings()->bIsovalueLineOn;
				if (_field.GetType() == FieldDG) {
					accum_ptr->aRenderTraingleStrips = ViewManagerInst().GetSettings()->bShadingOn;
				} else {
					accum_ptr->aRenderTraingles = ViewManagerInst().GetSettings()->bShadingOn;
				}
				accum_ptr->update();
				_pRC->accelStruct->drawOn(ViewManagerInst().GetSettings()->bIsGridDraw);
				//mfp_debug("Adfre update");

		}
		break;
		case CLEAR: {
			Reset(true);
		}
		break;
		case INIT_LEGEND:
		{
			/*VtxAccumulator::NodeColors& colors = _field.GetGLHandle()->GetCurrentVtxAccumulator().getNodeColors();
			VtxAccumulator::NodeColors::size_type size = colors.size();
			Legend& legend = ViewManagerInst().GetLegend();
			//legend.Dump();
			//std::cout<<"Po dump legend\n";
			for (unsigned i = 0; i < size; i++)
			{

				fvmath::Vec3f& col = colors[i];
				double value = legend.GetValue ( ColorRGB( col.x, col.y, col.z));

				if (value < legend.GetStart() || value > legend.GetEnd()) {
					std::cout << "<<<<<<<<<<<<<< = " << value << std::endl;
				}
				std::cout << "i = " << i << std::endl;
				_currLegend.GetColor(value);
				col.x = _currLegend.color.rgb.r;
				col.y = _currLegend.color.rgb.g;
				col.z = _currLegend.color.rgb.b;
				if (col.x ==0 && col.y ==0 && col.z==0) std::cout << "kolor czarny dla wart: " << value << std::endl;
			}
			for(size_t i(0); i < _currLegend.GetColors().size(); ++i)
				std::cout << i <<" = " << _currLegend.GetColors()[i].value << std::endl;
			ViewManagerInst().GetLegend() = _currLegend;
			ViewManagerInst().GetLegend().Update();
			*/
		}
		break;


		}// end switch

		return result;
	}

	void ModelCtrl::Reset(bool all_)
	{
		//mfp_log_debug("reset modelctr\n");
		Clear();
		if(all_) this->Destroy();
	}

	void ModelCtrl::Destroy()
	{
		FV_ASSERT(!"Not implemented");
	}

	void ModelCtrl::BeforeRender()
	{
		//_vEGL.clear();
		//Mesh* currMesh = GetTPtr<ModelCtrlMap,Mesh>(_meshes, _mId);
	}


	int  ModelCtrl::TestConnection() const
	{
#ifdef PARALLEL
		const int MCTRL_TAG_ID = 1342;
		const int l = 1;
		mfp_log_debug("W tescie\n");
		if(this->_procId == pcr_print_master()) {
			assert(_nrProcesses >= 2);
			mfp_log_debug("waiting\n")
			int *tab = new int[_nrProcesses + 1];
			tab[this->_procId] = this->_procId;
			for(int i = 1; i <= this->_nrProcesses; ++i){
				if (this->_procId == i)
				{
					tab[i] = true;
					continue;
				}
				pcr_receive_int(i,MCTRL_TAG_ID,l,&tab[i]);
			}
			mfp_log_debug("Checinkg\n")
			// Ceck table if everything is OK
			for(int i = 1; i <= this->_nrProcesses; ++i) {
				bool test = (i == tab[i]);
				mf_test(test, "Master: report incorrect number %d from process %d!\n",tab[i],i);
				if (!test) {
					FV_DELETE_ARRAY(tab);
					return(0);
				}
			}
			FV_DELETE_ARRAY(tab);
			mfp_log_debug("After checking\n");
		}
		else {
			mfp_log_debug("Sending\n");
			pcr_send_int(pcr_print_master(),MCTRL_TAG_ID,l,const_cast<int *>(&(this->_procId)));
		}
		mfp_log_debug("Waiting on barrier\n");
		pcr_barrier();
		mfp_log_debug("Test connection passed\n");
#endif
		return(1);
	}

	int ModelCtrl::InitHosts(int numHosts)
	{
//		mfp_log_debug("InitHosts\n");
//		assert(numHosts > 0);
//		assert(_hosts == nullptr);
//
//		_hosts = new host_info[numHosts];
//
//		for (int i = 0; i < numHosts; ++i) {
//			int size = initOpenCL(&_hosts[i],CL_TRUE);
//			if (size > 0) {
//				if (_hosts[i].devices.at(0).info > 0) std::cout << "GL-CL supported\n";
//			}
//		}
//
//		return 1;

	}

#define BYTES_PER_ELEM_DATA	160
#define BYTES_PER_ELEM_DOFS 512
#define ITEM_OFFSET_DOFS	128
//#define MAX_NUM_DOFS		126


// This functio initialize all data in omp way
int ModelCtrl::InitData()
{
	mfp_debug("+++++++++++++++++++++++++++++++\n");
	Legend lg;
	// Handle to model
	///ModelCtrl* mctrl_ptr = &ModelCtrlInst();
	// Initialize the geometry - mesh data and accelerator structure
	Mesh* mesh_ptr = &this->_mesh;
	//assert(mfvBaseObject::parentMeshPtr!= NULL);
	mesh_ptr->Init("ModFEM");
	size_type nEls = mesh_ptr->GetElems().size();
	Elem_Info *elinfo_ptr = mesh_ptr->GetElems().data();
	// Vectors of basic data:
	// Elem data;
	//std::vector<prism_info<mfvFloat_t> > prism_data(nEls);
	elData.resize(nEls);
	prism_info<mfvFloat_t>* pinf_ptr = elData.data();
	std::vector<BBox3D> elBBoxes(nEls);
	BBox3D* elboxes_ptr = elBBoxes.data();
	// Coeffs + pdeg data
	coeffsData.resize(nEls);
	coeffs_info<mfvFloat_t>* coeffs_ptr = coeffsData.data();


	// Initialize the approximation data - field data
	Field* field_ptr = &this->_field;
	field_ptr->Init(-1,"ModFEM");
	// get bounding box of the current mesh via list of its nodes
	BBox3D g_bbox;
	size_type nno = mesh_ptr->GetNumNodes();
	size_type nel = mesh_ptr->GetNumElems();
	assert(nno > 4);
	fvmath::Vec3d g_min = {  FV_LARGE, FV_LARGE, FV_LARGE };
	fvmath::Vec3d g_max = { -FV_LARGE,-FV_LARGE,-FV_LARGE };
	double g_minv = FV_LARGE,g_maxv=-FV_LARGE;
	fv_timer t;
	t.start();
	size_type ino, iel;
	//RContext* rctx = ModelCtrlInst().RenderingContext();
	#pragma omp parallel default(none)  shared(lg,nno,nel,g_min,g_max,g_bbox,g_minv,g_maxv) \
	firstprivate(mesh_ptr,field_ptr,elinfo_ptr,nEls,pinf_ptr,coeffs_ptr,elboxes_ptr)
	{
		fvmath::CVec3d el_coords[6];
		fvmath::Vec3d l_min = {  FV_LARGE, FV_LARGE, FV_LARGE };
		fvmath::Vec3d l_max = { -FV_LARGE,-FV_LARGE,-FV_LARGE };

		// Generating bounding box
		// Loop over array of selected elements
		BBox3D eb;
		#pragma omp for
		for (iel = 0; iel < nEls; ++iel)
		{

			fvmath::Vec3d el_min = {  FV_LARGE, FV_LARGE, FV_LARGE };
			fvmath::Vec3d el_max = { -FV_LARGE,-FV_LARGE,-FV_LARGE };
			// Grab element coordinates
			for (int ivt = 1; ivt <= elinfo_ptr[iel].nodes[0]; ++ivt) {
				// Get element coordinates from mesh to buffer
				mesh_ptr->GetNodeCoor(elinfo_ptr[iel].nodes[ivt],el_coords[ivt-1].v);
				// Compute bbox for element - min
				el_min.x = fv_min(el_min.x,el_coords[ivt-1].x);
				el_min.y = fv_min(el_min.y,el_coords[ivt-1].y);
				el_min.z = fv_min(el_min.z,el_coords[ivt-1].z);
				// max
				el_max.x = fv_max(el_max.x,el_coords[ivt-1].x);
				el_max.y = fv_max(el_max.y,el_coords[ivt-1].y);
				el_max.z = fv_max(el_max.z,el_coords[ivt-1].z);
				// Store coordinates
				pinf_ptr[iel].vts[ivt-1] = el_coords[ivt-1];
			}
			// Store for local bbox of the thread
			l_min.x = fv_min(l_min.x,el_min.x);
			l_min.y = fv_min(l_min.y,el_min.y);
			l_min.z = fv_min(l_min.z,el_min.z);
			l_max.x = fv_max(l_max.x,el_max.x);
			l_max.y = fv_max(l_max.y,el_max.y);
			l_max.z = fv_max(l_max.z,el_max.z);

			// Store elem bbox
			eb.mn.x = static_cast<float>(el_min.x);
			eb.mn.y = static_cast<float>(el_min.y);
			eb.mn.z = static_cast<float>(el_min.z);
			elboxes_ptr[iel].mn = eb.mn;
			eb.mx.x = static_cast<float>(el_max.x);
			eb.mx.y = static_cast<float>(el_max.y);
			eb.mx.z = static_cast<float>(el_max.z);
			elboxes_ptr[iel].mx = eb.mx;
		}
		// Accumulate to global bounding box
		#pragma omp critical(bbox)
		{
			g_min.x = fv_min(g_min.x,l_min.x);
			g_min.y = fv_min(g_min.y,l_min.y);
			g_min.z = fv_min(g_min.z,l_min.z);
			g_max.x = fv_max(g_max.x,l_max.x);
			g_max.y = fv_max(g_max.y,l_max.y);
			g_max.z = fv_max(g_max.z,l_max.z);
		}
		// ????
		#pragma omp barrier

		#pragma omp single
		{
			g_bbox.mn.x = static_cast<float>(g_min.x);
			g_bbox.mn.y = static_cast<float>(g_min.y);
			g_bbox.mn.z = static_cast<float>(g_min.z);
			g_bbox.mx.x = static_cast<float>(g_max.x);
			g_bbox.mx.y = static_cast<float>(g_max.y);
			g_bbox.mx.z = static_cast<float>(g_max.z);
//			printf("Mesh with %d vertices bbox:\n",nno);
			//printf("min={%f, %f, %f}\n\tmax={%f, %f, %f}\n",g_bbox.mn.x,g_bbox.mn.y,g_bbox.mn.z,
//					g_bbox.mx.x,g_bbox.mx.y,g_bbox.mx.z);
//			printf("sizeof prism_into<float> %u prism_info<double>%u\n",sizeof(prism_info<float>),sizeof(prism_info<double>));
//			printf("sizeof prism_into<float> %u prism_info<double>%u\n",sizeof(prism_info<float,4,4>),sizeof(prism_info<double,4,4>));
			// create accelelration strucutre
			//printf("before creating Grid\n");
			//rctx->accelStruct = new Grid(rctx,g_bbox,nEls);
		}

		// Calculate the minimal and maximal value
		size_type curr_el = 0;
		const Elem_Info* last = &elinfo_ptr[nEls];
		double el_dofs[APC_MAXELSD];
		double l_min_v = FV_LARGE,l_max_v=-FV_LARGE;
		int pdeg;
		#pragma omp for
		for (iel = 1; iel <= nel; iel++)
		{
			if (mesh_ptr->GetElemStatus(iel) != MMC_ACTIVE) continue;
			//Get element dofs
			int nr_dofs   = field_ptr->GetElementDofs(iel,el_dofs);
			int nr_shup   = field_ptr->GetNumberOfShapeFunc(iel,&pdeg);
			int base_type = field_ptr->GetElementBaseType(iel);

			coeffs_ptr->pdeg = (short)pdeg;
			coeffs_ptr->base = (short)base_type;
			coeffs_ptr->nr_shap = nr_shup;
			// Copy coeffs
			for (int ndof = 0; ndof < nr_dofs; ++ndof)
				coeffs_ptr->coeffs[ndof] = static_cast<mfvFloat_t>(el_dofs[ndof]);
			// Get element coordinate
		    // Set to first element id
			int found = 0;
			// Find element on the list by its id element
			//elinfo_ptr = std::find(elinfo_ptr,last,static_cast<id_t>(iel));
			while (elinfo_ptr!=last) {
				if (elinfo_ptr->elemId.eid == iel) {found++; break;}
				++elinfo_ptr;
				++coeffs_ptr;
				++curr_el;
			}

			if (found)
			//if (elinfo_ptr != last)
			{	// We found an element on the list
				// Take coordinates from table on the list
				for (int ivt = 1; ivt <= elinfo_ptr->nodes[0]; ++ivt) {
					// Restore el_coords to array of doubles
					mesh_ptr->GetNodeCoor(elinfo_ptr->nodes[ivt],el_coords[ivt-1].v);
					el_coords[ivt-1] = pinf_ptr[curr_el].vts[ivt-1];
				}
				// Calculate faces's planes parameters
				if (elinfo_ptr->nodes[0] == 6) {
					Plane::CreatePlanesForPrism((Plane::vec3*)el_coords,(Plane::vec4*)pinf_ptr[curr_el].pls);
					//elBBox = Prizm::BoundingBox(el_coords);
					//cent = Prizm::Centrum(el_coords);
				}
				else if (elinfo_ptr->nodes[0] == 4) {
					Plane::CreatePlanesForTetra((Plane::vec3*)el_coords,(Plane::vec4*)pinf_ptr[curr_el].pls);
					//elBBox = Tetra::BoundingBox(el_coords);
					//cent = Tetra::Centrum(el_coords);
				}
				else {
					printf("incorrect type of the element: %d\n",elinfo_ptr->nodes[0]);
					exit(-1);
				}

				//#pragma omp critical(build_grid)
				//{
				//	rctx->accelStruct->test(elBBox,cent);
				//}
				// Stor coefficents
				// store pdeg and base


			}
			else { // The element is not on the list
				// Take its coords from mesh directly
				mesh_ptr->GetElementCoordinates(iel,NULL,el_coords[0].v);
			}
			// Get elements's coefficients
			//int pdeg[3];
			{
				const double dx = 0.1;
				const double dy = dx;
				const double dz = 0.2;
				const int control = 1;
				double mnval = FV_LARGE, mxval = -FV_LARGE;
				double sol[3];
				// Loops over 10 point in each directions
				// Start at the first vertex in prism
				CVec3d pt;
				pt.z = -1.0;
				for (pt.z = -1.0; pt.z <= 1.0; pt.z += dz) {
					for (pt.y = 0.0; pt.y <= 1.0; pt.y+= dy) {
						for (pt.x = 0.0; pt.x <= 1.0 - pt.y; pt.x += dx) {
							// vertexy trzeba zamnienic na dopuible albo dac templata na floata
							field_ptr->CalculateSolution(control,pdeg,base_type,el_coords[0].v, pt.v, el_dofs, sol);
							//printf("%d sol = %lf pt = %f %f %f\n",omp_get_thread_num(),sol,pt.x,pt.y,pt.z);
							mnval = fv_min(mnval,sol[0]);
							mxval = fv_max(mxval,sol[0]);
						}
					}
				}
				if (found) {
					// Store element min/max value in table
					/// TO DO: conversion between double and float
					pinf_ptr[curr_el].min_v = static_cast<mfvFloat_t>(mnval);
					pinf_ptr[curr_el].max_v = static_cast<mfvFloat_t>(mxval);
				}
				// Store in local min max value
				l_min_v = fv_min(l_min_v,mnval);
				l_max_v = fv_max(l_max_v,mxval);
			}
		}// end for
		// Accumulate to global min/max value
		#pragma omp critical(min_max_value)
		{
			//printf("thr %d g_min = %lf g_max = %lf\n",omp_get_thread_num(), l_min_v,l_max_v);
			//printf("tid %d nthrs %d\n",omp_get_thread_num(), omp_get_num_threads() );
			//printf("elem = %d, pdeg = %u mnv= %lf mxv= %lf\n",p.getId().eid, p.degree(), p.minV(),p.maxV());
			// Bounding box
//			g_min.x = fv_min(g_min.x,l_min.x);
//			g_min.y = fv_min(g_min.y,l_min.y);
//			g_min.z = fv_min(g_min.z,l_min.z);
//			g_max.x = fv_max(g_max.x,l_max.x);
//			g_max.y = fv_max(g_max.y,l_max.y);
//			g_max.z = fv_max(g_max.z,l_max.z);
				// Min/Max value
			g_minv = fv_min(g_minv,l_min_v);
			g_maxv = fv_max(g_maxv,l_max_v);
		}
		// ?????
		#pragma omp barrier

		// initialize the color map - legend
		//#pragma omp single
		//{
			// lg.Set(g_minv,g_maxv,g_minv,g_maxv);
			 //lg.Dump();
			// Create accelerator
//			#pragma omp section
//			{ rctx->accelStruct->create(); }
		//}
		// Create OpenGL window
		/*elinfo_ptr = mesh_ptr->GetElems().data();
		BBox3D elBBox;
		#pragma omp for
		for (iel=0; iel < nEls; iel++)
		{
			int div = elinfo_ptr[iel].nodes[0];
			if (div== 6) {
				elBBox = Prizm::BoundingBox(pinf_ptr[iel].vts);
			}
			else {
				elBBox = Tetra::BoundingBox(pinf_ptr[iel].vts);
			}
			CVec3f cent(pinf_ptr[iel].vts[0]);

			for (int i=1;i<div;++i) cent += pinf_ptr[iel].vts[i];
			cent *= 1./div;
			id_t ID(iel);
			// Clear info for active and boundary
			//UNSET_BOUND(ID);
			//UNSET_ACTIVE(ID);
			#pragma omp critical(build_grid)
			{
				rctx->accelStruct->insert(elBBox,cent,ID);
			}

		}*/

	}// end pragma omp parallel
	//printf("duration = %f\n",t.get_duration_ms());
	//printf("Global values:\nmin = %lf\tmax = %lf\n",g_minv,g_maxv);
	//std::cout << g_minv << " " << g_maxv << "\n";
	lg.Set(g_minv,g_maxv,g_minv,g_maxv);
    gridinfo_t gridInfo;
    cl_int* c_ptr = NULL;
    cl_int* l_ptr = NULL;
    create_grid(density,nEls,g_bbox,elBBoxes.data(),&c_ptr,&l_ptr,&gridInfo);
    //printf("After creating grid data: %f %f %f\n",gridInfo.cellSize.x,gridInfo.cellSize.y,gridInfo.cellSize.z);
    // Bcfs C99!
    gridData.minDimension.s[0] = gridInfo.minDimension.x;
    gridData.minDimension.s[1] = gridInfo.minDimension.y;
    gridData.minDimension.s[2] = gridInfo.minDimension.z;
    gridData.maxDimension.s[0] = gridInfo.maxDimension.x;
    gridData.maxDimension.s[1] = gridInfo.maxDimension.y;
    gridData.maxDimension.s[2] = gridInfo.maxDimension.z;
    gridData.cellSize.s[0] = gridInfo.cellSize.x;
    gridData.cellSize.s[1] = gridInfo.cellSize.y;
    gridData.cellSize.s[2] = gridInfo.cellSize.z;
    gridData.cellCount.s[0] = gridInfo.cellCount.x;
    gridData.cellCount.s[1] = gridInfo.cellCount.y;
    gridData.cellCount.s[2] = gridInfo.cellCount.z;
    C_ptr = c_ptr;
    L_ptr = l_ptr;
    double stop = t.get_duration_sec();
    mfp_log_info("Data initialization: %lf sec\n",stop);
    ViewManagerInst().DrawAccelStruct(g_bbox.mn.v,g_bbox.mx.v,&gridData);
    // Init rendering context
//    if (_pRC) delete _pRC;
//    _pRC = new RContext(ViewManagerInst(),mesh_ptr,field_ptr);
    //delete [] C_ptr;
    //delete [] L_ptr;
//	int flag;
//std::cin >> flag;
//	for (size_type i(0);i <100/* static_cast<size_type>(prism_data.size())*/;++i) {
//		std::cout << "Element: " << i << " cordinates:\n";
//		std::cout << prism_data[i].vts[0] << std::endl;
//		std::cout << prism_data[i].vts[1] << std::endl;
//		std::cout << prism_data[i].vts[2] << std::endl;
//		std::cout << prism_data[i].vts[3] << std::endl;
//		std::cout << prism_data[i].vts[4] << std::endl;
//		std::cout << prism_data[i].vts[5] << std::endl;
//		std::cout << "Element: " << i << " planes:\n";
//		for(int ipl=0;ipl<5;ipl++)
//			printf("n = {%f, %f, %f\t\td = %f\n",prism_data[i].pls[ipl].x,
//					prism_data[i].pls[ipl].y,prism_data[i].pls[ipl].z,prism_data[i].pls[ipl].w);
//
//		std::cout << "Element: " << i << " coeffs data:\n";
//		printf("\tpdeg=%d base=%d nr_shap=%d\n",coeffs_data[i].pdeg,coeffs_data[i].base,coeffs_data[i].nr_shap);
//		for(int ndof=0;ndof<coeffs_data[i].nr_shap;ndof++)
//			printf("%f ", coeffs_data[i].coeffs[ndof]);
//		printf("\n");
//	}
//	printf("duration: %f ms\n",t.get_duration_ms());
	return(1);

	//mfp_debug("+++++++++++++++++++++++++++++++\n");
	return EXIT_SUCCESS;
}


	
} // end namespaces
