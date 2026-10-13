#include "chrome.h"

#include "ui_text.h"

#include "logo_data.h"

#include "icon_data.h"

#include <wx/dcbuffer.h>

#include <wx/graphics.h>

#include <wx/bmpbndl.h>

#include <wx/mstream.h>

#include <algorithm>

#ifdef __WXMSW__

#include <windows.h>

#include <windowsx.h>
#undef IsMaximized

#endif

namespace {

wxFont UiFont(int size=10,bool bold=false){return wxFontInfo(size).FaceName("Segoe UI").Weight(bold?wxFONTWEIGHT_SEMIBOLD:wxFONTWEIGHT_NORMAL);}

wxBitmap Logo(int size){wxMemoryInputStream stream(noty_logo,sizeof(noty_logo));wxImage image(stream,wxBITMAP_TYPE_PNG);return wxBitmap(image.Scale(size,size,wxIMAGE_QUALITY_HIGH));}

void Icon(wxDC& dc,const wxString& name,int x,int y,int size,bool dark){

    wxString svg=wxString::FromUTF8(noty::IconSvg(name.ToStdString()));svg.Replace("#687181",dark?"#bcc6d6":"#687181");

    auto bundle=wxBitmapBundle::FromSVG(svg.ToUTF8().data(),{size,size});if(bundle.IsOk())dc.DrawBitmap(bundle.GetBitmap({size,size}),x,y,true);

}

wxString Elide(wxDC& dc,wxString text,int width){while(text.size()>1&&dc.GetTextExtent(text).x>width)text=text.Left(text.size()-2)+"…"_n;return text;}

}

NotyIconButton::NotyIconButton(wxWindow* parent,const wxString& icon,const wxString& label):wxControl(parent,wxID_ANY,wxDefaultPosition,parent->FromDIP(wxSize(36,36)),wxBORDER_NONE),symbol(icon){

    SetName(label);SetToolTip(label);SetBackgroundStyle(wxBG_STYLE_PAINT);

    Bind(wxEVT_PAINT,[this](wxPaintEvent&){wxAutoBufferedPaintDC dc(this);auto p=NotyPalette::Get(dark);dc.SetBackground(wxBrush(p.surface));dc.Clear();std::unique_ptr<wxGraphicsContext> g(wxGraphicsContext::CreateFromUnknownDC(dc));if(g&&(hover_||HasFocus())){g->SetPen(*wxTRANSPARENT_PEN);g->SetBrush(wxBrush(symbol=="close"?wxColour(219,68,78):p.hover));g->DrawRoundedRectangle(0,0,GetClientSize().x,GetClientSize().y,FromDIP(8));}int size=FromDIP(18);Icon(dc,symbol,(GetClientSize().x-size)/2,(GetClientSize().y-size)/2,size,dark);});

    Bind(wxEVT_ENTER_WINDOW,[this](wxMouseEvent&){hover_=true;Refresh();});Bind(wxEVT_LEAVE_WINDOW,[this](wxMouseEvent&){hover_=false;Refresh();});

    Bind(wxEVT_LEFT_DOWN,[this](wxMouseEvent&){down_=true;SetFocus();if(!HasCapture())CaptureMouse();});

    Bind(wxEVT_LEFT_UP,[this](wxMouseEvent& e){bool activate=down_&&GetClientRect().Contains(e.GetPosition());down_=false;if(HasCapture())ReleaseMouse();if(activate)Activate();});

    Bind(wxEVT_MOUSE_CAPTURE_LOST,[this](wxMouseCaptureLostEvent&){down_=false;});

    Bind(wxEVT_KEY_DOWN,[this](wxKeyEvent& e){if(e.GetKeyCode()==WXK_SPACE||e.GetKeyCode()==WXK_RETURN)Activate();else e.Skip();});

    Bind(wxEVT_SET_FOCUS,[this](wxFocusEvent& e){Refresh();e.Skip();});Bind(wxEVT_KILL_FOCUS,[this](wxFocusEvent& e){Refresh();e.Skip();});

}

void NotyIconButton::Activate(){if(IsEnabled()&&action)action();}

NotyChrome::NotyChrome(wxWindow* parent):wxPanel(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE|wxTAB_TRAVERSAL){

    SetBackgroundStyle(wxBG_STYLE_PAINT);SetMinSize({-1,FromDIP(64)});SetName("Noty Werkzeugleiste"_n);

    auto* row=new wxBoxSizer(wxHORIZONTAL);row->AddSpacer(FromDIP(20));auto* logo=new wxStaticBitmap(this,wxID_ANY,Logo(FromDIP(25)));row->Add(logo,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,FromDIP(10));

    title_=new wxStaticText(this,wxID_ANY,"Noty");title_->SetFont(UiFont(14,true));row->Add(title_,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,FromDIP(26));

    auto button=[&](const wxString& icon,const wxString& name,std::function<void()> action){auto* b=new NotyIconButton(this,icon,name);b->action=action;buttons_.push_back(b);row->Add(b,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,FromDIP(6));return b;};

    button("menu","Datei und Befehle"_n,[this]{if(menu)menu(0);});button("save","Speichern · Strg+S"_n,[this]{wxCommandEvent e(wxEVT_MENU,wxID_SAVE);GetParent()->GetEventHandler()->ProcessEvent(e);});

    button("search","Suchen · Strg+F"_n,[this]{if(menu)menu(3);});row->AddStretchSpacer();

    restart_=button("restart","Update bereit · Neu starten"_n,[this]{if(restart)restart();});restart_->Hide();

    button("settings","Einstellungen und Werkzeuge"_n,[this]{if(menu)menu(7);});

    fullscreen=button("fullscreen","Vollbild · F11"_n,[this]{auto* f=static_cast<wxFrame*>(GetParent());f->ShowFullScreen(!f->IsFullScreen(),wxFULLSCREEN_ALL);});

#ifdef __WXMSW__

    row->AddSpacer(FromDIP(12));minimize=button("minimize","Minimieren"_n,[this]{static_cast<wxFrame*>(GetParent())->Iconize();});

    maximize=button("maximize","Maximieren / Wiederherstellen"_n,[this]{auto* f=static_cast<wxFrame*>(GetParent());if(f->IsFullScreen())f->ShowFullScreen(false);else f->Maximize(!f->IsMaximized());});

    closeWindow=button("close","Schließen"_n,[this]{GetParent()->Close();});

#endif

    row->AddSpacer(FromDIP(10));SetSizer(row);

    Bind(wxEVT_PAINT,[this](wxPaintEvent&){wxAutoBufferedPaintDC dc(this);Draw(dc);});

}

void NotyChrome::Theme(bool value){dark=value;auto p=NotyPalette::Get(dark);SetBackgroundColour(p.surface);title_->SetForegroundColour(p.text);for(auto* b:buttons_){b->dark=dark;b->Refresh();}Refresh();}

void NotyChrome::UpdateReady(bool value){ready=value;restart_->Show(value);Layout();}

bool NotyChrome::IsInteractive(wxPoint p)const{for(auto* child:GetChildren())if(child->IsShown()&&child->GetRect().Contains(p))return true;return false;}

void NotyChrome::Draw(wxDC& dc){auto p=NotyPalette::Get(dark);dc.SetBackground(wxBrush(p.surface));dc.Clear();}

#ifdef __WXMSW__

WXLRESULT NotyChrome::MSWWindowProc(WXUINT msg,WXWPARAM wp,WXLPARAM lp){if(msg==WM_NCHITTEST){POINT point{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};::ScreenToClient(reinterpret_cast<HWND>(GetHandle()),&point);if(!IsInteractive({point.x,point.y}))return HTTRANSPARENT;}return wxPanel::MSWWindowProc(msg,wp,lp);}

#endif

NotySidebar::NotySidebar(wxWindow* parent):wxPanel(parent,wxID_ANY,wxDefaultPosition,parent->FromDIP(wxSize(236,-1)),wxBORDER_NONE|wxWANTS_CHARS){

    SetMinSize(FromDIP(wxSize(200,-1)));SetBackgroundStyle(wxBG_STYLE_PAINT);SetName("Notizen"_n);SetToolTip("Notiz auswählen · Strg+Tab wechselt Notizen"_n);

    Bind(wxEVT_PAINT,[this](wxPaintEvent&){wxAutoBufferedPaintDC dc(this);Draw(dc);});

    Bind(wxEVT_SIZE,[this](wxSizeEvent& e){Refresh();e.Skip();});

    Bind(wxEVT_LEFT_UP,[this](wxMouseEvent& e){Click(e.GetPosition());});Bind(wxEVT_MIDDLE_UP,[this](wxMouseEvent& e){Click(e.GetPosition(),true);});

    Bind(wxEVT_MOTION,[this](wxMouseEvent& e){int i=RowAt(e.GetPosition());if(i!=hover_){hover_=i;Refresh();}});Bind(wxEVT_LEAVE_WINDOW,[this](wxMouseEvent&){hover_=-1;Refresh();});

    Bind(wxEVT_MOUSEWHEEL,[this](wxMouseEvent& e){int count=notes?static_cast<int>(notes().size()):0;int visible=std::max(1,(GetClientSize().y-FromDIP(110))/FromDIP(58));offset_=std::clamp(offset_+(e.GetWheelRotation()>0?-3:3),0,std::max(0,count-visible));Refresh();});

    Bind(wxEVT_KEY_DOWN,[this](wxKeyEvent& e){int count=notes?static_cast<int>(notes().size()):0;if(count&&selected&&select&&(e.GetKeyCode()==WXK_UP||e.GetKeyCode()==WXK_DOWN)){select(std::clamp(selected()+(e.GetKeyCode()==WXK_UP?-1:1),0,count-1));SetFocus();}else e.Skip();});

}

int NotySidebar::RowAt(wxPoint p)const{if(p.y<FromDIP(106)||p.x<FromDIP(12)||p.x>=GetClientSize().x-FromDIP(12))return -1;int i=offset_+(p.y-FromDIP(106))/FromDIP(58);return notes&&i<static_cast<int>(notes().size())?i:-1;}

void NotySidebar::Reveal(int index){int visible=std::max(1,(GetClientSize().y-FromDIP(110))/FromDIP(58));if(index<offset_)offset_=index;else if(index>=offset_+visible)offset_=index-visible+1;Refresh();}

void NotySidebar::Click(wxPoint p,bool middle){if(p.y>=FromDIP(50)&&p.y<FromDIP(90)){if(add)add();return;}int i=RowAt(p);if(i<0)return;if(middle||p.x>GetClientSize().x-FromDIP(38)){if(close)close(i);}else if(select)select(i);}

void NotySidebar::Draw(wxDC& dc){auto p=NotyPalette::Get(dark);dc.SetBackground(wxBrush(p.surface));dc.Clear();std::unique_ptr<wxGraphicsContext> g(wxGraphicsContext::CreateFromUnknownDC(dc));if(!g)return;

    dc.SetFont(UiFont(10,true));dc.SetTextForeground(p.muted);dc.DrawText("NOTIZEN"_n,FromDIP(22),FromDIP(18));

    g->SetPen(*wxTRANSPARENT_PEN);g->SetBrush(wxBrush(dark?wxColour(41,55,80):wxColour(228,236,251)));g->DrawRoundedRectangle(FromDIP(14),FromDIP(50),GetClientSize().x-FromDIP(28),FromDIP(40),FromDIP(8));Icon(dc,"add",FromDIP(26),FromDIP(61),FromDIP(18),dark);dc.SetTextForeground(p.accent);dc.DrawText("Neue Notiz"_n,FromDIP(56),FromDIP(61));

    auto list=notes?notes():std::vector<NotyTab>{};int active=selected?selected():0;

    for(int i=offset_,y=FromDIP(106);i<static_cast<int>(list.size())&&y+FromDIP(52)<=GetClientSize().y;i++,y+=FromDIP(58)){

        wxRect r(FromDIP(14),y,GetClientSize().x-FromDIP(28),FromDIP(52));g->SetBrush(wxBrush(i==active?p.paper:p.hover));if(i==active||i==hover_)g->DrawRoundedRectangle(r.x,r.y,r.width,r.height,FromDIP(8));

        Icon(dc,"note",r.x+FromDIP(10),y+FromDIP(10),FromDIP(20),dark);dc.SetTextForeground(p.text);dc.SetFont(UiFont(10,i==active));dc.DrawText(Elide(dc,list[i].label,r.width-FromDIP(68)),r.x+FromDIP(40),y+FromDIP(8));dc.SetFont(UiFont(9));dc.SetTextForeground(p.muted);dc.DrawText(list[i].modified?"Ungespeichert"_n:"Notiz"_n,r.x+FromDIP(40),y+FromDIP(29));if(i==hover_)Icon(dc,"close",r.GetRight()-FromDIP(24),y+FromDIP(17),FromDIP(14),dark);

    }

}

NotyStatus::NotyStatus(wxWindow* parent):wxPanel(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE){SetMinSize({-1,FromDIP(36)});SetBackgroundStyle(wxBG_STYLE_PAINT);Bind(wxEVT_PAINT,[this](wxPaintEvent&){wxAutoBufferedPaintDC dc(this);Draw(dc);});Bind(wxEVT_LEFT_UP,[this](wxMouseEvent& e){if(!click)return;int x=e.GetX();if(x>GetClientSize().x-FromDIP(120))click(2);else if(x>GetClientSize().x-FromDIP(260))click(1);else if(x>GetClientSize().x-FromDIP(345))click(0);});}

void NotyStatus::Draw(wxDC& dc){auto p=NotyPalette::Get(dark);dc.SetBackground(wxBrush(p.surface));dc.Clear();dc.SetFont(UiFont(9));dc.SetTextForeground(p.muted);int right=GetClientSize().x-FromDIP(365);dc.DrawText(Elide(dc,left,std::max(0,right-FromDIP(28))),FromDIP(22),FromDIP(11));int x=GetClientSize().x;dc.DrawText(encoding,x-FromDIP(105),FromDIP(11));dc.DrawText(eol,x-FromDIP(246),FromDIP(11));dc.DrawText(zoom,x-FromDIP(331),FromDIP(11));}

