#include "wxSolCalcDialog.h"

#include "../../utils/fv_assert.h"
#include "../../utils/fv_exception.h"
#include "../../utils/fv_mathparser.h"

#include <sstream>
#include <vector>

BEGIN_EVENT_TABLE(wxSolCalcDialog, wxDialog)
//EVT_BUTTON( wxID_OK, InitConfirmDlgWX::OnOK )

EVT_BUTTON( wxID_OK, wxSolCalcDialog::OnOk )
    EVT_BUTTON( wxID_HELP, wxSolCalcDialog::OnHelp )

    EVT_SPIN        (ID_SPIN,         wxSolCalcDialog::OnSpinButtonUpdate)
    //EVT_RADIOBOX    (ID_RADIOBOX,     wxMgrViewDialogSolutionCalculate::OnRadio)
    //EVT_BUTTON      (ID_TEST_FUNC,       wxMgrViewDialogSolutionCalculate::OnFunctionTest )

    EVT_BUTTON      (ID_TEXT_RESET,       wxSolCalcDialog::OnButtonTextReset )

    EVT_BUTTON      (ID_TEXT_FUNC_TEST,       wxSolCalcDialog::OnButtonTextFuncTest )

    //OnButtonTextReset

END_EVENT_TABLE()


wxSolCalcDialog::wxSolCalcDialog
(
	int & nreq,
	int & nr_sol,
	int & curr_sol,
	const std::string& formula
)
    : wxDialog(NULL, wxID_ANY, wxT("Solution parameters"), wxDefaultPosition, 
	    wxDefaultSize, wxCAPTION | wxRESIZE_BORDER),
	  _nreq(nreq),
	  _nr_sol(nr_sol),
	  _curr_sol(curr_sol),
	  _txtStd(),
	  _formula(formula)
{

	wxASSERT(_nreq > 0);
	wxASSERT(_nr_sol > 0);
	wxASSERT(_curr_sol >= 0);
	wxASSERT(!_formula.empty());

	/*ClumpToRange(nr_sol);
	SetFunction(_formula);*/
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);



    //*****************************************************
    //      Tekst pierwszy
    //*****************************************************


    wxStaticText* descr = new wxStaticText
    (
        this,
        wxID_STATIC,
        //wxT("Wybierz skladowa wektora jako rozwiazanie lub podaj funkcje\nna podstawie vektora rozwiazan, oraz stalych\nprzy uzyciu operacji +,-,*,/"),
		wxT("Chose components of the vector as a solution or specify math expression based on solution vector and constants with operation: +,-,*,/"),
        wxDefaultPosition,
        wxSize(400, 80),
        wxALIGN_CENTRE
    );


    topsizer->Add(descr, wxSizerFlags().Align(wxALIGN_CENTER).Border(wxALL & ~wxBOTTOM, 5));


    //*****************************************************
    //      Skladowa rozwiazania
    //*****************************************************


    wxBoxSizer *vectorValue = new wxStaticBoxSizer
    (
        new wxStaticBox(this, wxID_ANY, _T("Solution component") ),
        wxHORIZONTAL
    );

    topsizer->Add
    (
        vectorValue,
        wxSizerFlags().Align(wxALIGN_CENTER).Border(wxALL & ~wxBOTTOM, 5)
    );

    //dodanie możliwośći wyboru z zakresu
    //ID_SPIN
    //m_spintext = new wxTextCtrl( this, wxID_ANY, _T("0"), wxPoint(20,160), wxSize(80,wxDefaultCoord) );
    _spintext = new wxTextCtrl( this, wxID_ANY, _T("0") );

    //m_spinbutton = new wxSpinButton( this, ID_SPIN, wxPoint(103,160) );
    _spinbutton = new wxSpinButton( this, ID_SPIN );

    if(_nr_sol < 1)
		_spinbutton->SetRange(0,0);
    else
        _spinbutton->SetRange(0,_nr_sol-1);

	_spinbutton->SetValue(_curr_sol);

    wxString s2;

	s2 << wxT("Number of solutions: ");
    s2 << _nr_sol;

    wxStaticText* m_spinInfo = new wxStaticText ( this, wxID_STATIC, s2, wxDefaultPosition, wxDefaultSize, 0 );

    vectorValue->Add
    (
        m_spinInfo,
        wxSizerFlags().Align( wxALIGN_LEFT).Border( wxALL, 5 )
    );

    vectorValue->Add(_spintext, 0, wxALIGN_LEFT|wxALL, 5);
    vectorValue->Add(_spinbutton, 0, wxALIGN_LEFT|wxALL, 5);


    //*****************************************************
    //      Tekst drugi
    //*****************************************************


    wxBoxSizer* vectorBox = new wxBoxSizer(wxHORIZONTAL);

    //topsizer->Add(vectorBox, 0, wxGROW|wxALL, 5);

    topsizer->Add
    (
        vectorBox,
        wxSizerFlags().Align(wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP ).Border(wxALL & ~wxBOTTOM, 5)
    );




    wxStaticText* vectorA = new wxStaticText ( this, wxID_STATIC, wxT("Number of components in solution -  "), wxDefaultPosition, wxDefaultSize, 0 );
    vectorBox->Add(vectorA, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxString sNreq;
    sNreq << _nreq + wxT(" : ");

    //jesli rozmiar:
    // =0 - wypisanie komunikatu o braku skaldowych zdefiniowanych
    // >0 && <4 - wypisanie: v0, v1, v2
    // >=4 - wyposanie v0, v1...v4-1
    if ( _nreq == 0 )
    {
        sNreq << wxT("none");
    }
    else if (_nreq == 1)
    {
        sNreq << wxT("v0");
    }
    else if (_nreq == 2)
    {
        sNreq << wxT("v0, v1");
    }
    else if (_nreq == 3)
    {
        sNreq << wxT("v0, v1, v2");
    }
    else if (_nreq > 3)
    {
        sNreq << wxT("v0, v1...v");
        sNreq << _nreq-1;
    }

    wxStaticText* vectorB = new wxStaticText ( this, wxID_STATIC, sNreq, wxDefaultPosition, wxDefaultSize, 0 );
    vectorBox->Add(vectorB, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);


    //*****************************************************
    //      Wartosc funkcji
    //*****************************************************

    wxStaticBoxSizer *functionValue = new wxStaticBoxSizer( wxVERTICAL, this, _T("&Function formula") );

    //topsizer->Add( functionValue, 0, wxALL, 5 );
    topsizer->Add
    (
        functionValue,
        //wxSizerFlags(1).Align(wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP ).Border(wxALL & ~wxBOTTOM, 5)
        wxSizerFlags(1).Expand().Border(wxALL, 5)
    );

    //textctrl = new wxTextCtrl(this, -1, wxT("( ( 2 + 7 ) / 3 + ( 14 - 3 ) * 4 ) / 2"), wxPoint(-1, -1), wxSize(250, 150), wxTE_MULTILINE);
    //textctrl = new wxTextCtrl(this, -1, txtStd, wxPoint(-1, -1), wxSize(250, 150), wxTE_MULTILINE);
    //textctrl = new wxTextCtrl(this, -1, txtStd, wxDefaultPosition, wxSize(250, 150), wxTE_MULTILINE);
    _textctrl = new wxTextCtrl(this, wxID_ANY, _txtStd, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

    //functionValue->Add(textctrl, 0, wxALIGN_LEFT|wxALL, 5);
    functionValue->Add(_textctrl, wxSizerFlags(1).Expand().Border(wxALL, 5) );


    wxBoxSizer* buttonRTBox = new wxBoxSizer(wxHORIZONTAL);

    //functionValue->Add(buttonRTBox, 0, wxALIGN_LEFT|wxALL, 5);
    functionValue->Add(buttonRTBox, wxSizerFlags().Align(wxALIGN_CENTER) );

    //wxButton * textctrlReset = new wxButton();
    wxButton *textctrlReset = new wxButton(this, ID_TEXT_RESET, wxT("Reset"));
    buttonRTBox->Add(textctrlReset, 0, wxALIGN_LEFT|wxALL, 5);

    wxButton *textctrlTest = new wxButton(this, ID_TEXT_FUNC_TEST, wxT("Test"));
    buttonRTBox->Add(textctrlTest, 0, wxALIGN_LEFT|wxALL, 5);

    //*****************************************************
    //      przyciski ok, help
    //*****************************************************

    wxBoxSizer* buttonBox = new wxBoxSizer(wxHORIZONTAL);

    wxButton* ok = new wxButton ( this, wxID_OK, wxT("&OK"));
    buttonBox->Add(ok, wxSizerFlags().Border(wxALL, 7));

    wxButton* help = new wxButton( this, wxID_HELP, wxT("&Help"));
    buttonBox->Add(help, wxSizerFlags().Border(wxALL, 7));

    topsizer->Add(buttonBox, wxSizerFlags().Center());

    //*****************************************************
    //      ---
    //*****************************************************

    this->SetSizer(topsizer);

    //p->SetSizer(topsizer);
    topsizer->SetSizeHints(this);

	ClumpToRange(nr_sol);
	SetFunction(_formula);

}
/*
//wxSolCalcDialog::wxSolCalcDialog(const wxString& title, long style, const unsigned int & nreq, const unsigned int & vSize)
wxSolCalcDialog::wxSolCalcDialog
(
const wxString& title,
const wxPoint& pos,
const wxSize& size,
long style,
const int & nr_sol_start,
const int & nr_sol,
const int & vSize
)
    : wxDialog(NULL, -1, title, pos, size, style | wxCAPTION | wxRESIZE_BORDER | wxDEFAULT_FRAME_STYLE), txtStd(wxT("v0"))
{

    //SetMinSize(wxSize(400, 500));

    //this->nreq = nreq;
    this->vSize = vSize;
    this->nr_sol_start = nr_sol_start;
    this->nr_sol = nr_sol;

    //wxPanel * p = new wxPanel(this, wxID_ANY);

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

    //wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    //topsizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* descr = new wxStaticText(
        this,
        wxID_STATIC,
        //wxT("aaaa"),
        //wxT("Wybierz skladowa wektora jako rozwi¹zanie lub podaj"),//n funkcjê na podstawie vektora rozwi¹zañ, oraz sta³ych przy/n u¿yciu operacji"),
        wxT("Wybierz skladowa wektora jako rozwiazanie lub podaj funkcje\nna podstawie vektora rozwiazan, oraz stalych\nprzy uzyciu operacji +,-,*,/"),
        //wxT("podaj rozwiazanie"),
        //wxT("Please enter your name, age and sex, and specify whether you wish to\nvote in a general election."),
        wxDefaultPosition,
        wxDefaultSize,
        //0
        wxALIGN_CENTRE
        );


    topsizer->Add(descr, wxSizerFlags(1).Align(wxALIGN_CENTER).Border(wxALL & ~wxBOTTOM, 5));
    //boxSizer->Add(descr, 0, wxALIGN_LEFT|wxALL, 5);

        //skladowa horyzontalna : vetor, liczba sk³adowych wektora

    //dodanie opcji reste

    //wxStaticBoxSizer *vectorValue = new wxStaticBoxSizer( wxHORIZONTAL, this, _T("Skladowa rozwiazania") );
    wxBoxSizer *vectorValue = new wxStaticBoxSizer
    (
        new wxStaticBox(this, wxID_ANY, _T("Skladowa rozwiazania") ),
        wxHORIZONTAL
        //wxVERTICAL
    );

    //boxSizer->Add( vectorValue, 0, wxALL, 5 );
    topsizer->Add
    (
        vectorValue,
        wxSizerFlags(1).Expand().Border(wxALL, 10)
    );

    //dodanie możliwośći wyboru z zakresu
    //ID_SPIN
    //m_spintext = new wxTextCtrl( this, wxID_ANY, _T("0"), wxPoint(20,160), wxSize(80,wxDefaultCoord) );
    m_spintext = new wxTextCtrl( this, wxID_ANY, _T("0") );

    //m_spinbutton = new wxSpinButton( this, ID_SPIN, wxPoint(103,160) );
    m_spinbutton = new wxSpinButton( this, ID_SPIN );

    //1
    //m_spinbutton->SetRange(0,this->nreq-1);
    //2
    //if (this->nreq < 1)
    if (nr_sol < 1)
        {m_spinbutton->SetRange(0,0);}
    else
        //m_spinbutton->SetRange(0,this->nreq-1);
        m_spinbutton->SetRange(nr_sol_start,nr_sol_start + nr_sol - 1);


    m_spinbutton->SetValue(0);

    wxString s2;
    //s2 << _T("Wartosc z zakresu od 0 do ");

    s2 << wxT("Wartosc z zakresu od ");
    s2 << nr_sol_start;
    s2 << wxT(" do ");
    if (nr_sol < 1)
        {s2 << nr_sol_start;}
    else
        s2 << nr_sol_start + nr_sol - 1;


    wxStaticText* m_spinInfo = new wxStaticText ( this, wxID_STATIC, s2, wxDefaultPosition, wxDefaultSize, 0 );

    //vectorValue->Add(m_spinInfo, 0, wxALIGN_LEFT|wxALL, 5);
    vectorValue->Add
    (
        m_spinInfo,
        wxSizerFlags().Align( wxALIGN_LEFT).Border( wxALL, 5 )
    );

    vectorValue->Add(m_spintext, 0, wxALIGN_LEFT|wxALL, 5);
    vectorValue->Add(m_spinbutton, 0, wxALIGN_LEFT|wxALL, 5);

    //--

    wxBoxSizer* vectorBox = new wxBoxSizer(wxHORIZONTAL);
    topsizer->Add(vectorBox, 0, wxGROW|wxALL, 5);

    wxStaticText* vectorA = new wxStaticText ( this, wxID_STATIC, wxT("Liczba skladowych wektora rozwiazania:  n ="), wxDefaultPosition, wxDefaultSize, 0 );
    vectorBox->Add(vectorA, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxString sVSize;
    sVSize << vSize;

    sVSize << wxT(" : ");

    //jesli rozmiar:
    // =0 - wypisanie komunikatu o braku skaldowych zdefiniowanych
    // >0 && <4 - wypisanie: v0, v1, v2
    // >=4 - wyposanie v0, v1...v4-1
    if ( vSize == 0 )
    {
        sVSize << wxT("brak skladowych wektora");
    }
    else if (vSize == 1)
    {
        sVSize << wxT("v0");
    }
    else if (vSize == 2)
    {
        sVSize << wxT("v0, v1");
    }
    else if (vSize == 3)
    {
        sVSize << wxT("v0, v1, v2");
    }
    else if (vSize > 3)
    {
        sVSize << wxT("v0, v1...v");
        sVSize << vSize-1;
    }

    wxStaticText* vectorB = new wxStaticText ( this, wxID_STATIC, sVSize, wxDefaultPosition, wxDefaultSize, 0 );
    vectorBox->Add(vectorB, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    //--


    wxStaticBoxSizer *functionValue = new wxStaticBoxSizer( wxVERTICAL, this, _T("&Wartosc funkcji") );
    topsizer->Add( functionValue, 0, wxALL, 5 );

    //textctrl = new wxTextCtrl(this, -1, wxT("( ( 2 + 7 ) / 3 + ( 14 - 3 ) * 4 ) / 2"), wxPoint(-1, -1), wxSize(250, 150), wxTE_MULTILINE);
    textctrl = new wxTextCtrl(this, -1, txtStd, wxPoint(-1, -1), wxSize(250, 150), wxTE_MULTILINE);

    functionValue->Add(textctrl, 0, wxALIGN_LEFT|wxALL, 5);

    //wxButton * textctrlReset = new wxButton();
    wxButton *textctrlReset = new wxButton(this, ID_TEXT_RESET, wxT("Reset"));
    functionValue->Add(textctrlReset, 0, wxALIGN_LEFT|wxALL, 5);

    wxButton *textctrlTest = new wxButton(this, ID_TEXT_FUNC_TEST, wxT("Test"));
    functionValue->Add(textctrlTest, 0, wxALIGN_LEFT|wxALL, 5);


    wxBoxSizer* okCancelBox = new wxBoxSizer(wxHORIZONTAL);
    topsizer->Add(okCancelBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    //OK
    wxButton* ok = new wxButton ( this, wxID_OK, wxT("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    okCancelBox->Add(ok, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    //Help
    wxButton* help = new wxButton( this, wxID_HELP, wxT("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    okCancelBox->Add(help, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    //1
    this->SetSizer(topsizer);
    //2
    //p->SetSizer(topsizer);
    topsizer->SetSizeHints(this);

}
*/
void wxSolCalcDialog::OnSpinButtonUpdate( wxSpinEvent &event )
{
    wxString value;
    //value.Printf( _T("%d"), event.GetPosition() );
    value << event.GetPosition();
    _spintext->SetValue( value );

    //value.Printf( _T("Spin control range: (%d, %d), current = %d\n"),
    //             m_spinbutton->GetMin(), m_spinbutton->GetMax(),
    //             m_spinbutton->GetValue());

    //m_text->AppendText(value);
}

void wxSolCalcDialog::OnButtonTextReset( wxCommandEvent& event )
{
    _textctrl->SetValue(_txtStd);
}

void wxSolCalcDialog::OnButtonTextFuncTest( wxCommandEvent& event )
{
	
    #ifdef __WXDEBUG__
	std::cout << " func test: *" << this->GetFunction() << "*" << endl;
    #endif

    std::vector<fvmathParser::MathElement> vEl;
    std::vector<double>      vDouble;
    double rDouble;

    for(int i = 0; i < _nreq; ++i)
        vDouble.push_back( (double)i );

    try
    {
        //vSize

        std::string str = this->GetFunction();
        fvmathParser::MathCalculator::ONP(str, _nreq, vEl);

        //wykonanie analizy
        //sprawdzenie wyliczenia wyniku dla testowych
        //dla danych testowaych - dla odpowiedniej wielkosci wektora ze stworzonymi danymi
        //v0 - 0
        //v1 - 1
        //v2 - 2

        #ifdef __WXDEBUG__
		std::cout << "poprawne wykonanie analizy, test wyliczenia" << std::endl;
        #endif

		fvmathParser::MathCalculator::ONPCalculate(vEl, vDouble, rDouble);


        wxString s2;
        s2 << _T("Poprawny testu funkcji, wynik: ");
        s2 << rDouble;

        wxMessageDialog *dial = new wxMessageDialog(NULL, s2, wxT("Info"), wxOK);
        dial->ShowModal();

    }
    catch(fv_exception & ex)
    {

        #ifdef __WXDEBUG__
            cout << "Blad analizy:" << ex << endl;
        #endif

        //wxString s2 = wxString::FromAscii("another text");
			std::ostringstream os;
			os << ex;
			wxString s2 = wxString::FromAscii( os.str().c_str() );

        wxMessageDialog *dial = new wxMessageDialog(NULL, s2, wxT("Error"), wxOK);
        dial->ShowModal();
		dial->Destroy();

    }
    catch(...)
    {
        wxMessageDialog *dial = new wxMessageDialog(NULL, wxT("Nieznany blad analizy funkcji"), wxT("Error"), wxOK);
        dial->ShowModal();
		dial->Destroy();
    }

	
}

void wxSolCalcDialog::OnOk( wxCommandEvent& event )
{


    #ifdef __WXDEBUG__
        cout << "wxSolCalcDialog::OnOK" << endl;
    #endif

    try
    {

        //analiza

        //obliczenie

        //zakonczenie porawnie dialog


        #ifdef __WXDEBUG__
            cout << "zakonczenie" << endl;
        #endif

        EndModal( wxID_OK );//EndModal( wxID_CANCEL );

    }
    catch(...)
    {
        //wystapil wyjateknalezy ostrzec przez bledem i zasugerowac poprawe
        #ifdef __WXDEBUG__
            cout << "blad zakonczenia" << endl;
        #endif
    }

}

void wxSolCalcDialog::OnHelp( wxCommandEvent& event )
{

    #ifdef __WXDEBUG__
        cout << "wxSolCalcDialog::OnHelp" << endl;
    #endif

    //prezentacja krotkiej instukcj wedlug jakiej konwencji tworzyc poprawne funkcje

}

std::string wxSolCalcDialog::GetFunction(void)
{
    std::string str, strTmp;
    for(int i = 0; i < _textctrl->GetNumberOfLines(); ++i)
    {
        strTmp = _textctrl->GetLineText(i).mb_str(wxConvLibc);
        str = str + strTmp;
    }
    return str;
}

void wxSolCalcDialog::SetFunction(const std::string &str)
{
    wxString s2 = wxString::FromAscii( str.c_str() );
    _textctrl->SetValue(s2);
}



//nr_sol_start,nr_sol_start + nr_sol - 1

void wxSolCalcDialog::SetCurrSol(int val)
{

	ClumpToRange(val);

    wxString value;
    value << _curr_sol;

    _spintext->SetValue(value);
    _spinbutton->SetValue(val);

}



//***************************************************************
//  Ustawienia plaszczyzny tnacej
//***************************************************************

BEGIN_EVENT_TABLE(wxPlaneCutDialog, wxDialog)

    //EVT_BUTTON( wxID_OK, InitConfirmDlgWX::OnOK )

    EVT_BUTTON( wxID_OK, wxPlaneCutDialog::OnOk )

END_EVENT_TABLE()

wxPlaneCutDialog::wxPlaneCutDialog
(
    const wxString & title,
    const float & A,
    const float & B,
    const float & C,
    const float & D
): wxDialog(
    NULL, wxID_ANY, title,
    wxDefaultPosition,
    wxDefaultSize,
    wxCAPTION | wxRESIZE_BORDER
    ),
_A(A),  _B ( B ) , _C (C ) ,
_D (D)
{

	wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer *fgs = new wxFlexGridSizer(4, 2, 9, 25);

	wxStaticText * stA = new wxStaticText(this, wxID_ANY, wxT("a: "));
	wxStaticText * stB = new wxStaticText(this, wxID_ANY, wxT("b: "));
	wxStaticText * stC = new wxStaticText(this, wxID_ANY, wxT("c: "));
	wxStaticText * stD = new wxStaticText(this, wxID_ANY, wxT("d: "));

	//wxTextCtrl *tcA = new wxTextCtrl(this, -1);
	//wxTextCtrl *tcB = new wxTextCtrl(this, -1);
	//wxTextCtrl *tcC = new wxTextCtrl(this, -1);
	//wxTextCtrl *tcD = new wxTextCtrl(this, -1);


	wxString s;

	s.Clear();
	s << _A;

	tcA = new wxTextCtrl(this, wxID_ANY, s, wxDefaultPosition, wxDefaultSize, 0,
	wxTextValidator(
			//wxFILTER_ALPHA
			wxFILTER_NUMERIC
	)
	);

	s.Clear();
	s << _B;

	tcB = new wxTextCtrl(this, wxID_ANY, s, wxDefaultPosition, wxDefaultSize, 0,
	wxTextValidator(
			//wxFILTER_ALPHA
			wxFILTER_NUMERIC
	)
	);

	s.Clear();
	s << _C;

	tcC = new wxTextCtrl(this, wxID_ANY, s, wxDefaultPosition, wxDefaultSize, 0,
	wxTextValidator(
			//wxFILTER_ALPHA
			wxFILTER_NUMERIC
	)
	);

	s.Clear();
	s << _D;

	tcD = new wxTextCtrl(this, wxID_ANY, s, wxDefaultPosition, wxDefaultSize, 0,
	wxTextValidator(
			//wxFILTER_ALPHA
			wxFILTER_NUMERIC
	)
	);

	fgs->Add(stA, wxSizerFlags().Align(wxALIGN_CENTER) );
	fgs->Add(tcA, wxSizerFlags().Expand());

	fgs->Add(stB, wxSizerFlags().Align(wxALIGN_CENTER) );
	fgs->Add(tcB, wxSizerFlags().Expand());

	fgs->Add(stC, wxSizerFlags().Align(wxALIGN_CENTER) );
	fgs->Add(tcC, wxSizerFlags().Expand());

	fgs->Add(stD, wxSizerFlags().Align(wxALIGN_CENTER) );
	fgs->Add(tcD, wxSizerFlags().Expand());

	topsizer->Add(fgs, wxSizerFlags(1).Border(5).Align(wxALIGN_CENTER));

	//wxSizer * sizerOkHelp = CreateButtonSizer(wxOK | wxHELP);
	wxSizer * sizerOkHelp = CreateButtonSizer(wxOK | wxCANCEL);
	topsizer->Add(sizerOkHelp, wxSizerFlags().Border(5).Bottom());

	this->SetSizer(topsizer);
	topsizer->SetSizeHints(this);

}

void wxPlaneCutDialog::OnOk(wxCommandEvent& event) {

#ifdef __WXDEBUG__
	cout << "wxPlaneCutDialog::OnOK" << endl;
#endif

	wxString s;
	wxString number;
	//double value;

	try {

		/*
		 if ( ! tcA->GetValue().ToDouble( &m_A ) )
		 throw;

		 if ( ! tcB->GetValue().ToDouble( &m_B ) )
		 throw;

		 if ( ! tcC->GetValue().ToDouble( &m_C ) )
		 throw;

		 if ( ! tcD->GetValue().ToDouble( &m_D ) )
		 throw;
		 */

		/*
		 s.Clear();
		 s << tcA->GetValue();
		 s >> m_A;

		 s.Clear();
		 s << tcB->GetValue();
		 s >> m_B;

		 s.Clear();
		 s << tcC->GetValue();
		 s >> m_C;

		 s.Clear();
		 s << tcD->GetValue();
		 s >> m_D;
		 */

		number = tcA->GetValue();
		if (!number.ToDouble(&_A))
			throw -1;

		number = tcB->GetValue();
		if (!number.ToDouble(&_B))
			throw -1;

		number = tcC->GetValue();
		if (!number.ToDouble(&_C))
			throw -1;

		number = tcD->GetValue();
		if (!number.ToDouble(&_D))
			throw -1;

#ifdef __WXDEBUG__
		cout << "Odp Ok" << endl;
#endif

		EndModal(wxID_OK);//EndModal( wxID_CANCEL );

	} catch (...) {
		//wystapil wyjateknalezy ostrzec przez bledem i zasugerowac poprawe
#ifdef __WXDEBUG__
		cout << "blad zakonczenia" << endl;
#endif

		wxMessageDialog *dial =
				new wxMessageDialog(NULL,
						wxT("Błąd konwesji wartośći do double"),
						wxT("Error"), wxOK);
		dial->ShowModal();
		dial->Destroy();
	}

}


