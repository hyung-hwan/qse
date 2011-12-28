#include <qse/cmn/rex.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/stdio.h>

#include <wx/wx.h>
#include <wx/cmdline.h>

#include <string.h>
#include <locale.h>

class MyApp: public wxApp
{
public:
	virtual bool OnInit();

	virtual int OnExit();
	virtual int OnRun();

	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);


	wxString matPattern;
	wxString matString;
};

DECLARE_APP(MyApp)

class MyFrame: public wxFrame
{
public:
	MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	~MyFrame ()
	{
		if (rex) qse_rex_close (rex);
	}

	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnComp(wxCommandEvent& event);

	void OnPaint (wxPaintEvent& event);

	DECLARE_EVENT_TABLE()

protected:
	qse_rex_t* rex;
	qse_rex_node_t* start;

	void drawArrow (wxDC& dc, qse_rex_node_t* f, qse_rex_node_t* t);
	void drawNode (wxDC& dc, qse_rex_node_t* n);
	void drawChain (wxDC& dc, qse_rex_node_t* n);

	int nodex, nodey;
};

enum
{
	ID_Quit = 1,
	ID_Comp,
	ID_About,
};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_MENU(ID_Quit, MyFrame::OnQuit)
	EVT_MENU(ID_Comp, MyFrame::OnComp)
	EVT_MENU(ID_About, MyFrame::OnAbout)
	EVT_PAINT(MyFrame::OnPaint)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	if (!wxApp::OnInit()) return false;

	MyFrame *frame = new MyFrame( 
		_T("\uB108 \uBB50\uAC00 \uC798\uB0AC\uC5B4?"),
		wxPoint(50,50), wxSize(450,340) 
	);

	frame->Show(TRUE);
	SetTopWindow(frame);
	return TRUE;
} 

int MyApp::OnExit()
{
	// clean up
	return 0;
}
 
int MyApp::OnRun()
{
	int exitcode = wxApp::OnRun();

	//wxTheClipboard->Flush();

	return exitcode;
}

void MyApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	static const wxCmdLineEntryDesc g_cmdLineDesc [] =
	{
		{ wxCMD_LINE_SWITCH, wxT("h"), wxT("help"),
		  wxT("displays help on the command line parameters"),
		  wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },

		//{ wxCMD_LINE_SWITCH, wxT("t"), wxT("test"),
		//  wxT("test switch"), 
		//  wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_MANDATORY  },

		{ wxCMD_LINE_PARAM,  NULL, NULL, wxT("pattern"), 
		  wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY },

		{ wxCMD_LINE_PARAM,  NULL, NULL, wxT("string"), 
		  wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY },

		{ wxCMD_LINE_NONE }
	};

	parser.SetDesc (g_cmdLineDesc);
	// must refuse '/' as parameter starter or cannot use "/path" style paths
	parser.SetSwitchChars (wxT("-"));
}
 
bool MyApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	//silent_mode = parser.Found(wxT("s"));
 
	// to get at your unnamed parameters use
	/*
	wxArrayString files;
	for (int i = 0; i < parser.GetParamCount(); i++)
	{
		files.Add(parser.GetParam(i));
	}
	*/
	if (parser.GetParamCount() != 2)
	{
		wxMessageBox(_T("Usage: XXXXXXXXXXXXXXXXXXXXXXXXXXXXx"),
			_T("Error"), wxOK | wxICON_INFORMATION, NULL);
		return false;
	}

	matPattern = parser.GetParam(0);
	matString = parser.GetParam(1);
 
	// and other command line parameters
	// then do what you need with them.
 
	return true;
}

MyFrame::MyFrame (
	const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame((wxFrame *)NULL, -1, title, pos, size), rex (NULL)
{
	wxMenu *menuFile = new wxMenu;

	menuFile->Append( ID_About, _T("&About...") );
	menuFile->Append( ID_Comp, _T("&Compile...") );
	menuFile->AppendSeparator();
	menuFile->Append( ID_Quit, _T("E&xit") );

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, _T("&File") );

	SetMenuBar( menuBar );

/*
	wxPanel *panel = new wxPanel(this, -1);
	wxFlexGridSizer* fgs = new wxFlexGridSizer (1, 2);

	wxButton* but = new wxButton (panel, wxID_ANY, _T("XXXX"));
	wxTextCtrl* textctrl = new wxTextCtrl(panel, -1, wxT(""), wxPoint(-1, -1),
     wxSize(250, 150));

	fgs->Add (but);
	fgs->Add (textctrl, 1, wxEXPAND);

	panel->SetSizer (fgs);	
*/

	CreateStatusBar();
	SetStatusText( _T("Welcome to wxWidgets!") );
	SetSize (wxSize (700,500));
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(TRUE);
}

void MyFrame::OnComp(wxCommandEvent& WXUNUSED(event))
{
	if (rex == NULL)
	{
		rex = qse_rex_open (QSE_MMGR_GETDFL(), 0, QSE_NULL);
		if (rex == NULL)
		{
			wxMessageBox(_T("Cannot open rex"),
				_T("Error"), wxOK | wxICON_INFORMATION, this);
			return;
		}
	}

	//const qse_char_t* x = QSE_T("y(abc|def|xyz|1234)x");
	//const qse_char_t* x = QSE_T("(abc|def|xyz|1234)x");
	//const qse_char_t* x = QSE_T("y(abc|def|xyz|1234");
	//const qse_char_t* x = QSE_T("(abc|abcdefg)");
	const qse_char_t* x = QSE_T("a*b?c*defg");

//((MyApp*)wxTheApp)->matPattern;
	MyApp& app = wxGetApp();

	start = qse_rex_comp (rex, app.matPattern.wx_str(), app.matPattern.Len());
	if (start == QSE_NULL)
	{
		wxMessageBox(_T("Cannot compile rex"),
			_T("Error"), wxOK | wxICON_INFORMATION, this);
		return;
	}

	//const qse_char_t* text = QSE_T("abcyabcxxx");
	const qse_char_t* text = QSE_T("abcdefg");
	qse_rex_exec (rex, 
		app.matString.wx_str(),app.matString.Len(),
		app.matString.wx_str(),app.matString.Len());

	Refresh ();
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(_T("This is a wxWidgets Hello world sample"),
		_T("\uB108 \uBB50\uAC00 \uC798\uB0AC\uC5B4?"), wxOK | wxICON_INFORMATION, this);
}

void MyFrame::drawArrow (wxDC& dc, qse_rex_node_t* f, qse_rex_node_t* t)
{
}

void MyFrame::drawNode (wxDC& dc, qse_rex_node_t* n)
{
	if (n->id == QSE_REX_NODE_BRANCH)
	{
		dc.DrawText (_T("<BR>"), nodex, nodey);
	}
	else if (n->id == QSE_REX_NODE_BOL)
	{
		dc.DrawText (_T("<^>"), nodex, nodey);
	}
	else if (n->id == QSE_REX_NODE_EOL)
	{
		dc.DrawText (_T("<$>"), nodex, nodey);
	}
	else if (n->id == QSE_REX_NODE_ANYCHAR)
	{
		dc.DrawText (_T("<AY>"), nodex, nodey);
	}
	else if (n->id == QSE_REX_NODE_CHAR)
	{
		qse_char_t x[2];

		x[0] = n->u.c;
		x[1] = QSE_T('\0');
		dc.DrawText (x, nodex, nodey);
	}
	else if (n->id == QSE_REX_NODE_START)
	{
		dc.DrawText (_T("<ST>"), nodex, nodey);
	}
	else if (n->id == QSE_REX_NODE_END)
	{
		dc.DrawText (_T("<E>"), nodex, nodey);
	}
	else if (n->id == QSE_REX_NODE_GROUP)
	{
		dc.DrawText (_T("<G>"), nodex, nodey);
	}
	else if (n->id == QSE_REX_NODE_GROUPEND)
	{
		dc.DrawText (_T("<GE>"), nodex, nodey);
	}
	else if (n->id == QSE_REX_NODE_NOP)
	{
		dc.DrawText (_T("<NP>"), nodex, nodey);
	}
}

void MyFrame::drawChain (wxDC& dc, qse_rex_node_t* n)
{
	qse_rex_node_t* t = n;

	while (t != QSE_NULL)
	{
		if (t->id == QSE_REX_NODE_BRANCH)
		{	
			drawNode (dc, t);
			nodex += 40;

			int oldx = nodex;
			drawChain (dc, t->u.b.left);

			nodex = oldx;
			nodey += 40;
			drawChain (dc, t->u.b.right);
		}
		else
		{
			drawNode (dc, t);
			nodex += 40;
		}

		if (t->id == QSE_REX_NODE_GROUP)
			t = t->u.g.head;
		else if (t->id == QSE_REX_NODE_GROUPEND)
			t = t->u.ge.group->next;
		else t = t->next;
	}
}

void MyFrame::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	//wxClientDC dc (this);

	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxGREY_BRUSH);

	// Get window dimensions
	wxSize sz = GetClientSize();

#if 0
	// Our rectangle dimensions
	wxCoord w = 100, h = 50;

	// Center the rectangle on the window, but never
	// draw at a negative position.
	int x = wxMax (0, (sz.x - w) / 2);
	int y = wxMax (0, (sz.y - h) / 2);

	wxRect rectToDraw(x, y, w, h);

	// For efficiency, do not draw if not exposed
	if (IsExposed(rectToDraw)) dc.DrawRectangle(rectToDraw);
#endif

	if (start) 
	{
		nodex = 5; nodey = 5;
		drawChain (dc, start);
	}
}



