#include "cApp.h"
#include <wx/file.h>
wxIMPLEMENT_APP(cApp);
static wxString assertionReport;
cApp::cApp(){}cApp::~cApp(){}
bool cApp::OnInit(){if(argc>=3&&wxString(argv[1])=="--self-test"){assertionReport=wxString(argv[2])+".assert";wxSetAssertHandler([](const wxString&file,int line,const wxString&func,const wxString&cond,const wxString&msg){wxFile f(assertionReport,wxFile::write);f.Write(file+wxString::Format(":%d ",line)+func+" "+cond+" "+msg);std::_Exit(2);});}auto frame=new cMain();if(argc>=3&&wxString(argv[1])=="--self-test"){bool ok=frame->GetBoard()->RunSelfTests(wxString(argv[2]));frame->Destroy();std::_Exit(ok?0:1);}frame->Show();return true;}
