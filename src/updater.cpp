#include "updater.h"
#include <nlohmann/json.hpp>
#include <array>
#include <charconv>
#include <optional>
#include <string_view>
namespace {
std::optional<std::array<int,3>> Version(std::string_view value) {
    if (!value.empty() && value.front() == 'v') value.remove_prefix(1);
    std::array<int,3> result{};
    for (int i=0; i<3; ++i) {
        auto end = value.find('.');
        auto part = value.substr(0,end);
        auto [p,ec] = std::from_chars(part.data(),part.data()+part.size(),result[i]);
        if (ec != std::errc{} || p != part.data()+part.size() || result[i]<0) return {};
        if (i<2) { if(end==value.npos) return {}; value.remove_prefix(end+1); }
        else if(end!=value.npos) return {};
    }
    return result;
}
}
Updater::Updater(wxWindow* owner) : owner_(owner) {
    Bind(wxEVT_WEBREQUEST_STATE, &Updater::OnState, this);
}
void Updater::Check() {
    if(request_.IsOk() && request_.GetState()==wxWebRequest::State_Active) return;
    request_=wxWebSession::GetDefault().CreateRequest(this,
        "https://api.github.com/repos/Lyvt-Dev/Noty/releases/latest");
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
        if(event.GetResponse().GetStatus()!=200) throw std::runtime_error("HTTP");
        auto data=nlohmann::json::parse(event.GetResponse().AsString().ToStdString());
        auto remote=Version(data.at("tag_name").get<std::string>());
        auto local=Version(NOTY_VERSION);
        if(!remote || !local) throw std::runtime_error("Version");
        if(*remote<=*local) {
            wxMessageBox("Noty ist aktuell (Version " NOTY_VERSION ").","Noty",wxOK,owner_); return;
        }
        // Keine fremden URLs aus Release-Metadaten ausführen.
        if(wxMessageBox("Eine neue Version ist verfügbar. Download-Seite öffnen?\n"
            "Speichere vor der Installation deine Dokumente.",
            "Noty Update",wxYES_NO|wxICON_INFORMATION,owner_)==wxYES)
            wxLaunchDefaultBrowser("https://github.com/Lyvt-Dev/Noty/releases/latest");
    } catch(...) {
        wxMessageBox("Ungültige Antwort vom Update-Dienst.","Noty",wxOK|wxICON_WARNING,owner_);
    }
}
