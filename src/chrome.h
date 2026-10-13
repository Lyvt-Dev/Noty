#pragma once
#include <wx/wx.h>
#include <functional>
#include <vector>
struct NotyPalette {
    wxColour title, surface, paper, text, muted, hover, stroke, accent;
    static NotyPalette Get(bool dark) {
        if(dark)return {{25,28,34},{25,28,34},{33,37,44},{237,240,245},{157,168,184},{43,50,61},{48,55,65},{106,159,244}};
        return {{244,246,250},{244,246,250},{255,255,255},{34,42,56},{106,116,133},{231,236,245},{229,234,242},{55,106,205}};
    }
};
struct NotyTab {wxString label;bool modified=false;};
class NotyIconButton : public wxControl {
public:
    bool dark=false;
    wxString symbol;
    std::function<void()> action;
    NotyIconButton(wxWindow* parent,const wxString& icon,const wxString& label);
    void Activate();
private:
    bool hover_=false,down_=false;
};
class NotyChrome : public wxPanel {
public:
    std::function<std::vector<NotyTab>()> tabs;
    std::function<int()> selected;
    std::function<void(int)> menu;
    std::function<void()> restart;
    bool dark=false,ready=false;
    explicit NotyChrome(wxWindow* parent);
    bool IsInteractive(wxPoint point) const;
    int CaptionButton(wxPoint) const{return -1;}
    void Draw(wxDC& dc);
    void Theme(bool value);
    void UpdateReady(bool value);
    NotyIconButton *minimize=nullptr,*maximize=nullptr,*closeWindow=nullptr,*fullscreen=nullptr;
private:
    wxStaticText* title_=nullptr;
    NotyIconButton* restart_=nullptr;
    std::vector<NotyIconButton*> buttons_;
#ifdef __WXMSW__
    WXLRESULT MSWWindowProc(WXUINT,WXWPARAM,WXLPARAM) override;
#endif
};
class NotySidebar : public wxPanel {
public:
    bool dark=false;
    std::function<std::vector<NotyTab>()> notes;
    std::function<int()> selected;
    std::function<void(int)> select,close;
    std::function<void()> add;
    explicit NotySidebar(wxWindow* parent);
    void Draw(wxDC& dc);
    void Click(wxPoint p,bool middle=false);
    void Reveal(int index);
    int RowAt(wxPoint p) const;
private:
    int offset_=0,hover_=-1;
};
class NotyStatus : public wxPanel {
public:
    bool dark=false;
    wxString left,zoom="100%",eol="Windows (CRLF)",encoding="UTF-8";
    std::function<void(int)> click;
    explicit NotyStatus(wxWindow* parent);
    void Draw(wxDC& dc);
};
