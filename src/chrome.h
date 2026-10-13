#pragma once
#include <wx/wx.h>
#include <functional>
#include <vector>
struct NotyPalette {
    wxColour title, surface, paper, text, muted, hover, stroke, accent;
    static NotyPalette Get(bool dark) {
        if(dark)return {{31,34,40},{39,42,49},{29,31,36},{237,239,243},{162,169,180},{52,57,65},{55,59,67},{91,181,220}};
        return {{218,232,244},{244,247,251},{253,253,254},{37,42,49},{108,116,128},{227,234,242},{228,232,237},{0,120,164}};
    }
};
struct NotyTab {wxString label;bool modified=false;};
class NotyChrome : public wxPanel {
public:
    std::function<std::vector<NotyTab>()> tabs;
    std::function<int()> selected;
    std::function<void(int)> select, close;
    std::function<void(int,int)> reorder;
    std::function<void()> add;
    std::function<void(int)> menu;
    bool dark=false;
    explicit NotyChrome(wxWindow* parent);
    bool IsInteractive(wxPoint point) const;
    int CaptionButton(wxPoint point) const;
    void Draw(wxDC& dc);
private:
    int hover_=-1,pressed_=-1,drag_=-1;
    bool dragged_=false;
    wxPoint down_;
    std::vector<std::pair<int,wxRect>> tabRects_;
    wxRect addRect_;
    std::vector<wxRect> menuRects_;
    void LayoutRects();
    int TabAt(wxPoint point) const;
    void Click(wxPoint point,bool middle=false);
#ifdef __WXMSW__
    WXLRESULT MSWWindowProc(WXUINT,WXWPARAM,WXLPARAM) override;
#endif
};
class NotyStatus : public wxPanel {
public:
    bool dark=false;
    wxString left,zoom="100%",eol="Windows (CRLF)",encoding="UTF-8";
    std::function<void(int)> click;
    explicit NotyStatus(wxWindow* parent);
    void Draw(wxDC& dc);
};
