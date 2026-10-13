#pragma once
#include <wx/wx.h>
#include <wx/webrequest.h>
#include <functional>
class Updater : public wxEvtHandler {
public:
    Updater(wxWindow* owner,const wxString& configDir);
    ~Updater();
    std::function<void()> changed;
    void Check(bool interactive=true);
    bool Ready()const{return ready_;}
    bool Automatic()const{return automatic_;}
    void SetAutomatic(bool value);
    void Restart();
    void CancelRestart(){restart_=false;}
    bool ApplyOnExit();
private:
    wxWindow* owner_;
    wxString configDir_,stage_,suffix_,assetName_;
    wxWebRequest request_,download_;
    std::string digest_;
    bool ready_=false,automatic_=true,interactive_=false,restart_=false;
    void OnState(wxWebRequestEvent& event);
    void Error(const wxString& message);
};
