#include "updater.h"
#include "ui_text.h"
#include <nlohmann/json.hpp>
#include <array>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/utils.h>
#include <picosha2.h>
#include <charconv>
#include <optional>
#include <string_view>
#include "version.h"
#include <wx/stdpaths.h>
#include <wx/config.h>
#include <wx/process.h>
using noty::Version;
Updater::Updater(wxWindow* owner,const wxString& configDir) : owner_(owner),configDir_(configDir),stage_(configDir+wxFILE_SEP_PATH+"updates") {
    wxConfig config("Noty");config.Read("automaticUpdates",&automatic_,true);
    Bind(wxEVT_WEBREQUEST_STATE, &Updater::OnState, this);
}
Updater::~Updater() {
    if(request_.IsOk() && request_.GetState()==wxWebRequest::State_Active) request_.Cancel();
    if(download_.IsOk() && download_.GetState()==wxWebRequest::State_Active) download_.Cancel();
}
void Updater::Error(const wxString& message){if(interactive_)wxMessageBox(message,"Noty Update",wxOK|wxICON_WARNING,owner_);}
void Updater::SetAutomatic(bool value){automatic_=value;wxConfig config("Noty");config.Write("automaticUpdates",value);config.Flush();}
void Updater::Restart(){if(!ready_)return;restart_=true;if(!owner_->Close())restart_=false;}
bool Updater::ApplyOnExit(){
    if(!ready_)return true;
    wxString executable=wxStandardPaths::Get().GetExecutablePath();
    wxFileName helper(executable);helper.SetFullName(
#ifdef __WXMSW__
    "NotyUpdate.exe"
#else
    "NotyUpdate"
#endif
    );
    wxString staged=stage_+wxFILE_SEP_PATH+helper.GetFullName();
    if(!wxCopyFile(helper.GetFullPath(),staged,true)){Error("Update-Helfer fehlt. Bitte das Setup von GitHub verwenden."_n);return false;}
#ifndef __WXMSW__
    wxChmod(staged,0755);
#endif
    wxString argv[]={staged,wxString::Format("%ld",wxGetProcessId()),stage_+wxFILE_SEP_PATH+assetName_,wxString::FromUTF8(digest_),executable,restart_?"restart":"close",wxString()};
    const wchar_t* args[7];for(int i=0;i<6;++i)args[i]=argv[i].wc_str();args[6]=nullptr;
    if(wxExecute(args,wxEXEC_ASYNC|wxEXEC_HIDE_CONSOLE)<=0){Error("Update konnte nicht gestartet werden."_n);return false;}return true;
}
void Updater::Check(bool interactive) {
    if(!interactive&&!automatic_)return;
    interactive_=interactive;
    if(ready_){if(interactive)wxMessageBox("Update ist bereit. Klicke auf Neu starten."_n,"Noty Update",wxOK,owner_);return;}
    if(download_.IsOk() && download_.GetState()==wxWebRequest::State_Active) return;
    if(request_.IsOk() && request_.GetState()==wxWebRequest::State_Active) return;
    request_=wxWebSession::GetDefault().CreateRequest(this,
        "https://api.github.com/repos/Lyvt-Dev/Noty/releases?per_page=20");
    if(!request_.IsOk()) {
        Error("Update-Dienst nicht verfügbar."_n); return;
    }
    request_.SetHeader("Accept","application/vnd.github+json");
    request_.SetHeader("User-Agent","Noty/" NOTY_VERSION);
    request_.Start();
}
void Updater::OnState(wxWebRequestEvent& event) {
    if(event.GetState()==wxWebRequest::State_Failed || event.GetState()==wxWebRequest::State_Unauthorized) {
        Error("Updates konnten nicht geprüft werden. Bitte später erneut versuchen."_n); return;
    }
    if(event.GetState()!=wxWebRequest::State_Completed) return;
    if(event.GetResponse().GetStatus()==404) {
        if(interactive_)wxMessageBox("Es ist noch kein Release verfügbar."_n,"Noty",wxOK,owner_); return;
    }
    try {
        if(download_.IsOk() && event.GetRequest().GetId()==2) {
            if(event.GetResponse().GetStatus()!=200) throw std::runtime_error("Download");
            wxString source=event.GetResponse().GetDataFile();
            wxFile file(source);
            if(!file.IsOpened()) throw std::runtime_error("Download file");
            picosha2::hash256_one_by_one hash;
            std::array<unsigned char,65536> buffer;
            for(;;) {
                auto count=file.Read(buffer.data(),buffer.size());
                if(count<0) throw std::runtime_error("Read");
                if(count==0) break;
                hash.process(buffer.begin(),buffer.begin()+count);
            }
            file.Close();hash.finish();
            if(picosha2::get_hash_hex_string(hash)!=digest_) throw std::runtime_error("Checksum");
            wxFileName::Mkdir(stage_,wxS_DIR_DEFAULT,wxPATH_MKDIR_FULL);
            wxString destination=stage_+wxFILE_SEP_PATH+assetName_;
            if(!wxCopyFile(source,destination,true))throw std::runtime_error("Stage");
            ready_=true;if(changed)changed();
            if(interactive_)wxMessageBox("Update geprüft und vorbereitet. Klicke auf Neu starten, um es zu installieren. Ungespeicherte Notizen werden vorher abgefragt."_n,"Noty Update",wxOK,owner_);
            return;
        }
        if(event.GetResponse().GetStatus()!=200) throw std::runtime_error("HTTP");
        auto releases=nlohmann::json::parse(event.GetResponse().AsString().ToStdString(wxConvUTF8));
        nlohmann::json data;
        std::array<int,3> newest{};
        for(const auto& release:releases) {
            if(release.value("draft",false)) continue;
            auto version=Version(release.at("tag_name").get<std::string>());
            if(version && *version>newest) {newest=*version;data=release;}
        }
        if(data.is_null()) {if(interactive_)wxMessageBox("Noch keine passende Version verfügbar."_n,"Noty",wxOK,owner_);return;}
        auto remote=Version(data.at("tag_name").get<std::string>());
        auto local=Version(NOTY_VERSION);
        if(!remote || !local) throw std::runtime_error("Version");
        if(*remote<=*local) {
            if(interactive_)wxMessageBox("Noty ist aktuell (Version " NOTY_VERSION ").","Noty",wxOK,owner_); return;
        }
        // Nur ein für die Plattform bestimmtes Asset aus diesem Repository akzeptieren.
#ifdef __WXMSW__
        const std::string wanted="Noty-windows-x64-setup.exe";
        suffix_=".exe";
#elif defined(__WXOSX__)
        const std::string wanted="Noty-macos-arm64.dmg";
        suffix_=".dmg";
#else
        const std::string wanted="Noty-linux-x64.deb";
        suffix_=".deb";
#endif
        for(const auto& asset:data.at("assets")) {
            if(asset.at("name").get<std::string>()!=wanted) continue;
            auto url=asset.at("browser_download_url").get<std::string>();
            auto digest=asset.value("digest",std::string{});
            auto size=asset.at("size").get<long long>();
            const std::string expected="https://github.com/Lyvt-Dev/Noty/releases/download/"+
                data.at("tag_name").get<std::string>()+"/"+wanted;
            if(url!=expected || digest.size()!=71 || !digest.starts_with("sha256:") ||
                size<=0 || size>256*1024*1024) throw std::runtime_error("Asset");
            assetName_=wxString::FromUTF8(wanted);
            digest_=digest.substr(7);
            download_=wxWebSession::GetDefault().CreateRequest(this,wxString::FromUTF8(url.c_str()),2);
            if(!download_.IsOk()) throw std::runtime_error("Request");
            download_.SetStorage(wxWebRequest::Storage_File);
            download_.SetHeader("User-Agent","Noty/" NOTY_VERSION);
            download_.Start();return;
        }
        if(interactive_)wxMessageBox("Für diese Plattform fehlt ein Installer. Die Release-Seite wird geöffnet."_n,
            "Noty",wxOK,owner_);
        if(interactive_)wxLaunchDefaultBrowser("https://github.com/Lyvt-Dev/Noty/releases");
    } catch(...) {
        Error("Ungültige Antwort vom Update-Dienst."_n);
    }
}
