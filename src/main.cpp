#include <wx/wx.h>
#include "frame.h"
#include "ui_text.h"
class NotyApp : public wxApp {
public:
    bool OnInit() override {
        SetAppName("Noty");wxInitAllImageHandlers();
        bool portable=false,restore=true,smoke=false;wxString screenshot;
        for(int i=1;i<argc;++i){wxString argument=argv[i];if(argument=="--portable")portable=true;else if(argument=="--no-session")restore=false;else if(argument=="--smoke-test"){smoke=true;portable=true;restore=false;}else if(argument.StartsWith("--screenshot="))screenshot=argument.AfterFirst('=');}
        auto* frame=new NotyFrame(portable,restore);frame->Show();
        if(smoke){CallAfter([this,frame,screenshot]{bool ok=frame->SmokeTest(screenshot);result_=ok?0:1;frame->Destroy();});}
        else for(int i=1;i<argc;++i){wxString path=argv[i];if(!path.StartsWith("--"))frame->Open(path);}
        return true;
    }
    int OnExit() override{return result_;}
    int OnRun() override{wxApp::OnRun();return result_;}
private:int result_=0;
};
wxIMPLEMENT_APP(NotyApp);
