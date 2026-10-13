#pragma once
#include <wx/string.h>
#include <optional>
#include <vector>
namespace noty {
struct RegexHit{int start,end;std::vector<wxString> groups;};
std::optional<RegexHit> RegexFind(const wxString& text,const wxString& pattern,int fromByte,int toByte,bool matchCase,bool wholeWord);
wxString RegexReplacement(const wxString& replacement,const std::vector<wxString>& groups);
}
