#ifndef _MAIN_WX_WINDOW_H_
#define _MAIN_WX_WINDOW_H_


// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
  #include "wx/wx.h"
#endif

#if !wxUSE_THREADS
    #error "This module requires thread support!"
#endif // wxUSE_THREADS

#include "wx/thread.h"
#include "wx/toolbar.h"
#ifdef WIN32
#include "../win/resource.h"
#else
#include "../wx/resource.h"
#endif



class wxFileHistory;
class wxToolBarToolBase;
class wxToolBarBase;


namespace FemViewer {

class ViewManager;
class ModelCtrl;

namespace WxGUI     {

wxDECLARE_EVENT(wxEVT_COMMAND_MYTHREAD_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_MYTHREAD_UPDATE, wxThreadEvent);


// Forword declarations
class wxMainWindow;
class wxRenderCanvas;

class RenderThread : public wxThread
{
  public:
	RenderThread(wxMainWindow *pcanvas,unsigned delay = 16)
    : wxThread(wxTHREAD_DETACHED)
    , _handler(pcanvas)
    , _iRefreshDelay(delay)
  	{ ; }
	~RenderThread();

  protected:
	virtual void* Entry();

	wxMainWindow* _handler;
	unsigned 	  _iRefreshDelay;
};





// Main Window class
class wxMainWindow : public wxFrame
{
	friend class RenderThread;
	//static const long glIDwindow;
  public:
	//static bool m_shutdown;

  public:
	/// Constructor & destructor
	wxMainWindow(const int width,const int height,
			   	 const wxChar * title);
	~wxMainWindow(void);

	void DoInit(int cmd = 0);
	void Update();

	// Thread stuff
	void StartRenderThread();
	void DoPauseThread();
	void DoResumeThread();
	void OnThreadUpdate(wxThreadEvent& event);
	void OnThreadCompletion(wxThreadEvent& event);


  protected:
    //const bool &m_bshutdown;
    RenderThread *m_pThread;
	wxCriticalSection m_pThreadCS;



private:
	// Handle to Viewmnager
	ViewManager			*m_pVMngr;
	// Handle to model controler
	ModelCtrl			*m_pMCtrl;
	/// Status bar
	wxStatusBar			*m_status;
	// the search tool, initially NULL
	//wxToolBarToolBase	*m_searchTool;
	/// OpenGL context
	wxRenderCanvas		*m_canvas;
	#ifndef _USE_FV_LIB
	wxFileHistory		*m_fhistory;
	#endif
private:
	/// Keyboard service
	void OnChar(wxKeyEvent& event);
	// Menu File
	#ifndef _USE_FV_LIB
	void OnMenuFileOpen(wxCommandEvent& event);
	#endif
	void OnMenuFileRefresh(wxCommandEvent& event);
	void OnMenuFileReload(wxCommandEvent& event);
	void OnMenuFileReset(wxCommandEvent& event);
	void OnMenuFileHistory(wxCommandEvent& event);
	void OnMenuFileQuit(wxCommandEvent& event);

	// Menu View
	void OnMenuViewProjection(wxCommandEvent& event);
	void OnMenuChangeView(wxCommandEvent& event);
	void OnMenuViewMode(wxCommandEvent& event);
	void OnMenuViewNew(wxCommandEvent& event);
	void OnMenuViewNext(wxCommandEvent& event);
	void OnMenuViewPrevious(wxCommandEvent& event);
	void OnMenuViewDumpCurr(wxCommandEvent& event);
	void OnMenuViewDumpAll(wxCommandEvent& event);

	// Menu Configuraztion
	void OnMenuConfigAxes(wxCommandEvent& event);
	void OnMenuConfigGrid(wxCommandEvent& event);
	void OnMenuConfigBkgColor(wxCommandEvent& event);
	void OnMenuConfigLight(wxCommandEvent& event);
	void OnMenuConfigReset(wxCommandEvent& event);
	void OnMenuConfigSave(wxCommandEvent& event);
	void OnMenuConfigLegendEdit(wxCommandEvent& event);
	void OnMenuConfigModuleApprox(wxCommandEvent& event);
	
	// Menu Render 
	void OnMenuRenderSolutionSettings(wxCommandEvent& event);
	void OnMenuRenderDraw(wxCommandEvent& event);
	void OnMenuRenderDumpScreen(wxCommandEvent& event);
	void OnMenuRenderSettings(wxCommandEvent& event);
	void OnMenuRenderCutPlaneParams(wxCommandEvent& event);

	// Menu Help
	void OnMenuHelpAbout(wxCommandEvent& event);

	// Toolbar event funcrions
	void OnToolLeftClick(wxCommandEvent& event);
	//void GetFilePath(wxString& file_path, int file_type);
	void ScreenShot(const wxString& path);
	void OnIdle(wxIdleEvent &event);
	void OnCloseWindow(wxCloseEvent & event);
	//void OnResizeWindow(wxSizeEvent & event);
	// To managed with toolbar
	void CreateWndToolBar();
	void PopulateToolbar(wxToolBarBase* toolBar);

	// Set text on the frame bar
	void SetTitle(const wxString& title);
	void UpdateStatusBar();

	wxDECLARE_EVENT_TABLE();
};

} // end namespace WxGUI
} // end namespace FemViewer
#endif /* _MAIN_WX_WINDOW_H_
*/
