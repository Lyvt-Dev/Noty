#include <wx/wx.h>
#include <wx/aui/auibook.h>
#include <wx/stc/stc.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/textdlg.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/fontdlg.h>
#include "updater.h"
#include <memory>
#include <vector>
#ifdef __WXMSW__
#include <windows.h>
#include <dwmapi.h>
#endif
enum { ID_FIND=wxID_HIGHEST+1, ID_NEXT, ID_REPLACE, ID_THEME, ID_UPDATE, ID_GOTO,
       ID_ALLSAVE, ID_FONT, ID_LF, ID_CRLF };
class NotyDocument : public wxStyledTextCtrl {
public:
    wxString path;
    explicit NotyDocument(wxWindow* parent) : wxStyledTextCtrl(parent) {
        SetCodePage(wxSTC_CP_UTF8);
        SetMarginType(0,wxSTC_MARGIN_NUMBER); SetMarginWidth(0,48);
        SetMarginType(1,wxSTC_MARGIN_SYMBOL); SetMarginMask(1,wxSTC_MASK_FOLDERS);
        SetMarginWidth(1,16); SetMarginSensitive(1,true);
        for(int i=wxSTC_MARKNUM_FOLDEREND;i<=wxSTC_MARKNUM_FOLDEROPEN;++i)
            MarkerDefine(i,wxSTC_MARK_BOXPLUS);
        MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN,wxSTC_MARK_BOXMINUS);
        SetProperty("fold","1"); SetTabWidth(4); SetUseTabs(false);
        SetIndentationGuides(wxSTC_IV_LOOKBOTH); SetCaretLineVisible(true);
        SetWrapMode(wxSTC_WRAP_NONE);
        Bind(wxEVT_STC_MARGINCLICK,[this](wxStyledTextEvent& e){
            if(e.GetMargin()==1) ToggleFold(LineFromPosition(e.GetPosition()));
        });
        Bind(wxEVT_STC_CHARADDED,[this](wxStyledTextEvent& e){
            if(e.GetKey()=='\n') {
                int line=GetCurrentLine();
                if(line>0) {
                    SetLineIndentation(line,GetLineIndentation(line-1));
                    GotoPos(GetLineIndentPosition(line));
                }
            }
        });
    }
    void Language() {
        auto ext=wxFileName(path).GetExt().Lower();
        int lexer=wxSTC_LEX_NULL;
        if(ext=="cpp"||ext=="h"||ext=="hpp"||ext=="c"||ext=="js") lexer=wxSTC_LEX_CPP;
        else if(ext=="py") lexer=wxSTC_LEX_PYTHON;
        else if(ext=="json") lexer=wxSTC_LEX_JSON;
        else if(ext=="xml"||ext=="html"||ext=="htm") lexer=wxSTC_LEX_HTML;
        else if(ext=="css") lexer=wxSTC_LEX_CSS;
        else if(ext=="md") lexer=wxSTC_LEX_MARKDOWN;
        else if(ext=="bat"||ext=="cmd") lexer=wxSTC_LEX_BATCH;
        SetLexer(lexer);
        SetKeyWords(0,lexer==wxSTC_LEX_PYTHON ?
            "False None True and as assert async await break class continue def del elif else except finally for from global if import in is lambda nonlocal not or pass raise return try while with yield" :
            "alignas auto bool break case catch char class const constexpr continue default delete do double else enum false float for if int long namespace new nullptr private protected public return short signed sizeof static struct switch template this throw true try typedef typename union unsigned using virtual void volatile while");
        Colourise(0,-1);
    }
    void Theme(bool dark) {
        wxColour bg=dark?wxColour(24,26,31):wxColour(250,250,252);
        wxColour fg=dark?wxColour(222,225,233):wxColour(32,36,44);
        StyleSetFont(wxSTC_STYLE_DEFAULT,wxFontInfo(11).Family(wxFONTFAMILY_TELETYPE));
        StyleSetBackground(wxSTC_STYLE_DEFAULT,bg); StyleSetForeground(wxSTC_STYLE_DEFAULT,fg);
        StyleClearAll();
        StyleSetForeground(wxSTC_STYLE_LINENUMBER,dark?wxColour(135,143,158):wxColour(100,108,123));
        StyleSetBackground(wxSTC_STYLE_LINENUMBER,bg);
        SetCaretForeground(fg);
        SetCaretLineBackground(dark?wxColour(34,38,47):wxColour(235,240,248));
        SetSelBackground(true,dark?wxColour(47,73,110):wxColour(192,217,251));
        SetFoldMarginColour(true,bg); SetFoldMarginHiColour(true,bg);
        // Scintilla verwendet pro Lexer eigene Stilnummern.
        auto paint=[this](int style,wxColour colour){StyleSetForeground(style,colour);};
        wxColour green=dark?wxColour(133,186,138):wxColour(46,120,62);
        wxColour blue=dark?wxColour(127,173,255):wxColour(40,91,180);
        wxColour orange=dark?wxColour(224,178,138):wxColour(150,75,35);
        switch(GetLexer()) {
        case wxSTC_LEX_CPP:
            paint(wxSTC_C_COMMENT,green); paint(wxSTC_C_COMMENTLINE,green);
            paint(wxSTC_C_WORD,blue); paint(wxSTC_C_STRING,orange); paint(wxSTC_C_NUMBER,orange); break;
        case wxSTC_LEX_PYTHON:
            paint(wxSTC_P_COMMENTLINE,green); paint(wxSTC_P_WORD,blue);
            paint(wxSTC_P_STRING,orange); paint(wxSTC_P_CHARACTER,orange); break;
        case wxSTC_LEX_HTML:
            paint(wxSTC_H_TAG,blue); paint(wxSTC_H_COMMENT,green);
            paint(wxSTC_H_DOUBLESTRING,orange); paint(wxSTC_H_SINGLESTRING,orange); break;
        case wxSTC_LEX_JSON:
            paint(wxSTC_JSON_STRING,orange); paint(wxSTC_JSON_PROPERTYNAME,blue);
            paint(wxSTC_JSON_NUMBER,orange); break;
        case wxSTC_LEX_CSS:
            paint(wxSTC_CSS_COMMENT,green); paint(wxSTC_CSS_TAG,blue); break;
        case wxSTC_LEX_BATCH: paint(wxSTC_BAT_COMMENT,green); paint(wxSTC_BAT_WORD,blue); break;
        case wxSTC_LEX_MARKDOWN:
            paint(wxSTC_MARKDOWN_HEADER1,blue); paint(wxSTC_MARKDOWN_HEADER2,blue); break;
        }
    }
};
class NotyFrame : public wxFrame {
    wxAuiNotebook* tabs_;
    bool dark_=true;
    wxString query_;
    std::unique_ptr<Updater> updater_;
    NotyDocument* Current() const { return dynamic_cast<NotyDocument*>(tabs_->GetCurrentPage()); }
    wxString Label(NotyDocument* d) const {
        return (d->path.empty() ? wxString("Unbenannt") : wxFileName(d->path).GetFullName()) +
            (d->GetModify() ? " *" : "");
    }
    void RefreshLabel(NotyDocument* d) {
        int index=tabs_->GetPageIndex(d);
        if(index!=wxNOT_FOUND) tabs_->SetPageText(index,Label(d));
    }
    void ApplyTheme() {
        SetBackgroundColour(dark_?wxColour(24,26,31):wxColour(250,250,252));
        for(size_t i=0;i<tabs_->GetPageCount();++i)
            static_cast<NotyDocument*>(tabs_->GetPage(i))->Theme(dark_);
#ifdef __WXMSW__
        auto hwnd=reinterpret_cast<HWND>(GetHandle());
        BOOL enabled=dark_;
        DwmSetWindowAttribute(hwnd,20,&enabled,sizeof(enabled));
        int corners=2, material=2;
        DwmSetWindowAttribute(hwnd,33,&corners,sizeof(corners));
        DwmSetWindowAttribute(hwnd,38,&material,sizeof(material));
#endif
        Refresh();
    }
    void New() {
        auto* d=new NotyDocument(tabs_);
        tabs_->AddPage(d,"Unbenannt",true); d->Language(); d->Theme(dark_);
        d->Bind(wxEVT_STC_SAVEPOINTLEFT,[this,d](wxStyledTextEvent&){RefreshLabel(d);});
        d->Bind(wxEVT_STC_SAVEPOINTREACHED,[this,d](wxStyledTextEvent&){RefreshLabel(d);});
        d->Bind(wxEVT_STC_UPDATEUI,[this,d](wxStyledTextEvent&){
            if(d==Current()) SetStatusText(wxString::Format("Zeile %d  ·  Spalte %d  ·  UTF-8",
                d->GetCurrentLine()+1,d->GetColumn(d->GetCurrentPos())+1),0);
        });
        d->SetFocus();
    }
    bool Save(NotyDocument* d,bool saveAs=false) {
        wxString path=d->path;
        if(path.empty()||saveAs) {
            wxFileDialog dialog(this,"Speichern",wxEmptyString,path,"Alle Dateien (*.*)|*.*",
                wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
            if(dialog.ShowModal()!=wxID_OK) return false;
            path=dialog.GetPath();
        }
        // Temporäre Datei im Zielordner, erst nach erfolgreichem Schreiben ersetzen.
        wxString temp=wxFileName::CreateTempFileName(path+".noty-");
        if(temp.empty()) { wxMessageBox("Temporäre Datei konnte nicht erstellt werden."); return false; }
        auto bytes=d->GetText().utf8_str();
        wxFile output(temp,wxFile::write);
        bool ok=output.IsOpened() && output.Write(bytes.data(),bytes.length())==bytes.length();
        if(output.IsOpened()) ok=output.Close() && ok;
        if(ok) ok=wxRenameFile(temp,path,true);
        if(!ok) {
            wxRemoveFile(temp); wxMessageBox("Speichern fehlgeschlagen. Das Dokument bleibt geöffnet.",
                "Noty",wxOK|wxICON_ERROR,this); return false;
        }
        d->path=path; d->SetSavePoint(); d->Language(); d->Theme(dark_); RefreshLabel(d); return true;
    }
    bool Confirm(NotyDocument* d) {
        if(!d->GetModify()) return true;
        int answer=wxMessageBox("Änderungen in "+Label(d)+" speichern?","Noty",
            wxYES_NO|wxCANCEL|wxICON_QUESTION,this);
        return answer==wxNO || (answer==wxYES && Save(d));
    }
    void Find(bool prompt) {
        auto* d=Current(); if(!d) return;
        if(prompt || query_.empty()) {
            wxTextEntryDialog dialog(this,"Suchtext","Suchen",query_);
            if(dialog.ShowModal()!=wxID_OK) return; query_=dialog.GetValue();
        }
        if(query_.empty()) return;
        int start=d->GetSelectionEnd();
        d->SetSearchFlags(0); d->SetTargetStart(start); d->SetTargetEnd(d->GetTextLength());
        int pos=d->SearchInTarget(query_);
        if(pos<0) {d->SetTargetStart(0);d->SetTargetEnd(start);pos=d->SearchInTarget(query_);}
        if(pos>=0) { d->SetSelection(d->GetTargetStart(),d->GetTargetEnd()); d->EnsureCaretVisible(); }
        else SetStatusText("Suchtext nicht gefunden");
    }
public:
    NotyFrame() : wxFrame(nullptr,wxID_ANY,"Noty",wxDefaultPosition,wxSize(1100,750)) {
        wxConfig config("Noty"); config.Read("dark",&dark_,true);
        auto* file=new wxMenu;
        file->Append(wxID_NEW,"&Neu\tCtrl+N"); file->Append(wxID_OPEN,"Ö&ffnen…\tCtrl+O");
        file->Append(wxID_SAVE,"&Speichern\tCtrl+S"); file->Append(wxID_SAVEAS,"Speichern &unter…\tCtrl+Shift+S");
        file->Append(ID_ALLSAVE,"Alle speichern");file->Append(wxID_CLOSE,"Tab schließen\tCtrl+W");
        file->AppendSeparator();file->Append(wxID_EXIT,"Beenden");
        auto* edit=new wxMenu;
        edit->Append(wxID_UNDO,"Rückgängig\tCtrl+Z");edit->Append(wxID_REDO,"Wiederholen\tCtrl+Y");
        edit->AppendSeparator();edit->Append(wxID_CUT,"Ausschneiden\tCtrl+X");
        edit->Append(wxID_COPY,"Kopieren\tCtrl+C");edit->Append(wxID_PASTE,"Einfügen\tCtrl+V");
        edit->Append(wxID_SELECTALL,"Alles auswählen\tCtrl+A");
        edit->AppendSeparator();edit->Append(ID_FIND,"Suchen…\tCtrl+F");edit->Append(ID_NEXT,"Weitersuchen\tF3");
        edit->Append(ID_REPLACE,"Alle ersetzen…\tCtrl+H");edit->Append(ID_GOTO,"Gehe zu Zeile…\tCtrl+G");
        auto* view=new wxMenu;
        view->AppendCheckItem(ID_THEME,"Dunkles Editor-Theme");view->Check(ID_THEME,dark_);
        view->Append(ID_FONT,"Editor-Schrift…");view->Append(ID_LF,"Zeilenenden: LF");view->Append(ID_CRLF,"Zeilenenden: CRLF");
        auto* help=new wxMenu;help->Append(ID_UPDATE,"Nach Updates suchen…");help->Append(wxID_ABOUT,"Über Noty");
        auto* bar=new wxMenuBar;bar->Append(file,"Datei");bar->Append(edit,"Bearbeiten");bar->Append(view,"Ansicht");bar->Append(help,"Hilfe");SetMenuBar(bar);
        tabs_=new wxAuiNotebook(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,
            wxAUI_NB_DEFAULT_STYLE|wxAUI_NB_TAB_MOVE|wxAUI_NB_CLOSE_ON_ALL_TABS);
        auto* layout=new wxBoxSizer(wxVERTICAL);layout->Add(tabs_,1,wxEXPAND);SetSizer(layout);
        CreateStatusBar();updater_=std::make_unique<Updater>(this);
        Bind(wxEVT_MENU,[this](wxCommandEvent& e){
            auto* d=Current();
            switch(e.GetId()) {
            case wxID_NEW:New();break;
            case wxID_OPEN:{
                wxFileDialog dialog(this,"Öffnen",wxEmptyString,wxEmptyString,"Alle Dateien (*.*)|*.*",wxFD_OPEN|wxFD_FILE_MUST_EXIST|wxFD_MULTIPLE);
                if(dialog.ShowModal()==wxID_OK){wxArrayString paths;dialog.GetPaths(paths);for(auto& p:paths) Open(p);}break;}
            case wxID_SAVE:if(d)Save(d);break;case wxID_SAVEAS:if(d)Save(d,true);break;
            case ID_ALLSAVE:for(size_t i=0;i<tabs_->GetPageCount();++i)if(!Save(static_cast<NotyDocument*>(tabs_->GetPage(i))))break;break;
            case wxID_CLOSE:if(d&&Confirm(d))tabs_->DeletePage(tabs_->GetSelection());break;
            case wxID_EXIT:Close();break;case wxID_UNDO:if(d)d->Undo();break;case wxID_REDO:if(d)d->Redo();break;
            case wxID_CUT:if(d)d->Cut();break;case wxID_COPY:if(d)d->Copy();break;case wxID_PASTE:if(d)d->Paste();break;
            case wxID_SELECTALL:if(d)d->SelectAll();break;
            case ID_FIND:Find(true);break;case ID_NEXT:Find(false);break;
            case ID_REPLACE:{
                if(!d)break;
                wxTextEntryDialog search(this,"Suchtext (wörtlich, Groß-/Kleinschreibung beachten)","Alle ersetzen");
                if(search.ShowModal()!=wxID_OK || search.GetValue().empty())break;
                wxTextEntryDialog replacement(this,"Ersetzen durch","Alle ersetzen");
                if(replacement.ShowModal()!=wxID_OK)break;
                auto text=d->GetText();auto count=text.Replace(search.GetValue(),replacement.GetValue(),true);
                if(count){d->BeginUndoAction();d->SetTargetStart(0);d->SetTargetEnd(d->GetTextLength());d->ReplaceTarget(text);d->EndUndoAction();}
                break;}
            case ID_GOTO:{
                if(!d)break;wxTextEntryDialog dialog(this,"Zeilennummer","Gehe zu Zeile");long line;
                if(dialog.ShowModal()==wxID_OK&&dialog.GetValue().ToLong(&line)&&line>0&&line<=d->GetLineCount())d->GotoLine(static_cast<int>(line-1));break;}
            case ID_THEME:dark_=e.IsChecked();ApplyTheme();{wxConfig config("Noty");config.Write("dark",dark_);}break;
            case ID_FONT:{if(!d)break;wxFontData data;data.SetInitialFont(d->StyleGetFont(wxSTC_STYLE_DEFAULT));
                wxFontDialog dialog(this,data);if(dialog.ShowModal()==wxID_OK){for(int i=0;i<256;++i)d->StyleSetFont(i,dialog.GetFontData().GetChosenFont());}break;}
            case ID_LF:case ID_CRLF:if(d){d->ConvertEOLs(e.GetId()==ID_LF?wxSTC_EOL_LF:wxSTC_EOL_CRLF);d->SetEOLMode(e.GetId()==ID_LF?wxSTC_EOL_LF:wxSTC_EOL_CRLF);}break;
            case ID_UPDATE:updater_->Check();break;
            case wxID_ABOUT:wxMessageBox("Noty " NOTY_VERSION "\nC++20 · wxWidgets · Scintilla\nhttps://github.com/Lyvt-Dev/Noty","Noty",wxOK,this);break;
            }
        });
        tabs_->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE,[this](wxAuiNotebookEvent& e){
            auto* d=static_cast<NotyDocument*>(tabs_->GetPage(e.GetSelection()));if(!Confirm(d))e.Veto();
        });
        Bind(wxEVT_CLOSE_WINDOW,[this](wxCloseEvent& e){
            for(size_t i=0;i<tabs_->GetPageCount();++i)
                if(!Confirm(static_cast<NotyDocument*>(tabs_->GetPage(i)))){e.Veto();return;}
            Destroy();
        });
        New(); ApplyTheme();
    }
    void Open(const wxString& path) {
        for(size_t i=0;i<tabs_->GetPageCount();++i) {
            auto* d=static_cast<NotyDocument*>(tabs_->GetPage(i));
            if(d->path==path){tabs_->SetSelection(i);return;}
        }
        wxFile input(path);
        if(!input.IsOpened()){wxMessageBox("Datei kann nicht geöffnet werden.");return;}
        if(input.Length()>64*1024*1024){wxMessageBox("Diese Vorschau unterstützt Dateien bis 64 MiB.");return;}
        wxString text;
        if(!input.ReadAll(&text,wxConvUTF8)){wxMessageBox("Datei ist kein gültiges UTF-8. Bitte zuerst konvertieren.");return;}
        New();auto* d=Current();d->path=path;d->SetText(text);d->EmptyUndoBuffer();d->SetSavePoint();
        d->Language();d->Theme(dark_);RefreshLabel(d);
    }
};
class NotyApp : public wxApp {
public:
    bool OnInit() override {
        SetAppName("Noty");auto* frame=new NotyFrame;frame->Show();
        for(int i=1;i<argc;++i)frame->Open(argv[i]);
        return true;
    }
};
wxIMPLEMENT_APP(NotyApp);
