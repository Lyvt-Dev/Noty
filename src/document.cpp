#include "document.h"
#include <wx/filename.h>
#include <algorithm>
#include <map>
#include <wx/fontenum.h>
#include "ui_text.h"
    NotyDocument::NotyDocument(wxWindow* parent) : wxStyledTextCtrl(parent) {
        SetCodePage(wxSTC_CP_UTF8);
        SetMarginType(0,wxSTC_MARGIN_NUMBER); SetMarginWidth(0,0);
        SetMarginType(2,wxSTC_MARGIN_SYMBOL); SetMarginMask(2,wxSTC_MASK_FOLDERS);
        SetMarginWidth(2,0); SetMarginSensitive(2,true);
        SetMarginType(1,wxSTC_MARGIN_SYMBOL);SetMarginMask(1,1);SetMarginSensitive(1,true);SetMarginWidth(1,0);MarkerDefine(0,wxSTC_MARK_CIRCLE,wxColour(0,120,164),wxColour(0,120,164));
        for(int i=wxSTC_MARKNUM_FOLDEREND;i<=wxSTC_MARKNUM_FOLDEROPEN;++i)
            MarkerDefine(i,wxSTC_MARK_BOXPLUS);
        MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN,wxSTC_MARK_BOXMINUS);
        SetProperty("fold","1"); SetTabWidth(4); SetUseTabs(false);
        SetIndentationGuides(wxSTC_IV_NONE); SetCaretLineVisible(false);
        SetMarginLeft(16);SetMarginRight(12);SetMultipleSelection(true);SetAdditionalSelectionTyping(true);
        SetMouseSelectionRectangularSwitch(true);SetVirtualSpaceOptions(wxSTC_VS_RECTANGULARSELECTION);
        #ifdef __WXMSW__
        editorFont=wxFontInfo(11).Family(wxFONTFAMILY_TELETYPE).FaceName(wxFontEnumerator::IsValidFacename("Cascadia Mono")?"Cascadia Mono":"Consolas");
#elif defined(__WXOSX__)
        editorFont=wxFontInfo(11).FaceName("Menlo").Family(wxFONTFAMILY_TELETYPE);
#else
        editorFont=wxFontInfo(11).Family(wxFONTFAMILY_TELETYPE);
#endif
        SetWrapMode(wxSTC_WRAP_WORD);SetScrollWidth(1);SetScrollWidthTracking(true);
        Bind(wxEVT_STC_MARGINCLICK,[this](wxStyledTextEvent& e){
            if(e.GetMargin()==1){int line=LineFromPosition(e.GetPosition());if(MarkerGet(line)&1)MarkerDelete(line,0);else MarkerAdd(line,0);}
            if(e.GetMargin()==2) ToggleFold(LineFromPosition(e.GetPosition()));
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
    void NotyDocument::Language() {
        auto ext=wxFileName(path).GetExt().Lower();
        int lexer=wxSTC_LEX_NULL;
        if(ext=="cpp"||ext=="h"||ext=="hpp"||ext=="c"||ext=="js") lexer=wxSTC_LEX_CPP;
        else if(ext=="py") lexer=wxSTC_LEX_PYTHON;
        else if(ext=="json") lexer=wxSTC_LEX_JSON;
        else if(ext=="xml"||ext=="html"||ext=="htm") lexer=wxSTC_LEX_HTML;
        else if(ext=="css") lexer=wxSTC_LEX_CSS;
        else if(ext=="md") lexer=wxSTC_LEX_MARKDOWN;
        else if(ext=="bat"||ext=="cmd") lexer=wxSTC_LEX_BATCH;
        else {static const std::map<wxString,int> languages={{"rs",wxSTC_LEX_RUST},{"sql",wxSTC_LEX_SQL},{"yaml",wxSTC_LEX_YAML},{"yml",wxSTC_LEX_YAML},{"sh",wxSTC_LEX_BASH},{"lua",wxSTC_LEX_LUA},{"ps1",wxSTC_LEX_POWERSHELL},{"ini",wxSTC_LEX_PROPERTIES},{"rb",wxSTC_LEX_RUBY},{"pl",wxSTC_LEX_PERL},{"pas",wxSTC_LEX_PASCAL},{"r",wxSTC_LEX_R},{"tex",wxSTC_LEX_LATEX},{"diff",wxSTC_LEX_DIFF},{"asm",wxSTC_LEX_ASM},{"vb",wxSTC_LEX_VB},{"vbs",wxSTC_LEX_VBSCRIPT},{"cmake",wxSTC_LEX_CMAKE},{"java",wxSTC_LEX_CPP},{"cs",wxSTC_LEX_CPP},{"ts",wxSTC_LEX_CPP},{"go",wxSTC_LEX_CPP},{"swift",wxSTC_LEX_CPP},{"php",wxSTC_LEX_HTML}};auto found=languages.find(ext);if(found!=languages.end())lexer=found->second;}
        SetLexer(lexer);
        SetKeyWords(0,lexer==wxSTC_LEX_PYTHON ?
            "False None True and as assert async await break class continue def del elif else except finally for from global if import in is lambda nonlocal not or pass raise return try while with yield" :
            "alignas auto bool break case catch char class const constexpr continue default delete do double else enum false float for if int long namespace new nullptr private protected public return short signed sizeof static struct switch template this throw true try typedef typename union unsigned using virtual void volatile while");
        Colourise(0,-1);UpdateMargins();
    }
    void NotyDocument::Theme(bool dark) {
        wxColour bg=dark?NotyPalette::Get(true).paper:NotyPalette::Get(false).paper;
        wxColour fg=dark?NotyPalette::Get(true).text:NotyPalette::Get(false).text;
        StyleSetFont(wxSTC_STYLE_DEFAULT,editorFont);
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
        if(GetLexer()!=wxSTC_LEX_NULL){paint(1,green);paint(2,orange);paint(3,blue);paint(4,blue);paint(5,orange);paint(6,orange);}
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

void NotyDocument::UpdateMargins(){SetMarginWidth(0,numbers?FromDIP(48):0);SetMarginWidth(1,numbers?FromDIP(16):0);SetMarginWidth(2,numbers?FromDIP(16):0);}
void NotyDocument::Bookmark(){int line=GetCurrentLine();if(MarkerGet(line)&1)MarkerDelete(line,0);else MarkerAdd(line,0);}
void NotyDocument::NextBookmark(bool reverse){int line=reverse?MarkerPrevious(GetCurrentLine()-1,1):MarkerNext(GetCurrentLine()+1,1);if(line<0)line=reverse?MarkerPrevious(GetLineCount()-1,1):MarkerNext(0,1);if(line>=0)GotoLine(line);}
void NotyDocument::Comment(){
    auto ext=wxFileName(path).GetExt().Lower();wxString prefix=(ext=="py")?"# ":((ext=="bat"||ext=="cmd")?"REM ":"// ");
    int first=LineFromPosition(GetSelectionStart()),last=LineFromPosition(GetSelectionEnd());
    BeginUndoAction();for(int line=first;line<=last;++line){int pos=GetLineIndentPosition(line);SetTargetStart(pos);SetTargetEnd(std::min(GetLineEndPosition(line),pos+static_cast<int>(prefix.length())));if(GetTextRange(GetTargetStart(),GetTargetEnd())==prefix)ReplaceTarget("");else InsertText(pos,prefix);}EndUndoAction();
}
