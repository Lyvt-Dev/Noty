#include "updater.h"
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
using noty::Version;
Updater::Updater(wxWindow* owner) : owner_(owner) {
    Bind(wxEVT_WEBREQUEST_STATE, &Updater::OnState, this);
}
Updater::~Updater() {
    if(request_.IsOk() && request_.GetState()==wxWebRequest::State_Active) request_.Cancel();
    if(download_.IsOk() && download_.GetState()==wxWebRequest::State_Active) download_.Cancel();
}
void Updater::Check() {
    if(download_.IsOk() && download_.GetState()==wxWebRequest::State_Active) return;
    if(request_.IsOk() && request_.GetState()==wxWebRequest::State_Active) return;
    request_=wxWebSession::GetDefault().CreateRequest(this,
        "https://api.github.com/repos/Lyvt-Dev/Noty/releases?per_page=20");
    if(!request_.IsOk()) {
        wxMessageBox("Update-Dienst nicht verfügbar.", "Noty", wxOK|wxICON_ERROR, owner_); return;
    }
    request_.SetHeader("Accept","application/vnd.github+json");
    request_.SetHeader("User-Agent","Noty/" NOTY_VERSION);
    request_.Start();
}
void Updater::OnState(wxWebRequestEvent& event) {
    if(event.GetState()==wxWebRequest::State_Failed || event.GetState()==wxWebRequest::State_Unauthorized) {
        wxMessageBox("Updates konnten nicht geprüft werden. Bitte später erneut versuchen.",
            "Noty",wxOK|wxICON_WARNING,owner_); return;
    }
    if(event.GetState()!=wxWebRequest::State_Completed) return;
    if(event.GetResponse().GetStatus()==404) {
        wxMessageBox("Es ist noch kein Release verfügbar.","Noty",wxOK,owner_); return;
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
            wxString base=wxFileName::CreateTempFileName("noty-update-");
            if(base.empty()) throw std::runtime_error("Temp");
            wxString destination=base+suffix_;
            if(!wxCopyFile(source,destination,false)) {wxRemoveFile(base);throw std::runtime_error("Copy");}
            wxRemoveFile(base);
            if(wxMessageBox("Das Update wurde heruntergeladen und die SHA-256-Prüfsumme bestätigt.\n"
                "Bitte Dokumente speichern und Noty schließen. Installer jetzt öffnen?",
                "Noty Update",wxYES_NO|wxICON_INFORMATION,owner_)==wxYES)
                wxLaunchDefaultApplication(destination);
            else wxMessageBox("Update gespeichert unter:\n"+destination,"Noty Update",wxOK,owner_);
            return;
        }
        if(event.GetResponse().GetStatus()!=200) throw std::runtime_error("HTTP");
        auto releases=nlohmann::json::parse(event.GetResponse().AsString().ToStdString());
        nlohmann::json data;
        std::array<int,3> newest{};
        for(const auto& release:releases) {
            if(release.value("draft",false)) continue;
            auto version=Version(release.at("tag_name").get<std::string>());
            if(version && *version>newest) {newest=*version;data=release;}
        }
        if(data.is_null()) {wxMessageBox("Noch keine passende Version verfügbar.","Noty",wxOK,owner_);return;}
        auto remote=Version(data.at("tag_name").get<std::string>());
        auto local=Version(NOTY_VERSION);
        if(!remote || !local) throw std::runtime_error("Version");
        if(*remote<=*local) {
            wxMessageBox("Noty ist aktuell (Version " NOTY_VERSION ").","Noty",wxOK,owner_); return;
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
            if(wxMessageBox("Eine neue Version ist verfügbar (auch Vorschau-Versionen).\n"
                "Update herunterladen und prüfen?","Noty Update",
                wxYES_NO|wxICON_INFORMATION,owner_)!=wxYES) return;
            digest_=digest.substr(7);
            download_=wxWebSession::GetDefault().CreateRequest(this,wxString::FromUTF8(url.c_str()),2);
            if(!download_.IsOk()) throw std::runtime_error("Request");
            download_.SetStorage(wxWebRequest::Storage_File);
            download_.SetHeader("User-Agent","Noty/" NOTY_VERSION);
            download_.Start();return;
        }
        wxMessageBox("Für diese Plattform fehlt ein Installer. Die Release-Seite wird geöffnet.",
            "Noty",wxOK,owner_);
        wxLaunchDefaultBrowser("https://github.com/Lyvt-Dev/Noty/releases");
    } catch(...) {
        wxMessageBox("Ungültige Antwort vom Update-Dienst.","Noty",wxOK|wxICON_WARNING,owner_);
    }
}
