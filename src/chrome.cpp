#include "chrome.h"
#include "ui_text.h"
#include "logo_data.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/mstream.h>
#include <algorithm>
#include <cmath>
#ifdef __WXMSW__
#include <windows.h>
#endif
namespace {
wxFont UiFont(int size=10){return wxFontInfo(size).FaceName("Segoe UI");}
wxPoint RectCenter(const wxRect& r){return {r.x+r.width/2,r.y+r.height/2};}
wxBitmap Logo(int size){wxMemoryInputStream stream(noty_logo,sizeof(noty_logo));wxImage image(stream,wxBITMAP_TYPE_PNG);return wxBitmap(image.Scale(size,size,wxIMAGE_QUALITY_HIGH));}
void X(wxDC& dc,int x,int y,int size){dc.DrawLine(x,y,x+size,y+size);dc.DrawLine(x+size,y,x,y+size);}
}
NotyChrome::NotyChrome(wxWindow* parent):wxPanel(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE){
    SetBackgroundStyle(wxBG_STYLE_PAINT);SetMinSize({-1,FromDIP(80)});SetName("Dokument-Tabs und Menüs"_n);
    Bind(wxEVT_PAINT,[this](wxPaintEvent&){wxAutoBufferedPaintDC dc(this);Draw(dc);});
    Bind(wxEVT_SIZE,[this](wxSizeEvent& e){LayoutRects();Refresh();e.Skip();});
    Bind(wxEVT_MOTION,[this](wxMouseEvent& e){
        int hit=TabAt(e.GetPosition());if(hit!=hover_){hover_=hit;Refresh();}
        if(e.Dragging()&&drag_>=0&&(std::abs(e.GetX()-down_.x)>FromDIP(4)||std::abs(e.GetY()-down_.y)>FromDIP(4)))dragged_=true;
        if(e.Dragging()&&drag_>=0&&hit>=0&&hit!=drag_&&reorder){reorder(drag_,hit);drag_=hit;Refresh();}
    });
    Bind(wxEVT_LEAVE_WINDOW,[this](wxMouseEvent&){hover_=-1;Refresh();});
    Bind(wxEVT_LEFT_DOWN,[this](wxMouseEvent& e){dragged_=false;down_=e.GetPosition();pressed_=TabAt(down_);drag_=pressed_;if(pressed_>=0&&!HasCapture())CaptureMouse();});
    Bind(wxEVT_LEFT_UP,[this](wxMouseEvent& e){if(HasCapture())ReleaseMouse();drag_=-1;if(!dragged_)Click(e.GetPosition());dragged_=false;});
    Bind(wxEVT_MOUSEWHEEL,[this](wxMouseEvent& e){auto list=tabs?tabs():std::vector<NotyTab>{};if(list.empty()||!select||!selected)return;int index=selected()+(e.GetWheelRotation()>0?-1:1);select(std::clamp(index,0,static_cast<int>(list.size())-1));});
    Bind(wxEVT_MOUSE_CAPTURE_LOST,[this](wxMouseCaptureLostEvent&){drag_=-1;});
    Bind(wxEVT_MIDDLE_UP,[this](wxMouseEvent& e){Click(e.GetPosition(),true);});
    Bind(wxEVT_RIGHT_UP,[this](wxMouseEvent& e){int i=TabAt(e.GetPosition());if(i>=0&&select)select(i);if(menu)menu(6);});
    Bind(wxEVT_LEFT_DCLICK,[this](wxMouseEvent& e){if(TabAt(e.GetPosition())<0&&e.GetY()<FromDIP(42))static_cast<wxFrame*>(GetParent())->Maximize(!static_cast<wxFrame*>(GetParent())->IsMaximized());});
}
void NotyChrome::LayoutRects(){
    tabRects_.clear();menuRects_.clear();auto list=tabs?tabs():std::vector<NotyTab>{};
    int reserved=FromDIP(190),available=std::max(FromDIP(110),GetClientSize().x-reserved-FromDIP(44));
    int width=std::clamp(available/std::max(1,static_cast<int>(list.size())),FromDIP(112),FromDIP(205));
    int count=std::max(1,available/width),active=selected?selected():0;
    int first=active>=count?active-count+1:0;
    int x=FromDIP(42);
    for(int i=first;i<static_cast<int>(list.size())&&static_cast<int>(tabRects_.size())<count;++i){tabRects_.push_back({i,{x,FromDIP(8),width-FromDIP(3),FromDIP(36)}});x+=width;}
    addRect_={x+FromDIP(4),FromDIP(8),FromDIP(32),FromDIP(32)};
    int mx=FromDIP(8);for(int w:{47,83,62}){menuRects_.push_back({mx,FromDIP(46),FromDIP(w),FromDIP(30)});mx+=FromDIP(w);}
    menuRects_.push_back({GetClientSize().x-FromDIP(42),FromDIP(46),FromDIP(34),FromDIP(30)});
}
int NotyChrome::TabAt(wxPoint p)const{for(auto& [i,r]:tabRects_)if(r.Contains(p))return i;return -1;}
int NotyChrome::CaptionButton(wxPoint p)const{
#ifdef __WXMSW__
    if(p.y>=0&&p.y<FromDIP(42)&&p.x>=GetClientSize().x-FromDIP(138))return std::clamp((p.x-(GetClientSize().x-FromDIP(138)))/FromDIP(46),0,2);
#endif
    return -1;
}
bool NotyChrome::IsInteractive(wxPoint p)const{return TabAt(p)>=0||addRect_.Contains(p)||p.y>=FromDIP(44);}
void NotyChrome::Click(wxPoint p,bool middle){
    int i=TabAt(p);if(i>=0){for(auto& [index,r]:tabRects_)if(index==i){if(middle||p.x>r.GetRight()-FromDIP(27)){if(close)close(i);}else if(select)select(i);}return;}
    if(addRect_.Contains(p)){if(add)add();return;}
    for(std::size_t m=0;m<menuRects_.size();++m)if(menuRects_[m].Contains(p)&&menu){menu(static_cast<int>(m==3?7:m));return;}
}
void NotyChrome::Draw(wxDC& dc){
    LayoutRects();auto p=NotyPalette::Get(dark);dc.SetBackground(wxBrush(p.title));dc.Clear();dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(p.surface));dc.DrawRectangle(0,FromDIP(44),GetClientSize().x,FromDIP(36));
    dc.DrawBitmap(Logo(FromDIP(20)),FromDIP(12),FromDIP(15),true);
    auto list=tabs?tabs():std::vector<NotyTab>{};int active=selected?selected():0;
    dc.SetFont(UiFont(10));dc.SetTextForeground(p.text);
    for(auto& [i,r]:tabRects_){
        dc.SetBrush(wxBrush(i==active?p.paper:(i==hover_?p.hover:p.title)));
        dc.DrawRoundedRectangle(r,FromDIP(7));if(i==active)dc.DrawRectangle(r.x,r.y+FromDIP(10),r.width,r.height-FromDIP(8));
        wxString label=list[i].label;int maxWidth=r.width-FromDIP(52);while(label.size()>3&&dc.GetTextExtent(label).x>maxWidth)label=label.Left(label.size()-2)+"…"_n;
        dc.DrawText(label,r.x+FromDIP(14),r.y+FromDIP(10));
        dc.SetPen(wxPen(p.muted,FromDIP(1)));int cx=r.GetRight()-FromDIP(18),cy=r.y+FromDIP(17);
        if(list[i].modified&&i!=hover_){dc.SetBrush(wxBrush(p.muted));dc.DrawCircle(cx,cy,FromDIP(2));}
        else if(i==active||i==hover_)X(dc,cx-FromDIP(3),cy-FromDIP(3),FromDIP(6));
        dc.SetPen(*wxTRANSPARENT_PEN);
    }
    dc.SetPen(wxPen(p.muted,FromDIP(1)));auto c=RectCenter(addRect_);dc.DrawLine(c.x-FromDIP(4),c.y,c.x+FromDIP(4),c.y);dc.DrawLine(c.x,c.y-FromDIP(4),c.x,c.y+FromDIP(4));
#ifdef __WXMSW__
    for(int b=0;b<3;++b){int cx=GetClientSize().x-FromDIP(115)+b*FromDIP(46),cy=FromDIP(19);dc.SetPen(wxPen(p.text,FromDIP(1)));
        if(b==0)dc.DrawLine(cx-FromDIP(4),cy+FromDIP(2),cx+FromDIP(4),cy+FromDIP(2));
        else if(b==1){dc.SetBrush(*wxTRANSPARENT_BRUSH);dc.DrawRectangle(cx-FromDIP(4),cy-FromDIP(4),FromDIP(8),FromDIP(8));}
        else X(dc,cx-FromDIP(4),cy-FromDIP(4),FromDIP(8));
    }
#endif
    const wxString labels[]={"Datei"_n,"Bearbeiten"_n,"Ansicht"_n,"Suchen"_n,"Werkzeuge"_n,"Hilfe"_n};
    dc.SetFont(UiFont(10));dc.SetTextForeground(p.text);for(int i=0;i<3;++i)dc.DrawText(labels[i],menuRects_[i].x+FromDIP(8),FromDIP(54));
    auto gear=RectCenter(menuRects_.back());dc.SetPen(wxPen(p.muted,FromDIP(1)));dc.SetBrush(*wxTRANSPARENT_BRUSH);dc.DrawCircle(gear,FromDIP(5));dc.DrawCircle(gear,FromDIP(2));
    for(int i=0;i<8;++i){double a=i*3.141592653589793/4;dc.DrawLine(gear.x+static_cast<int>(std::cos(a)*FromDIP(5)),gear.y+static_cast<int>(std::sin(a)*FromDIP(5)),gear.x+static_cast<int>(std::cos(a)*FromDIP(8)),gear.y+static_cast<int>(std::sin(a)*FromDIP(8)));}
}
#ifdef __WXMSW__
WXLRESULT NotyChrome::MSWWindowProc(WXUINT msg,WXWPARAM wp,WXLPARAM lp){
    if(msg==WM_NCHITTEST){POINT point{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};::ScreenToClient(reinterpret_cast<HWND>(GetHandle()),&point);wxPoint p(point.x,point.y);if(!IsInteractive(p)||CaptionButton(p)>=0||p.y<FromDIP(5))return HTTRANSPARENT;}
    return wxPanel::MSWWindowProc(msg,wp,lp);
}
#endif
NotyStatus::NotyStatus(wxWindow* parent):wxPanel(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE){
    SetMinSize({-1,FromDIP(28)});SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT,[this](wxPaintEvent&){wxAutoBufferedPaintDC dc(this);Draw(dc);});
    Bind(wxEVT_LEFT_UP,[this](wxMouseEvent& e){if(!click)return;int x=e.GetX();if(x>GetClientSize().x-FromDIP(120))click(2);else if(x>GetClientSize().x-FromDIP(260))click(1);else if(x>GetClientSize().x-FromDIP(345))click(0);});
}
void NotyStatus::Draw(wxDC& dc){auto p=NotyPalette::Get(dark);dc.SetBackground(wxBrush(p.surface));dc.Clear();dc.SetPen(wxPen(p.stroke));dc.DrawLine(0,0,GetClientSize().x,0);dc.SetFont(UiFont(9));dc.SetTextForeground(p.muted);dc.DrawText(left,FromDIP(16),FromDIP(7));int x=GetClientSize().x;dc.DrawText(encoding,x-FromDIP(105),FromDIP(7));dc.DrawText(eol,x-FromDIP(246),FromDIP(7));dc.DrawText(zoom,x-FromDIP(331),FromDIP(7));for(int offset:{120,260,345}){dc.SetPen(wxPen(p.stroke));dc.DrawLine(x-FromDIP(offset),FromDIP(7),x-FromDIP(offset),FromDIP(21));}}
