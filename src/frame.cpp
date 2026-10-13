#include "frame.h"
#include "search.h"
#include "ui_text.h"
#include "logo_data.h"
#include <wx/filedlg.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/textdlg.h>
#include <wx/fontdlg.h>
#include <wx/choicdlg.h>
#include <wx/dirdlg.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/mstream.h>
#include <wx/config.h>
#include <wx/dir.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <set>
#include <regex>
#include <wx/regex.h>
#ifdef __WXMSW__
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#endif


namespace {
std::string Utf8(const wxString& s){return s.ToStdString(wxConvUTF8);}
wxString Wx(const std::string& s){return wxString::FromUTF8(s.data(),s.size());}
wxString EncodingName(noty::Encoding e){switch(e){case noty::Encoding::Utf8:return "UTF-8";case noty::Encoding::Utf8Bom:return "UTF-8 BOM";case noty::Encoding::Utf16LE:return "UTF-16 LE";case noty::Encoding::Utf16BE:return "UTF-16 BE";case noty::Encoding::Windows1252:return "Windows-1252";default:return "ISO-8859-1";}}
std::filesystem::path FsPath(const wxString& s){
#ifdef __WXMSW__
    return std::filesystem::path(s.ToStdWstring());
#else
    return std::filesystem::u8path(Utf8(s));
#endif
}
wxString ReadFile(const wxString& path,noty::Encoding& encoding){
    wxFile file(path);if(!file.IsOpened()||file.Length()<0||file.Length()>128*1024*1024)throw std::runtime_error("File unavailable or too large");
    std::string bytes(static_cast<std::size_t>(file.Length()),'\0');if(!bytes.empty()&&file.Read(bytes.data(),bytes.size())!=static_cast<wxFileOffset>(bytes.size()))throw std::runtime_error("Read failed");
    auto decoded=noty::Decode(bytes);encoding=decoded.encoding;return Wx(decoded.utf8);
}
int SearchTarget(NotyDocument* d,const wxString& query,int flags,std::vector<wxString>& groups){
    if(!(flags&wxSTC_FIND_REGEXP))return d->SearchInTarget(query);
    try{auto hit=noty::RegexFind(d->GetText(),query,d->GetTargetStart(),d->GetTargetEnd(),(flags&wxSTC_FIND_MATCHCASE)!=0,(flags&wxSTC_FIND_WHOLEWORD)!=0);if(!hit)return -1;groups=hit->groups;d->SetTargetStart(hit->start);d->SetTargetEnd(hit->end);return hit->start;}catch(...){return -2;}
}
void AtomicJson(const std::filesystem::path& path,const std::string& bytes){
    auto temporary=path;temporary+=".tmp";auto backup=path;backup+=".bak";
    {std::ofstream file(temporary,std::ios::binary|std::ios::trunc);file.write(bytes.data(),bytes.size());file.flush();if(!file)throw std::runtime_error("Session write");}
    std::error_code error;std::filesystem::remove(backup,error);error.clear();
    if(std::filesystem::exists(path)){std::filesystem::rename(path,backup,error);if(error)throw std::runtime_error("Session backup");}
    std::filesystem::rename(temporary,path,error);if(error){std::error_code ignored;if(std::filesystem::exists(backup))std::filesystem::rename(backup,path,ignored);throw std::runtime_error("Session replace");}
}
}
NotyDocument* NotyFrame::Current()const{return documents_.empty()?nullptr:documents_[std::clamp(current_,0,static_cast<int>(documents_.size())-1)];}
NotyFrame::NotyFrame(bool portable,bool restore):wxFrame(nullptr,wxID_ANY,"Noty",wxDefaultPosition,wxSize(1040,700)),timer_(this){
    SetMinSize(FromDIP(wxSize(600,360)));SetFont(wxFontInfo(10).FaceName("Segoe UI"));
    configDir_=portable?wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath():wxStandardPaths::Get().GetUserDataDir();
    wxFileName::Mkdir(configDir_,wxS_DIR_DEFAULT,wxPATH_MKDIR_FULL);
    wxMemoryInputStream logo(noty_logo,sizeof(noty_logo));wxIcon icon;icon.CopyFromBitmap(wxBitmap(wxImage(logo,wxBITMAP_TYPE_PNG)));SetIcon(icon);
    auto* vertical=new wxBoxSizer(wxVERTICAL);chrome_=new NotyChrome(this);vertical->Add(chrome_,0,wxEXPAND);
    find_=new wxPanel(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE);auto* searchLayout=new wxBoxSizer(wxVERTICAL);
    auto* fields=new wxBoxSizer(wxHORIZONTAL);query_=new wxTextCtrl(find_,wxID_ANY,wxEmptyString,wxDefaultPosition,FromDIP(wxSize(220,30)),wxTE_PROCESS_ENTER);
    query_->SetHint("Suchen"_n);replacement_=new wxTextCtrl(find_,wxID_ANY,wxEmptyString,wxDefaultPosition,FromDIP(wxSize(220,30)));replacement_->SetHint("Ersetzen durch"_n);
    fields->Add(query_,1,wxRIGHT,FromDIP(6));fields->Add(replacement_,1,wxRIGHT,FromDIP(6));
    auto button=[&](wxString name,std::function<void()> action){auto* b=new wxButton(find_,wxID_ANY,name,wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);fields->Add(b,0,wxRIGHT,FromDIP(4));b->Bind(wxEVT_BUTTON,[action](wxCommandEvent&){action();});};
    button("Zurück"_n,[this]{Find(true);});button("Weiter"_n,[this]{Find();});button("Ersetzen"_n,[this]{Find(false,true);});button("Alle ersetzen"_n,[this]{Find(false,true,true);});button("×"_n,[this]{find_->Hide();Layout();if(Current())Current()->SetFocus();});
    searchLayout->Add(fields,0,wxEXPAND|wxALL,FromDIP(10));auto* options=new wxBoxSizer(wxHORIZONTAL);
    regex_=new wxCheckBox(find_,wxID_ANY,"Regex"_n);case_=new wxCheckBox(find_,wxID_ANY,"Groß-/Kleinschreibung"_n);word_=new wxCheckBox(find_,wxID_ANY,"Ganzes Wort"_n);all_=new wxCheckBox(find_,wxID_ANY,"Alle Tabs"_n);
    for(auto* box:{regex_,case_,word_,all_})options->Add(box,0,wxRIGHT,FromDIP(18));findInfo_=new wxStaticText(find_,wxID_ANY,wxEmptyString);options->Add(findInfo_,1);searchLayout->Add(options,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,FromDIP(12));find_->SetSizer(searchLayout);find_->Hide();vertical->Add(find_,0,wxEXPAND);
    query_->Bind(wxEVT_TEXT_ENTER,[this](wxCommandEvent&){Find();});
    auto* horizontal=new wxBoxSizer(wxHORIZONTAL);explorer_=new wxGenericDirCtrl(this,wxID_ANY,wxEmptyString,wxDefaultPosition,FromDIP(wxSize(210,400)),static_cast<long>(wxDIRCTRL_SHOW_FILTERS)|wxBORDER_NONE);explorer_->Hide();horizontal->Add(explorer_,0,wxEXPAND);
    explorer_->Bind(wxEVT_DIRCTRL_FILEACTIVATED,[this](wxTreeEvent&){auto path=explorer_->GetFilePath();if(!path.empty())Open(path);});
    split_=new wxSplitterWindow(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxSP_LIVE_UPDATE|wxBORDER_NONE);split_->SetMinimumPaneSize(FromDIP(160));
    host_=new wxPanel(split_,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE);host_->SetSizer(new wxBoxSizer(wxVERTICAL));split_->Initialize(host_);horizontal->Add(split_,1,wxEXPAND);vertical->Add(horizontal,1,wxEXPAND);
    results_=new wxListCtrl(this,wxID_ANY,wxDefaultPosition,FromDIP(wxSize(-1,160)),wxLC_REPORT|wxLC_SINGLE_SEL|wxBORDER_NONE);results_->InsertColumn(0,"Datei"_n,wxLIST_FORMAT_LEFT,FromDIP(250));results_->InsertColumn(1,"Zeile"_n,wxLIST_FORMAT_LEFT,FromDIP(60));results_->InsertColumn(2,"Treffer"_n,wxLIST_FORMAT_LEFT,FromDIP(600));results_->Hide();vertical->Add(results_,0,wxEXPAND);
    results_->Bind(wxEVT_LIST_ITEM_ACTIVATED,[this](wxListEvent& e){auto path=results_->GetItemText(e.GetIndex(),0);if(path=="Unbenannt"_n)return;long line=1;results_->GetItemText(e.GetIndex(),1).ToLong(&line);Open(path,static_cast<int>(line));});
    status_=new NotyStatus(this);vertical->Add(status_,0,wxEXPAND);SetSizer(vertical);
    chrome_->tabs=[this]{std::vector<NotyTab> out;for(auto* d:documents_)out.push_back({d->path.empty()?"Unbenannt"_n:wxFileName(d->path).GetFullName(),d->Modified()});return out;};chrome_->selected=[this]{return current_;};chrome_->select=[this](int i){Select(i);};chrome_->close=[this](int i){CloseTab(i);};chrome_->add=[this]{New();};chrome_->menu=[this](int i){Popup(i);};
    chrome_->reorder=[this](int from,int to){auto* d=documents_[from];documents_.erase(documents_.begin()+from);documents_.insert(documents_.begin()+to,d);Select(to);};
    status_->click=[this](int part){if(part==2)Command(ID_ENCODING);else if(part==1){wxMenu menu;menu.Append(ID_CRLF,"Windows (CRLF)");menu.Append(ID_LF,"Unix (LF)");menu.Append(ID_CR,"Mac (CR)");PopupMenu(&menu);}else Command(ID_ZOOMRESET);};
    Menus();updater_=std::make_unique<Updater>(this);Bind(wxEVT_MENU,[this](wxCommandEvent& e){Command(e.GetId());});
    std::vector<wxAcceleratorEntry> keys;auto key=[&](int modifiers,int code,int command){keys.emplace_back(modifiers,code,command);};
    for(auto [code,command]:std::vector<std::pair<int,int>>{{'N',wxID_NEW},{'O',wxID_OPEN},{'S',wxID_SAVE},{'W',wxID_CLOSE},{'F',ID_FIND},{'H',ID_REPLACE},{'G',ID_GOTO},{'Z',wxID_UNDO},{'Y',wxID_REDO},{'D',ID_DUPLICATE}})key(wxACCEL_CTRL,code,command);
    key(wxACCEL_CTRL|wxACCEL_SHIFT,'S',wxID_SAVEAS);key(wxACCEL_CTRL|wxACCEL_SHIFT,'P',ID_PALETTE);key(wxACCEL_CTRL|wxACCEL_SHIFT,'T',ID_REOPEN);key(wxACCEL_NORMAL,WXK_F3,ID_NEXT);key(wxACCEL_SHIFT,WXK_F3,ID_PREV);key(wxACCEL_CTRL,WXK_F2,ID_BOOKMARK);key(wxACCEL_NORMAL,WXK_F2,ID_BOOKNEXT);key(wxACCEL_CTRL,'Q',ID_COMMENT);key(wxACCEL_CTRL,WXK_SPACE,ID_COMPLETE);key(wxACCEL_CTRL,WXK_ADD,ID_ZOOMIN);key(wxACCEL_CTRL,WXK_SUBTRACT,ID_ZOOMOUT);SetAcceleratorTable(wxAcceleratorTable(static_cast<int>(keys.size()),keys.data()));
    Bind(wxEVT_CHAR_HOOK,[this](wxKeyEvent& e){if(e.ControlDown()&&e.GetKeyCode()==WXK_TAB){if(!documents_.empty())Select((current_+(e.ShiftDown()?static_cast<int>(documents_.size())-1:1))%documents_.size());return;}if(e.GetKeyCode()==WXK_ESCAPE&&find_->IsShown()){find_->Hide();Layout();if(Current())Current()->SetFocus();return;}e.Skip();});
    Bind(wxEVT_TIMER,[this](wxTimerEvent&){FinishOpening();if(++ticks_%10==0)Persist();if(search_.valid()&&search_.wait_for(std::chrono::seconds(0))==std::future_status::ready){try{for(auto& [path,line,text]:search_.get())Result(Wx(path),line,Wx(text));findInfo_->SetLabel("Suche abgeschlossen"_n);}catch(...){findInfo_->SetLabel("Dateisuche fehlgeschlagen"_n);}}});timer_.Start(1000);
    Bind(wxEVT_CLOSE_WINDOW,[this](wxCloseEvent& e){for(auto* d:documents_)if(!Confirm(d)){for(auto* pending:documents_)pending->discardOnClose=false;if(e.CanVeto())e.Veto();return;}closing_=true;timer_.Stop();if(persist_.valid()){persist_.wait();try{persist_.get();}catch(...){}}Persist();Destroy();});
    if(restore)Restore();if(documents_.empty())New();ApplyTheme();Centre();
#ifdef __WXMSW__
    SetWindowPos(reinterpret_cast<HWND>(GetHandle()),nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
#endif
}
void NotyFrame::New(){
    auto* d=new NotyDocument(host_);d->Hide();documents_.push_back(d);
    d->Bind(wxEVT_STC_SAVEPOINTLEFT,[this](wxStyledTextEvent&){chrome_->Refresh();});d->Bind(wxEVT_STC_SAVEPOINTREACHED,[this](wxStyledTextEvent&){chrome_->Refresh();});
    d->Bind(wxEVT_STC_UPDATEUI,[this,d](wxStyledTextEvent&){if(d!=Current())return;UpdateStatus();int pos=d->GetCurrentPos();int brace=pos>0?pos-1:0;int c=d->GetCharAt(brace);if(c=='('||c==')'||c=='['||c==']'||c=='{'||c=='}'){int other=d->BraceMatch(brace);if(other>=0)d->BraceHighlight(brace,other);else d->BraceBadLight(brace);}else d->BraceHighlight(-1,-1);});
    d->Bind(wxEVT_STC_MODIFIED,[this,d](wxStyledTextEvent& e){
        if(e.GetModificationType()&(wxSTC_MOD_INSERTTEXT|wxSTC_MOD_DELETETEXT)){try{auto count=static_cast<std::int64_t>(noty::Codepoints(Utf8(e.GetText())).size());d->characters+=((e.GetModificationType()&wxSTC_MOD_INSERTTEXT)?count:-count);d->characters=std::max<std::int64_t>(0,d->characters);}catch(...){}}
        if(recording_&&!replaying_&&d==Current()&&(e.GetModificationType()&(wxSTC_MOD_INSERTTEXT|wxSTC_MOD_DELETETEXT)))macro_.push_back({(e.GetModificationType()&wxSTC_MOD_INSERTTEXT)!=0,e.GetPosition()-macroBase_,e.GetLength(),Utf8(e.GetText())});
    });d->Theme(dark_);d->Language();Select(static_cast<int>(documents_.size())-1);
}
void NotyFrame::Select(int i){if(i<0||i>=static_cast<int>(documents_.size()))return;for(auto* d:documents_){host_->GetSizer()->Detach(d);d->Hide();}current_=i;auto* d=Current();host_->GetSizer()->Add(d,1,wxEXPAND|wxTOP|wxBOTTOM,FromDIP(10));d->Show();host_->Layout();d->SetFocus();if(second_)second_->SetDocPointer(d->GetDocPointer());chrome_->Refresh();UpdateStatus();}
void NotyFrame::ApplyTheme(){auto palette=NotyPalette::Get(dark_);SetBackgroundColour(palette.paper);host_->SetBackgroundColour(palette.paper);find_->SetBackgroundColour(palette.surface);for(auto* d:documents_)d->Theme(dark_);if(second_)second_->Theme(dark_);chrome_->dark=dark_;status_->dark=dark_;query_->SetBackgroundColour(palette.paper);query_->SetForegroundColour(palette.text);replacement_->SetBackgroundColour(palette.paper);replacement_->SetForegroundColour(palette.text);for(auto* box:{regex_,case_,word_,all_})box->SetForegroundColour(palette.text);findInfo_->SetForegroundColour(palette.muted);results_->SetBackgroundColour(palette.paper);results_->SetForegroundColour(palette.text);
#ifdef __WXMSW__
    HWND window=reinterpret_cast<HWND>(GetHandle());BOOL dark=dark_;int corners=2,material=2;DwmSetWindowAttribute(window,20,&dark,sizeof(dark));DwmSetWindowAttribute(window,33,&corners,sizeof(corners));DwmSetWindowAttribute(window,38,&material,sizeof(material));
#endif
    Refresh();}
void NotyFrame::UpdateStatus(){auto* d=Current();if(!d)return;auto count=static_cast<long long>(d->characters);status_->left=wxString::Format("Zeile %d, Spalte %d    %lld Zeichen"_n,d->GetCurrentLine()+1,d->GetColumn(d->GetCurrentPos())+1,count);if(recording_)status_->left+="    ● Aufnahme"_n;status_->zoom=wxString::Format("%d%%",100+d->GetZoom()*10);status_->eol=d->GetEOLMode()==wxSTC_EOL_CRLF?"Windows (CRLF)":(d->GetEOLMode()==wxSTC_EOL_LF?"Unix (LF)":"Mac (CR)");status_->encoding=EncodingName(d->encoding);status_->Refresh();SetTitle((d->path.empty()?"Unbenannt"_n:wxFileName(d->path).GetFullName())+" — Noty"_n);}
void NotyFrame::Open(const wxString& path,int line){
    for(int i=0;i<static_cast<int>(documents_.size());++i)if(!documents_[i]->path.empty()&&wxFileName(documents_[i]->path).SameAs(wxFileName(path))){Select(i);if(line>0)Current()->GotoLine(line-1);return;}
    for(auto& pending:opening_)if(pending.path==path)return;
    opening_.push_back({path,line,std::async(std::launch::async,[path]{noty::Encoding encoding;auto text=ReadFile(path,encoding);return std::make_pair(text,encoding);})});
    status_->left="Datei wird geöffnet…"_n;status_->Refresh();
}
void NotyFrame::FinishOpening(){
    for(std::size_t i=0;i<opening_.size();){auto& pending=opening_[i];if(pending.task.wait_for(std::chrono::seconds(0))!=std::future_status::ready){++i;continue;}
        try{auto [text,encoding]=pending.task.get();if(!Current()||!Current()->path.empty()||Current()->Modified()||Current()->GetTextLength()!=0)New();auto* d=Current();d->path=wxFileName(pending.path).GetFullPath();d->encoding=encoding;d->SetText(text);d->EmptyUndoBuffer();d->SetSavePoint();if(text.Contains("\r\n"))d->SetEOLMode(wxSTC_EOL_CRLF);else if(text.Contains("\n"))d->SetEOLMode(wxSTC_EOL_LF);else if(text.Contains("\r"))d->SetEOLMode(wxSTC_EOL_CR);d->Language();d->Theme(dark_);Select(current_);if(pending.line>0)d->GotoLine(pending.line-1);}
        catch(...){wxMessageBox("Die Datei konnte nicht geöffnet werden. Prüfe Zugriff, Encoding und Dateigröße (maximal 128 MiB)."_n,"Noty",wxOK|wxICON_ERROR,this);}
        opening_.erase(opening_.begin()+i);
    }
}
bool NotyFrame::Save(NotyDocument* d,bool saveAs){if(!d)return false;wxString path=d->path;if(path.empty()||saveAs){wxFileDialog dialog(this,"Speichern unter"_n,wxEmptyString,path,"Alle Dateien (*.*)|*.*",wxFD_SAVE|wxFD_OVERWRITE_PROMPT);if(dialog.ShowModal()!=wxID_OK)return false;path=dialog.GetPath();}try{auto bytes=noty::Encode(Utf8(d->GetText()),d->encoding);wxString temp=wxFileName::CreateTempFileName(path+".noty-");if(temp.empty())throw std::runtime_error("Temp");wxFile file(temp,wxFile::write);bool ok=file.IsOpened()&&file.Write(bytes.data(),bytes.size())==bytes.size();if(file.IsOpened())ok=file.Close()&&ok;if(ok)ok=wxRenameFile(temp,path,true);if(!ok){wxRemoveFile(temp);throw std::runtime_error("Write");}d->path=path;d->SetSavePoint();d->formatDirty=false;d->Language();chrome_->Refresh();UpdateStatus();return true;}catch(...){wxMessageBox("Speichern fehlgeschlagen. Prüfe Schreibrechte und das gewählte Encoding. Nicht darstellbare Zeichen werden niemals still ersetzt."_n,"Noty",wxOK|wxICON_ERROR,this);return false;}}
bool NotyFrame::Confirm(NotyDocument* d){if(!d||!d->Modified())return true;int result=confirmChoice_?confirmChoice_(d):wxMessageBox("Änderungen in „"_n+(d->path.empty()?"Unbenannt"_n:wxFileName(d->path).GetFullName())+"“ speichern?"_n,"Noty",wxYES_NO|wxCANCEL|wxICON_QUESTION,this);if(result==wxYES)return Save(d);if(result==wxNO){d->discardOnClose=true;return true;}return false;}
bool NotyFrame::CloseTab(int i){if(i<0||i>=static_cast<int>(documents_.size())||!Confirm(documents_[i]))return false;auto* d=documents_[i];if(!d->path.empty())recentlyClosed_.push_back(d->path);host_->GetSizer()->Detach(d);documents_.erase(documents_.begin()+i);d->Destroy();if(documents_.empty())New();else Select(std::min(i,static_cast<int>(documents_.size())-1));return true;}

void NotyFrame::Menus(){
    menus_.clear();for(int i=0;i<8;++i)menus_.push_back(std::make_unique<wxMenu>());
    auto item=[this](int menu,int id,wxString name,bool check=false){if(check)menus_[menu]->AppendCheckItem(id,name);else menus_[menu]->Append(id,name);commands_.push_back({id,name.BeforeFirst('\t')});};
    auto sep=[this](int menu){menus_[menu]->AppendSeparator();};
    item(0,wxID_NEW,"Neu\tCtrl+N"_n);item(0,wxID_OPEN,"Öffnen…\tCtrl+O"_n);item(0,wxID_SAVE,"Speichern\tCtrl+S"_n);item(0,wxID_SAVEAS,"Speichern unter…\tCtrl+Shift+S"_n);item(0,ID_ALLSAVE,"Alle speichern"_n);sep(0);item(0,wxID_CLOSE,"Tab schließen\tCtrl+W"_n);item(0,ID_CLOSEALL,"Alle Tabs schließen"_n);item(0,ID_CLOSEOTHERS,"Andere Tabs schließen"_n);item(0,ID_REOPEN,"Geschlossenen Tab öffnen\tCtrl+Shift+T"_n);sep(0);item(0,wxID_EXIT,"Beenden"_n);
    item(1,wxID_UNDO,"Rückgängig\tCtrl+Z"_n);item(1,wxID_REDO,"Wiederholen\tCtrl+Y"_n);sep(1);item(1,wxID_CUT,"Ausschneiden\tCtrl+X"_n);item(1,wxID_COPY,"Kopieren\tCtrl+C"_n);item(1,wxID_PASTE,"Einfügen\tCtrl+V"_n);item(1,wxID_SELECTALL,"Alles auswählen\tCtrl+A"_n);sep(1);item(1,ID_DUPLICATE,"Zeile duplizieren\tCtrl+D"_n);item(1,ID_DELETE,"Zeile löschen"_n);item(1,ID_COMMENT,"Kommentar umschalten\tCtrl+Q"_n);item(1,ID_UPPER,"GROSSBUCHSTABEN"_n);item(1,ID_LOWER,"kleinbuchstaben"_n);item(1,ID_TRIM,"Leerzeichen am Zeilenende entfernen"_n);item(1,ID_SORT,"Ausgewählte Zeilen sortieren"_n);item(1,ID_TABSPACE,"Tabs in Leerzeichen"_n);item(1,ID_SPACETAB,"Einrückungs-Leerzeichen in Tabs"_n);
    item(2,ID_THEME,"Dunkles Design"_n,true);item(2,ID_WRAP,"Zeilenumbruch"_n,true);item(2,ID_NUMBERS,"Zeilennummern und Lesezeichen"_n,true);item(2,ID_SPACES,"Leerzeichen anzeigen"_n,true);item(2,ID_INDENT,"Einrückungsführungen"_n,true);sep(2);item(2,ID_SPLIT,"Ansicht teilen"_n,true);item(2,ID_FOLDER,"Ordner öffnen…"_n);item(2,ID_FONT,"Schriftart…"_n);item(2,ID_ZOOMIN,"Vergrößern"_n);item(2,ID_ZOOMOUT,"Verkleinern"_n);item(2,ID_ZOOMRESET,"Zoom zurücksetzen"_n);sep(2);item(2,ID_FOLD,"Alles einklappen"_n);item(2,ID_UNFOLD,"Alles ausklappen"_n);
    item(3,ID_FIND,"Suchen…\tCtrl+F"_n);item(3,ID_REPLACE,"Ersetzen…\tCtrl+H"_n);item(3,ID_NEXT,"Weitersuchen\tF3"_n);item(3,ID_PREV,"Rückwärts suchen\tShift+F3"_n);item(3,ID_FINDALL,"Alle Treffer anzeigen"_n);item(3,ID_FIND_FILES,"In Dateien suchen…"_n);sep(3);item(3,ID_GOTO,"Gehe zu Zeile…\tCtrl+G"_n);item(3,ID_BOOKMARK,"Lesezeichen umschalten\tCtrl+F2"_n);item(3,ID_BOOKNEXT,"Nächstes Lesezeichen\tF2"_n);item(3,ID_BOOKPREV,"Vorheriges Lesezeichen"_n);
    item(4,ID_PALETTE,"Befehlspalette…\tCtrl+Shift+P"_n);item(4,ID_COMPLETE,"Wort vervollständigen\tCtrl+Space"_n);sep(4);item(4,ID_RECORD,"Makro aufnehmen / stoppen"_n,true);item(4,ID_PLAY,"Makro wiedergeben"_n);item(4,ID_MACRO_SAVE,"Makro speichern…"_n);item(4,ID_MACRO_LOAD,"Makro laden…"_n);sep(4);item(4,ID_ENCODING,"Encoding…"_n);item(4,ID_LF,"Zeilenenden → Unix (LF)"_n);item(4,ID_CRLF,"Zeilenenden → Windows (CRLF)"_n);item(4,ID_CR,"Zeilenenden → Mac (CR)"_n);item(4,ID_EXPORTHTML,"Als HTML exportieren…"_n);
    item(4,ID_LANGUAGE,"Sprache / Syntax…"_n);item(4,ID_PLUGIN,"Plugins…"_n);item(5,ID_UPDATE,"Nach Updates suchen…"_n);item(5,wxID_ABOUT,"Über Noty"_n);
    item(6,wxID_CLOSE,"Tab schließen"_n);item(6,ID_CLOSEOTHERS,"Andere Tabs schließen"_n);item(6,ID_CLOSEALL,"Alle Tabs schließen"_n);item(6,ID_REOPEN,"Geschlossenen Tab öffnen"_n);
    item(7,ID_THEME,"Dunkles Design"_n,true);item(7,ID_FONT,"Schriftart…"_n);item(7,ID_NUMBERS,"Zeilennummern"_n,true);item(7,ID_WRAP,"Zeilenumbruch"_n,true);item(7,ID_ENCODING,"Encoding…"_n);sep(7);item(7,ID_UPDATE,"Nach Updates suchen…"_n);
    menus_[7]->AppendSubMenu(menus_[3].release(),"Suchen"_n);menus_[7]->AppendSubMenu(menus_[4].release(),"Werkzeuge"_n);menus_[7]->AppendSubMenu(menus_[5].release(),"Hilfe"_n);
}
void NotyFrame::Popup(int i){if(i<0||i>=static_cast<int>(menus_.size())||!menus_[i])return;auto* d=Current();for(int m:{2,7}){menus_[m]->Check(ID_THEME,dark_);menus_[m]->Check(ID_WRAP,d&&d->GetWrapMode()!=wxSTC_WRAP_NONE);menus_[m]->Check(ID_NUMBERS,d&&d->numbers);}menus_[2]->Check(ID_SPACES,d&&d->whitespace);menus_[2]->Check(ID_SPLIT,second_!=nullptr);menus_[7]->Check(ID_RECORD,recording_);PopupMenu(menus_[i].get(),ScreenToClient(wxGetMousePosition()));}
void NotyFrame::ShowFind(bool replacing){find_->Show();replacement_->Show(replacing);Layout();query_->SetFocus();if(Current()&&!Current()->GetSelectedText().empty()&&!Current()->GetSelectedText().Contains("\n"))query_->SetValue(Current()->GetSelectedText());query_->SelectAll();}
void NotyFrame::Find(bool backwards,bool replacing,bool every){
    auto* active=Current();if(!active)return;if(query_->GetValue().empty()){ShowFind(replacing);return;}
    int flags=(case_->GetValue()?wxSTC_FIND_MATCHCASE:0)|(word_->GetValue()?wxSTC_FIND_WHOLEWORD:0)|(regex_->GetValue()?wxSTC_FIND_REGEXP|wxSTC_FIND_CXX11REGEX:0);
    int changes=0;std::vector<wxString> groups;auto list=all_->GetValue()?documents_:std::vector<NotyDocument*>{active};
    if(every){
        for(auto* d:list){int pos=0;d->SetSearchFlags(flags);d->BeginUndoAction();while(pos<=d->GetTextLength()){
            d->SetTargetStart(pos);d->SetTargetEnd(d->GetTextLength());int hit=SearchTarget(d,query_->GetValue(),flags,groups);if(hit==-2){findInfo_->SetLabel("Ungültiger regulärer Ausdruck"_n);break;}if(hit<0)break;
            int start=d->GetTargetStart(),end=d->GetTargetEnd();int length=replacing?(regex_->GetValue()?d->ReplaceTarget(noty::RegexReplacement(replacement_->GetValue(),groups)):d->ReplaceTarget(replacement_->GetValue())):end-start;
            ++changes;pos=start+length;if(end==start){int next=d->PositionAfter(pos);if(next==pos)break;pos=next;}
            if(!replacing)Result(d->path.empty()?"Unbenannt"_n:d->path,d->LineFromPosition(start)+1,d->GetLine(d->LineFromPosition(start)).Strip(wxString::both));
            if(changes>=10000){findInfo_->SetLabel("10.000 Treffer: Ergebnis begrenzt"_n);break;}
        }d->EndUndoAction();}
        findInfo_->SetLabel(wxString::Format(replacing?"%d Ersetzungen"_n:"%d Treffer"_n,changes));return;
    }
    int first=static_cast<int>(std::find(list.begin(),list.end(),active)-list.begin());
    for(int step=0;step<static_cast<int>(list.size());++step){int index=(first+step)%list.size();auto* d=list[index];d->SetSearchFlags(flags);int from=step==0?(backwards?d->GetSelectionStart():d->GetSelectionEnd()):(backwards?d->GetTextLength():0);
        if(replacing&&step==0)from=d->GetSelectionStart();d->SetTargetStart(from);d->SetTargetEnd(backwards?0:d->GetTextLength());int hit=SearchTarget(d,query_->GetValue(),flags,groups);
        if(hit==-2){findInfo_->SetLabel("Ungültiger regulärer Ausdruck"_n);return;}
        if(hit<0&&list.size()==1){d->SetTargetStart(backwards?d->GetTextLength():0);d->SetTargetEnd(from);hit=SearchTarget(d,query_->GetValue(),flags,groups);}
        if(hit>=0){int start=d->GetTargetStart(),end=d->GetTargetEnd();if(replacing){d->BeginUndoAction();int length=regex_->GetValue()?d->ReplaceTarget(noty::RegexReplacement(replacement_->GetValue(),groups)):d->ReplaceTarget(replacement_->GetValue());d->EndUndoAction();end=start+length;}
            Select(static_cast<int>(std::find(documents_.begin(),documents_.end(),d)-documents_.begin()));d->SetSelection(start,end);d->EnsureCaretVisible();findInfo_->SetLabel("Treffer gefunden"_n);return;}
    }findInfo_->SetLabel("Keine weiteren Treffer"_n);
}
void NotyFrame::Result(const wxString& path,int line,const wxString& text){if(!results_->IsShown()){results_->Show();Layout();}long i=results_->InsertItem(results_->GetItemCount(),path);results_->SetItem(i,1,wxString::Format("%d",line));results_->SetItem(i,2,text);}
void NotyFrame::FileSearch(){
    ShowFind(false);if(query_->GetValue().empty())return;if(search_.valid()&&search_.wait_for(std::chrono::seconds(0))!=std::future_status::ready){findInfo_->SetLabel("Eine Dateisuche läuft bereits"_n);return;}
    wxDirDialog dialog(this,"Ordner für die Suche"_n);if(dialog.ShowModal()!=wxID_OK)return;
    std::string pattern=Utf8(query_->GetValue());if(!regex_->GetValue()){std::string escaped;for(char c:pattern){if(std::string(".^$|()[]{}*+?\\").find(c)!=std::string::npos)escaped+='\\';escaped+=c;}pattern=escaped;}
    if(word_->GetValue())pattern="\\b(?:"+pattern+")\\b";wxRegEx expression;int regexFlags=wxRE_NEWLINE|(case_->GetValue()?0:wxRE_ICASE);
    try{if(!expression.Compile("(*UCP)"+Wx(pattern),regexFlags))throw std::runtime_error("Regex");}catch(...){findInfo_->SetLabel("Ungültiger regulärer Ausdruck"_n);return;}
    auto root=FsPath(dialog.GetPath());results_->DeleteAllItems();findInfo_->SetLabel("Dateisuche läuft…"_n);
    search_=std::async(std::launch::async,[root,pattern,regexFlags]{wxRegEx expression("(*UCP)"+Wx(pattern),regexFlags);std::vector<std::tuple<std::string,int,std::string>> out;std::error_code error;
        std::filesystem::recursive_directory_iterator it(root,std::filesystem::directory_options::skip_permission_denied,error),end;
        for(;it!=end&&out.size()<10000;it.increment(error)){if(error){error.clear();continue;}auto path=it->path();if(it->is_directory(error)){auto name=path.filename().string();if(name==".git"||name=="node_modules"||name=="build")it.disable_recursion_pending();continue;}if(!it->is_regular_file(error)||it->file_size(error)>16*1024*1024)continue;
            std::ifstream file(path,std::ios::binary);if(!file)continue;std::string bytes((std::istreambuf_iterator<char>(file)),{});if(bytes.find('\0')!=std::string::npos&&!bytes.starts_with("\xff\xfe")&&!bytes.starts_with("\xfe\xff"))continue;
            try{auto text=noty::Decode(bytes).utf8;std::istringstream lines(text);std::string line;int number=0;auto rawPath=path.u8string();std::string filename(rawPath.begin(),rawPath.end());while(std::getline(lines,line)&&out.size()<10000){++number;if(expression.Matches(Wx(line)))out.push_back({filename,number,line});}}catch(...){}
        }return out;});
}
void NotyFrame::Persist(){if(restoring_)return;if(persist_.valid()){if(persist_.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;try{persist_.get();}catch(...){status_->left="Sitzungsbackup konnte nicht geschrieben werden"_n;status_->Refresh();}}
    nlohmann::json data;data["dark"]=dark_;data["active"]=current_;data["documents"]=nlohmann::json::array();
    for(auto* d:documents_){nlohmann::json row={{"path",Utf8(d->path)},{"encoding",static_cast<int>(d->encoding)},{"position",d->GetCurrentPos()},{"numbers",d->numbers},{"wrap",d->GetWrapMode()!=wxSTC_WRAP_NONE},{"zoom",d->GetZoom()}};if(d->Modified()&&!(closing_&&d->discardOnClose)&&d->GetTextLength()<=8*1024*1024)row["backup"]=Utf8(d->GetText());data["documents"].push_back(row);}
    auto path=FsPath(configDir_+wxFILE_SEP_PATH+"session.json");auto bytes=data.dump();persist_=std::async(std::launch::async,[path,bytes]{AtomicJson(path,bytes);});
}
void NotyFrame::Restore(){restoring_=true;try{auto path=FsPath(configDir_+wxFILE_SEP_PATH+"session.json");std::ifstream file(path,std::ios::binary);if(!file){path+=".bak";file.open(path,std::ios::binary);}if(!file){restoring_=false;return;}auto data=nlohmann::json::parse(file);dark_=data.value("dark",false);for(auto& row:data.at("documents")){New();auto* d=Current();d->path=Wx(row.value("path",std::string{}));int encoding=row.value("encoding",0);d->encoding=static_cast<noty::Encoding>(std::clamp(encoding,0,5));if(row.contains("backup")){d->SetText(Wx(row.at("backup").get<std::string>()));d->formatDirty=true;}else if(!d->path.empty()){try{d->SetText(ReadFile(d->path,d->encoding));d->EmptyUndoBuffer();d->SetSavePoint();}catch(...){d->path.clear();}}
        d->numbers=row.value("numbers",false);d->UpdateMargins();d->SetWrapMode(row.value("wrap",true)?wxSTC_WRAP_WORD:wxSTC_WRAP_NONE);d->SetZoom(std::clamp(row.value("zoom",0),-5,10));d->GotoPos(std::clamp(row.value("position",0),0,d->GetTextLength()));d->Language();d->Theme(dark_);
    }Select(std::clamp(data.value("active",0),0,std::max(0,static_cast<int>(documents_.size())-1)));}catch(...){status_->left="Gesicherte Sitzung konnte nicht vollständig geladen werden"_n;}restoring_=false;}
void NotyFrame::PlayMacro(){auto* d=Current();if(!d||recording_||macro_.empty())return;int base=d->GetCurrentPos();replaying_=true;d->BeginUndoAction();for(auto& s:macro_){auto wide=static_cast<std::int64_t>(base)+s.offset;if(wide<0||wide>d->GetTextLength())break;int pos=static_cast<int>(wide);if(s.insert){d->InsertText(pos,Wx(s.text));d->GotoPos(pos+static_cast<int>(s.text.size()));}else{if(static_cast<std::int64_t>(pos)+s.length>d->GetTextLength())break;d->DeleteRange(pos,s.length);d->GotoPos(pos);}}d->EndUndoAction();replaying_=false;}

void NotyFrame::Plugins(){
    wxArrayString names;names.Add("Plugin laden…"_n);for(auto& plugin:plugins_)names.Add(wxString::FromUTF8(plugin.api->name_utf8));wxSingleChoiceDialog picker(this,"Native Noty-Plugins (ABI v1)"_n,"Plugins"_n,names);if(picker.ShowModal()!=wxID_OK)return;
    int choice=picker.GetSelection();if(choice==0){wxFileDialog file(this,"Noty-Plugin laden"_n,wxEmptyString,wxEmptyString,"Native Bibliotheken|*.dll;*.so;*.dylib",wxFD_OPEN|wxFD_FILE_MUST_EXIST);if(file.ShowModal()!=wxID_OK)return;
        if(wxMessageBox("Plugins führen nativen Code mit deinen Benutzerrechten aus. Nur ein vertrauenswürdiges Noty-Plugin laden. Notepad++-DLLs verwenden eine andere Schnittstelle."_n,"Plugin laden"_n,wxYES_NO|wxICON_WARNING,this)!=wxYES)return;
        auto library=std::make_unique<wxDynamicLibrary>();if(!library->Load(file.GetPath(),wxDL_NOW|wxDL_VERBATIM)){wxMessageBox("Bibliothek konnte nicht geladen werden."_n);return;}auto entry=reinterpret_cast<NotyPluginEntryV1>(library->GetSymbol("noty_plugin_v1"));auto* api=entry?entry():nullptr;if(!api||api->abi_version!=1||!api->name_utf8||!api->execute){wxMessageBox("Das Plugin unterstützt die Noty-ABI v1 nicht."_n);return;}plugins_.push_back({std::move(library),api});return;
    }
    auto* d=Current();if(!d)return;pluginText_=Utf8(d->GetText());NotyHostV1 host{1,this,[](void* ctx)->const char*{return static_cast<NotyFrame*>(ctx)->pluginText_.c_str();},[](void* ctx,const char* text){auto* frame=static_cast<NotyFrame*>(ctx);if(text&&frame->Current())frame->Current()->ReplaceSelection(wxString::FromUTF8(text));}};plugins_[choice-1].api->execute(&host);
}
void NotyFrame::Command(int id){
    auto* d=Current();if(!d&&id!=wxID_NEW)return;
    auto transform=[&](const std::function<wxString(wxString)>& operation){int start=d->GetSelectionStart(),end=d->GetSelectionEnd();if(start==end){start=0;end=d->GetTextLength();}auto text=operation(d->GetTextRange(start,end));d->BeginUndoAction();d->SetTargetStart(start);d->SetTargetEnd(end);int length=d->ReplaceTarget(text);d->SetSelection(start,start+length);d->EndUndoAction();};
    switch(id){
    case wxID_NEW:New();break;
    case wxID_OPEN:{wxFileDialog dialog(this,"Öffnen"_n,wxEmptyString,wxEmptyString,"Alle Dateien (*.*)|*.*",wxFD_OPEN|wxFD_MULTIPLE|wxFD_FILE_MUST_EXIST);if(dialog.ShowModal()==wxID_OK){wxArrayString paths;dialog.GetPaths(paths);for(auto& path:paths)Open(path);}break;}
    case wxID_SAVE:Save(d);break;case wxID_SAVEAS:Save(d,true);break;
    case ID_ALLSAVE:for(auto* document:documents_)if(document->Modified()&&!Save(document))break;break;
    case wxID_CLOSE:CloseTab(current_);break;
    case ID_CLOSEALL:{int count=static_cast<int>(documents_.size());for(int i=count-1;i>=0;--i)if(!CloseTab(i))break;break;}
    case ID_CLOSEOTHERS:{auto* keep=d;for(int i=static_cast<int>(documents_.size())-1;i>=0;--i)if(documents_[i]!=keep&&!CloseTab(i))break;break;}
    case ID_REOPEN:if(!recentlyClosed_.empty()){auto path=recentlyClosed_.back();recentlyClosed_.pop_back();Open(path);}break;
    case wxID_EXIT:Close();break;case wxID_UNDO:d->Undo();break;case wxID_REDO:d->Redo();break;
    case wxID_CUT:d->Cut();break;case wxID_COPY:d->Copy();break;case wxID_PASTE:d->Paste();break;case wxID_SELECTALL:d->SelectAll();break;
    case ID_FIND:ShowFind(false);break;case ID_REPLACE:ShowFind(true);break;case ID_NEXT:Find();break;case ID_PREV:Find(true);break;
    case ID_FINDALL:ShowFind(false);results_->DeleteAllItems();Find(false,false,true);break;case ID_FIND_FILES:FileSearch();break;
    case ID_GOTO:{wxTextEntryDialog dialog(this,"Zeilennummer"_n,"Gehe zu Zeile"_n);long line=0;if(dialog.ShowModal()==wxID_OK&&dialog.GetValue().ToLong(&line)&&line>0&&line<=d->GetLineCount())d->GotoLine(static_cast<int>(line-1));break;}
    case ID_THEME:dark_=!dark_;ApplyTheme();break;
    case ID_WRAP:d->SetWrapMode(d->GetWrapMode()==wxSTC_WRAP_NONE?wxSTC_WRAP_WORD:wxSTC_WRAP_NONE);break;
    case ID_NUMBERS:d->numbers=!d->numbers;d->UpdateMargins();break;
    case ID_SPACES:d->whitespace=!d->whitespace;d->SetViewWhiteSpace(d->whitespace?wxSTC_WS_VISIBLEALWAYS:wxSTC_WS_INVISIBLE);break;
    case ID_INDENT:d->SetIndentationGuides(d->GetIndentationGuides()==wxSTC_IV_NONE?wxSTC_IV_LOOKBOTH:wxSTC_IV_NONE);break;
    case ID_BOOKMARK:d->Bookmark();break;case ID_BOOKNEXT:d->NextBookmark();break;case ID_BOOKPREV:d->NextBookmark(true);break;
    case ID_COMMENT:d->Comment();break;case ID_DUPLICATE:d->LineDuplicate();break;case ID_DELETE:d->LineDelete();break;
    case ID_UPPER:transform([](wxString text){return text.MakeUpper();});break;case ID_LOWER:transform([](wxString text){return text.MakeLower();});break;
    case ID_TRIM:transform([](wxString text){wxString out;auto lines=wxSplit(text,'\n','\0');for(std::size_t i=0;i<lines.size();++i){bool cr=lines[i].EndsWith("\r");auto line=cr?lines[i].Left(lines[i].length()-1):lines[i];while(line.EndsWith(" ")||line.EndsWith("\t"))line.RemoveLast();out+=line;if(cr)out+='\r';if(i+1<lines.size())out+='\n';}return out;});break;
    case ID_SORT:transform([](wxString text){bool trailing=text.EndsWith("\n");auto lines=wxSplit(text,'\n','\0');if(trailing&&!lines.empty())lines.pop_back();std::sort(lines.begin(),lines.end());auto out=wxJoin(lines,'\n','\0');if(trailing)out+='\n';return out;});break;
    case ID_TABSPACE:transform([d](wxString text){text.Replace("\t",wxString(' ',d->GetTabWidth()));return text;});break;
    case ID_SPACETAB:d->BeginUndoAction();for(int line=0;line<d->GetLineCount();++line){int spaces=d->GetLineIndentation(line);d->SetUseTabs(true);d->SetLineIndentation(line,spaces);}d->EndUndoAction();break;
    case ID_LF:case ID_CRLF:case ID_CR:d->BeginUndoAction();d->ConvertEOLs(id==ID_LF?wxSTC_EOL_LF:(id==ID_CRLF?wxSTC_EOL_CRLF:wxSTC_EOL_CR));d->SetEOLMode(id==ID_LF?wxSTC_EOL_LF:(id==ID_CRLF?wxSTC_EOL_CRLF:wxSTC_EOL_CR));d->EndUndoAction();break;
    case ID_FOLD:d->FoldAll(wxSTC_FOLDACTION_CONTRACT);break;case ID_UNFOLD:d->FoldAll(wxSTC_FOLDACTION_EXPAND);break;
    case ID_FONT:{wxFontData data;data.SetInitialFont(d->editorFont);wxFontDialog dialog(this,data);if(dialog.ShowModal()==wxID_OK){for(auto* document:documents_){document->editorFont=dialog.GetFontData().GetChosenFont();document->Theme(dark_);}}break;}
    case ID_ZOOMIN:d->ZoomIn();break;case ID_ZOOMOUT:d->ZoomOut();break;case ID_ZOOMRESET:d->SetZoom(0);break;
    case ID_SPLIT:if(second_){split_->Unsplit(second_);second_->Destroy();second_=nullptr;}else{second_=new NotyDocument(split_);second_->SetDocPointer(d->GetDocPointer());second_->Theme(dark_);second_->SetLexer(d->GetLexer());split_->SplitVertically(host_,second_,GetClientSize().x/2);}break;
    case ID_FOLDER:{wxDirDialog dialog(this,"Projektordner öffnen"_n);if(dialog.ShowModal()==wxID_OK){explorer_->SetPath(dialog.GetPath());explorer_->Show();Layout();}break;}
    case ID_ENCODING:{wxMenu menu;menu.Append(ID_UTF8,"UTF-8");menu.Append(ID_UTF8BOM,"UTF-8 mit BOM"_n);menu.Append(ID_UTF16LE,"UTF-16 LE");menu.Append(ID_UTF16BE,"UTF-16 BE");menu.Append(ID_ANSI,"Windows-1252");menu.Append(ID_LATIN1,"ISO-8859-1");PopupMenu(&menu);break;}
    case ID_UTF8:case ID_UTF8BOM:case ID_UTF16LE:case ID_UTF16BE:case ID_ANSI:case ID_LATIN1:d->encoding=static_cast<noty::Encoding>(id-ID_UTF8);d->formatDirty=true;findInfo_->SetLabel("Encoding wird beim nächsten Speichern verwendet"_n);break;
    case ID_RECORD:if(recording_)recording_=false;else{recording_=true;macro_.clear();macroBase_=d->GetCurrentPos();}break;
    case ID_PLAY:PlayMacro();break;
    case ID_MACRO_SAVE:{wxFileDialog dialog(this,"Makro speichern"_n,wxEmptyString,"macro.json","JSON|*.json",wxFD_SAVE|wxFD_OVERWRITE_PROMPT);if(dialog.ShowModal()==wxID_OK){nlohmann::json data=nlohmann::json::array();for(auto& step:macro_)data.push_back({{"insert",step.insert},{"offset",step.offset},{"length",step.length},{"text",step.text}});try{AtomicJson(FsPath(dialog.GetPath()),data.dump(2));}catch(...){wxMessageBox("Makro konnte nicht gespeichert werden."_n);}}break;}
    case ID_MACRO_LOAD:{wxFileDialog dialog(this,"Makro laden"_n,wxEmptyString,wxEmptyString,"JSON|*.json",wxFD_OPEN|wxFD_FILE_MUST_EXIST);if(dialog.ShowModal()==wxID_OK){try{std::ifstream file(FsPath(dialog.GetPath()));auto data=nlohmann::json::parse(file);if(!data.is_array()||data.size()>10000)throw std::runtime_error("Macro");std::vector<MacroStep> steps;for(auto& step:data){auto text=step.at("text").get<std::string>();noty::Codepoints(text);int length=step.at("length").get<int>();if(length<0||text.size()>1024*1024)throw std::runtime_error("Macro");steps.push_back({step.at("insert").get<bool>(),step.at("offset").get<int>(),length,text});}macro_=std::move(steps);}catch(...){wxMessageBox("Ungültige Makrodatei."_n);}}break;}
    case ID_COMPLETE:{int pos=d->GetCurrentPos(),start=d->WordStartPosition(pos,true);std::set<std::string> words;auto text=Utf8(d->GetText());std::regex token("[A-Za-z_][A-Za-z_0-9]{2,}");for(std::sregex_iterator i(text.begin(),text.end(),token),end;i!=end&&words.size()<2000;++i)words.insert(i->str());std::string options;for(auto& word:words){if(!options.empty())options+=' ';options+=word;}if(!options.empty())d->AutoCompShow(pos-start,Wx(options));break;}
    case ID_PALETTE:{wxArrayString names;for(auto& [command,name]:commands_)names.Add(name);wxSingleChoiceDialog dialog(this,"Befehl auswählen"_n,"Befehlspalette"_n,names);if(dialog.ShowModal()==wxID_OK){int chosen=commands_[dialog.GetSelection()].first;if(chosen!=ID_PALETTE)Command(chosen);}break;}
    case ID_LANGUAGE:{
        const std::pair<wxString,int> choices[]={{"Text",wxSTC_LEX_NULL},{"C / C++ / JavaScript",wxSTC_LEX_CPP},{"Python",wxSTC_LEX_PYTHON},{"JSON",wxSTC_LEX_JSON},{"HTML / XML / PHP",wxSTC_LEX_HTML},{"CSS",wxSTC_LEX_CSS},{"Markdown",wxSTC_LEX_MARKDOWN},{"Batch",wxSTC_LEX_BATCH},{"Rust",wxSTC_LEX_RUST},{"SQL",wxSTC_LEX_SQL},{"YAML",wxSTC_LEX_YAML},{"Shell",wxSTC_LEX_BASH},{"Lua",wxSTC_LEX_LUA},{"PowerShell",wxSTC_LEX_POWERSHELL},{"Ruby",wxSTC_LEX_RUBY},{"Perl",wxSTC_LEX_PERL},{"Pascal",wxSTC_LEX_PASCAL},{"R",wxSTC_LEX_R},{"LaTeX",wxSTC_LEX_LATEX},{"Diff",wxSTC_LEX_DIFF},{"Assembly",wxSTC_LEX_ASM},{"Visual Basic",wxSTC_LEX_VB},{"CMake",wxSTC_LEX_CMAKE}};
        wxArrayString names;for(auto& choice:choices)names.Add(choice.first);wxSingleChoiceDialog dialog(this,"Syntax auswählen"_n,"Sprache"_n,names);if(dialog.ShowModal()==wxID_OK){d->SetLexer(choices[dialog.GetSelection()].second);d->Theme(dark_);d->Colourise(0,-1);}break;}
    case ID_PLUGIN:Plugins();break;
    case ID_EXPORTHTML:{wxFileDialog dialog(this,"HTML exportieren"_n,wxEmptyString,"document.html","HTML|*.html",wxFD_SAVE|wxFD_OVERWRITE_PROMPT);if(dialog.ShowModal()==wxID_OK){auto text=d->GetText();text.Replace("&","&amp;");text.Replace("<","&lt;");text.Replace(">","&gt;");wxFile file(dialog.GetPath(),wxFile::write);auto html=Utf8("<!doctype html><meta charset=\"utf-8\"><pre>"+text+"</pre>");if(!file.IsOpened()||file.Write(html.data(),html.size())!=html.size())wxMessageBox("Export fehlgeschlagen."_n);}break;}
    case ID_UPDATE:updater_->Check();break;
    case wxID_ABOUT:wxMessageBox("Noty " NOTY_VERSION "\nEin nativer Editor für Windows, macOS und Linux.\nhttps://github.com/Lyvt-Dev/Noty"_n,"Über Noty"_n,wxOK,this);break;
    }chrome_->Refresh();UpdateStatus();
}
#ifdef __WXMSW__
WXLRESULT NotyFrame::MSWWindowProc(WXUINT message,WXWPARAM wp,WXLPARAM lp){
    HWND window=reinterpret_cast<HWND>(GetHandle());
    if(message==WM_NCCALCSIZE&&wp){auto* params=reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);if(IsZoomed(window)){MONITORINFO info{sizeof(info)};GetMonitorInfo(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&info);params->rgrc[0]=info.rcWork;}return 0;}
    if(message==WM_NCHITTEST&&chrome_){POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};::ScreenToClient(window,&p);RECT area;::GetClientRect(window,&area);int edge=FromDIP(6);
        if(!IsZoomed(window)){bool left=p.x<edge,right=p.x>=area.right-edge,top=p.y<edge,bottom=p.y>=area.bottom-edge;if(top&&left)return HTTOPLEFT;if(top&&right)return HTTOPRIGHT;if(bottom&&left)return HTBOTTOMLEFT;if(bottom&&right)return HTBOTTOMRIGHT;if(top)return HTTOP;if(bottom)return HTBOTTOM;if(left)return HTLEFT;if(right)return HTRIGHT;}
        if(p.y<chrome_->FromDIP(44)){wxPoint point(p.x,p.y);int button=chrome_->CaptionButton(point);if(button==0)return HTMINBUTTON;if(button==1)return HTMAXBUTTON;if(button==2)return HTCLOSE;if(!chrome_->IsInteractive(point))return HTCAPTION;}
        return HTCLIENT;
    }
    if(message==WM_NCLBUTTONDOWN&&(wp==HTMINBUTTON||wp==HTMAXBUTTON||wp==HTCLOSE))return DefWindowProcW(window,message,wp,lp);
    if(message==WM_NCLBUTTONUP){if(wp==HTMINBUTTON){Iconize();return 0;}if(wp==HTMAXBUTTON){Maximize(!::IsZoomed(window));return 0;}if(wp==HTCLOSE){Close();return 0;}}
    return wxFrame::MSWWindowProc(message,wp,lp);
}
#endif

bool NotyFrame::SmokeTest(const wxString& screenshot){
    timer_.Stop();restoring_=true;bool ok=true;int check=0;wxString errors;auto require=[&](bool condition){++check;if(!condition){ok=false;errors+=wxString::Format(" %d",check);}};auto* d=Current();
    require(menus_[0]->FindItem(wxID_OPEN)->GetItemLabelText()=="Öffnen…"_n);
    require(menus_[1]->FindItem(wxID_UNDO)->GetItemLabelText()=="Rückgängig"_n);
    d->SetText("Grüße\nDie Straße ist größer.\n"_n);require(d->GetText()=="Grüße\nDie Straße ist größer.\n"_n);
    query_->SetValue("größer"_n);Find();require(d->GetSelectedText()=="größer"_n);
    regex_->SetValue(true);query_->SetValue("Stra.e");replacement_->SetValue("Welt");Find(false,true,true);require(d->GetText().Contains("Die Welt ist größer."_n));d->Undo();require(d->GetText().Contains("Straße"_n));regex_->SetValue(false);
    d->GotoPos(0);d->Bookmark();require((d->MarkerGet(0)&1)!=0);d->Bookmark();require((d->MarkerGet(0)&1)==0);
    Command(ID_SPLIT);require(second_&&second_->GetDocPointer()==d->GetDocPointer());if(second_)second_->AppendText("Gemeinsam"_n);require(d->GetText().EndsWith("Gemeinsam"_n));Command(ID_SPLIT);
    d->SetText("");d->GotoPos(0);Command(ID_RECORD);d->AppendText("Hallo");Command(ID_RECORD);d->SetText("");d->GotoPos(0);PlayMacro();require(d->GetText()=="Hallo");d->Undo();require(d->GetText().empty());
    d->path=configDir_+wxFILE_SEP_PATH+"smoke-grüße.txt"_n;d->SetText("Grüße 😀"_n);d->encoding=noty::Encoding::Utf16LE;require(Save(d));noty::Encoding decoded;require(ReadFile(d->path,decoded)=="Grüße 😀"_n);require(decoded==noty::Encoding::Utf16LE);wxRemoveFile(d->path);d->encoding=noty::Encoding::Utf8;
    query_->SetValue("$");regex_->SetValue(true);replacement_->SetValue("!");Find(false,true,true);require(d->GetText()=="Grüße 😀!"_n);regex_->SetValue(false);
    New();auto* unsaved=Current();unsaved->SetText("Nicht verlieren"_n);d->AppendText(" dirty");
    confirmChoice_=[d](NotyDocument* document){return document==d?wxNO:wxCANCEL;};require(!Close());require(d->Modified());require(!d->discardOnClose);
    confirmChoice_=[](NotyDocument*){return wxNO;};CloseTab(1);confirmChoice_=nullptr;Select(0);
    d->path="Noty.txt";d->SetText("Hallo!\n\nGrüße aus Noty.\n"_n);d->SetSavePoint();New();Current()->path="Notizen.txt";New();Current()->path="Ideen.txt";Select(0);dark_=false;ApplyTheme();find_->Hide();Layout();Update();
    if(!screenshot.empty()){
        wxBitmap bitmap(GetClientSize().x,GetClientSize().y,32);wxMemoryDC dc(bitmap);
#ifdef __WXMSW__
        ::ShowWindow(reinterpret_cast<HWND>(GetHandle()),SW_SHOWNOACTIVATE);::RedrawWindow(reinterpret_cast<HWND>(GetHandle()),nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
        BOOL painted=PrintWindow(reinterpret_cast<HWND>(GetHandle()),static_cast<HDC>(dc.GetHDC()),PW_CLIENTONLY|2);require(painted!=FALSE);
#else
        wxClientDC source(this);dc.Blit(0,0,GetClientSize().x,GetClientSize().y,&source,0,0);
#endif
        dc.SelectObject(wxNullBitmap);require(bitmap.SaveFile(screenshot,wxBITMAP_TYPE_PNG));
    }
    wxFile report(configDir_+wxFILE_SEP_PATH+"smoke-test.txt",wxFile::write);report.Write(ok?wxString("PASS: German UI, Unicode document, regex replacement, undo, bookmarks, split view, macro\n"):wxString("FAIL checks:")+errors+"\n");report.Close();return ok;
}