#pragma once
#include <wx/stc/stc.h>
#include "codec.h"
#include "chrome.h"
class NotyDocument : public wxStyledTextCtrl {
public:
    wxString path;
    noty::Encoding encoding=noty::Encoding::Utf8;
    bool numbers=false, whitespace=false;
    bool formatDirty=false;
    bool discardOnClose=false;
    bool Modified()const{return GetModify()||formatDirty;}
    std::int64_t characters=0;
    wxFont editorFont;
    explicit NotyDocument(wxWindow* parent);
    void Language();
    void Theme(bool dark);
    void UpdateMargins();
    void Bookmark();
    void NextBookmark(bool reverse=false);
    void Comment();
};
