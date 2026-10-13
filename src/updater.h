#pragma once
#include <wx/wx.h>
#include <wx/webrequest.h>
class Updater : public wxEvtHandler {
public:
    explicit Updater(wxWindow* owner);
    void Check();
private:
    wxWindow* owner_;
    wxWebRequest request_;
    void OnState(wxWebRequestEvent& event);
};
