#pragma once
#include <wx/wx.h>
#include <wx/webrequest.h>
class Updater : public wxEvtHandler {
public:
    explicit Updater(wxWindow* owner);
    ~Updater();
    void Check();
private:
    wxWindow* owner_;
    wxWebRequest request_;
    wxWebRequest download_;
    std::string digest_;
    wxString suffix_;
    void OnState(wxWebRequestEvent& event);
};
