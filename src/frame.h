#pragma once
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/checkbox.h>
#include <wx/dirctrl.h>
#include <wx/listctrl.h>
#include <wx/timer.h>
#include <wx/dynlib.h>
#include "plugin_api.h"
#include "document.h"
#include "updater.h"
#include <memory>
#include <vector>
#include <future>
#include <tuple>
#include <nlohmann/json.hpp>
enum { ID_FIND=wxID_HIGHEST+1,ID_NEXT,ID_PREV,ID_REPLACE,ID_THEME,ID_UPDATE,ID_GOTO,ID_ALLSAVE,ID_FONT,ID_LF,ID_CRLF,ID_CR,
 ID_WRAP,ID_NUMBERS,ID_SPACES,ID_INDENT,ID_BOOKMARK,ID_BOOKNEXT,ID_BOOKPREV,ID_COMMENT,ID_DUPLICATE,ID_DELETE,ID_UPPER,ID_LOWER,
 ID_TRIM,ID_SORT,ID_TABSPACE,ID_SPACETAB,ID_FOLD,ID_UNFOLD,ID_SPLIT,ID_FOLDER,ID_PALETTE,ID_RECORD,ID_PLAY,ID_REOPEN,ID_CLOSEALL,
 ID_CLOSEOTHERS,ID_ENCODING,ID_UTF8,ID_UTF8BOM,ID_UTF16LE,ID_UTF16BE,ID_ANSI,ID_LATIN1,ID_ZOOMIN,ID_ZOOMOUT,ID_ZOOMRESET,
 ID_PIN,ID_FINDALL,ID_FIND_FILES,ID_COMPLETE,ID_MACRO_SAVE,ID_MACRO_LOAD,ID_PLUGIN,ID_EXPORTHTML,ID_LANGUAGE };
class NotyFrame : public wxFrame {
public:
    explicit NotyFrame(bool portable=false,bool restore=true);
    void Open(const wxString& path,int line=-1);
    void Command(int id);
    bool SmokeTest(const wxString& screenshot);
private:
    wxPanel *host_=nullptr,*find_=nullptr;
    wxSplitterWindow* split_=nullptr;
    NotyDocument* second_=nullptr;
    wxGenericDirCtrl* explorer_=nullptr;
    wxListCtrl* results_=nullptr;
    NotyChrome* chrome_=nullptr;
    NotyStatus* status_=nullptr;
    wxTextCtrl *query_=nullptr,*replacement_=nullptr;
    wxCheckBox *regex_=nullptr,*case_=nullptr,*word_=nullptr,*all_=nullptr;
    wxStaticText* findInfo_=nullptr;
    std::vector<NotyDocument*> documents_;
    std::vector<wxString> recentlyClosed_;
    struct MacroStep{bool insert;int offset,length;std::string text;};
    std::vector<MacroStep> macro_;
    bool dark_=false,recording_=false,replaying_=false,restoring_=false,closing_=false;
    int current_=0,macroBase_=0;
    wxString configDir_;
    std::future<void> persist_;
    std::future<std::vector<std::tuple<std::string,int,std::string>>> search_;
    int ticks_=0;
    struct PendingOpen{wxString path;int line;std::future<std::pair<wxString,noty::Encoding>> task;};
    std::vector<PendingOpen> opening_;
    wxTimer timer_;
    std::unique_ptr<Updater> updater_;
    struct Plugin{std::unique_ptr<wxDynamicLibrary> library;const NotyPluginV1* api;};
    std::vector<Plugin> plugins_;
    std::string pluginText_;
    std::function<int(NotyDocument*)> confirmChoice_;
    std::vector<std::unique_ptr<wxMenu>> menus_;
    std::vector<std::pair<int,wxString>> commands_;
    NotyDocument* Current() const;
    void New();
    void Select(int index);
    bool CloseTab(int index);
    bool Confirm(NotyDocument* d);
    bool Save(NotyDocument* d,bool saveAs=false);
    void ApplyTheme();
    void UpdateStatus();
    void Menus();
    void Popup(int index);
    void Find(bool backward=false,bool replace=false,bool every=false);
    void ShowFind(bool replacing);
    void Persist();
    void Restore();
    void FinishOpening();
    void Result(const wxString& path,int line,const wxString& text);
    void FileSearch();
    void PlayMacro();
    void Plugins();
#ifdef __WXMSW__
    WXLRESULT MSWWindowProc(WXUINT,WXWPARAM,WXLPARAM) override;
#endif
};
