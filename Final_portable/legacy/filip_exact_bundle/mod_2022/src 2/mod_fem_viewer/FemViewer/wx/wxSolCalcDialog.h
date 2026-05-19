#ifndef _SOL_CALC_DIALOG_H_
#define _SOL_CALC_DIALOG_H_

#include <wx/wx.h>
#include <wx/defs.h>
#include <wx/spinbutt.h>
#include <string>

#ifdef __WXDEBUG__
    #include <iostream>
    using namespace std;
#endif

//! Okno dialogowe dajeca mozliwosc ustalenia funkcji
class wxSolCalcDialog: public wxDialog
{
	public:
		// Constructors
		wxSolCalcDialog(
			int & nreq,	     // the number of components of each solution
			int & nr_sol,		 // the number of solutions
			int & curr_sol,// the indef current solution - default: first solution
			const std::string& formula // the formula expresion
		);

        // Gets value from range [nr_sol_start, nr_sol_start + nr_sol - 1]
        // 0 - nreq-1
		int GetCurrSol(void){return _spinbutton->GetValue(); };

		// Sets values
		void SetCurrSol(int val);

		// Gets function's formula
		std::string GetFunction(void);
		
		// Updates function's description field
		void SetFunction(const std::string &str);

	private:

		// Tne number of components in solution
		int _nreq;
		// The number of solutions
		int _nr_sol;
		// The current solution's index
		int _curr_sol;

        wxTextCtrl   * _spintext;
        wxSpinButton * _spinbutton;
        wxTextCtrl   * _textctrl;

        wxString	   _txtStd;  //(wxT("v0"));
        std::string   _formula;

        void OnSpinButtonUpdate( wxSpinEvent &event );
        void OnButtonTextReset( wxCommandEvent& event );
        void OnButtonTextFuncTest( wxCommandEvent& event );

        void OnOk( wxCommandEvent& event );
        void OnHelp( wxCommandEvent& event );

        enum
        {
            ID_SPIN = 1000,
            ID_TEXT_RESET,
            ID_TEXT_FUNC_TEST
        };

        DECLARE_EVENT_TABLE()

		inline void ClumpToRange(int val)
		{
			if(val < 0) _curr_sol = 0;
			else if(val > (_nr_sol - 1)) _curr_sol = _nr_sol -1;
			else _curr_sol = val;
		}

};

//! Okno dialogowe pozwalaj�ce na mo�liwo�� modyfikacjiparamert�w p�aszczyzny
class wxPlaneCutDialog : public wxDialog
{

    public:
        wxPlaneCutDialog( const wxString & title, const float & A, const float & B, const float & C, const float & D );
        double _A, _B, _C, _D;

    protected:
        wxTextCtrl *tcA;
        wxTextCtrl *tcB;
        wxTextCtrl *tcC;
        wxTextCtrl *tcD;

        DECLARE_EVENT_TABLE()

        void OnOk( wxCommandEvent& event );

};


#endif /* _SOL_CALC_DIALOG_H_
*/

