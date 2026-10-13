#include "search.h"
#include "codec.h"
#include <wx/regex.h>
#include <wx/log.h>
#include <algorithm>
#include <stdexcept>
namespace noty {
std::optional<RegexHit> RegexFind(const wxString& text,const wxString& pattern,int fromByte,int toByte,bool matchCase,bool wholeWord){
    auto bytes=text.ToStdString(wxConvUTF8);int low=std::min(fromByte,toByte),high=std::max(fromByte,toByte);if(low<0||high>static_cast<int>(bytes.size()))return {};
    auto total=Codepoints(bytes).size();auto prefix=std::string_view(bytes).substr(0,low);auto offset=Codepoints(prefix).size();std::optional<RegexHit> best;
    auto query=wholeWord?wxString("\\b(?:")+pattern+")\\b":pattern;
    for(;;){
        // Vollständiger Kontext bleibt erhalten: Lookbehind und Zeilenanker
        // funktionieren auch bei einer Suche ab der aktuellen Cursorposition.
        auto wrapped=wxString::Format("(*UCP)\\A(?s:.{%zu}.*?)\\K(?:",offset)+query+")";
        wxRegEx expression;{wxLogNull quiet;if(!expression.Compile(wrapped,wxRE_NEWLINE|(matchCase?0:wxRE_ICASE)))throw std::runtime_error("Invalid regular expression");}
        if(!expression.Matches(text))break;std::size_t start=0,length=0;if(!expression.GetMatch(&start,&length))break;
        int begin=static_cast<int>(text.Left(start).utf8_str().length()),end=begin+static_cast<int>(text.Mid(start,length).utf8_str().length());
        if(begin>high)break;if(end<=high){RegexHit hit{begin,end,{}};for(std::size_t group=0;group<expression.GetMatchCount();++group)hit.groups.push_back(expression.GetMatch(text,group));best=std::move(hit);if(fromByte<=toByte)return best;}
        auto position=Codepoints(std::string_view(bytes).substr(0,begin)).size();if(position>=total)break;offset=position+1;
    }
    return best;
}
wxString RegexReplacement(const wxString& replacement,const std::vector<wxString>& groups){
    auto input=replacement.ToStdWstring();wxString out;
    for(std::size_t i=0;i<input.size();++i){auto c=input[i];if((c==L'\\'||c==L'$')&&i+1<input.size()){
        auto next=input[i+1];if(next>=L'0'&&next<=L'9'){std::size_t index=next-L'0';++i;if(index<groups.size())out+=groups[index];continue;}
        if(c==L'\\'){++i;if(next==L'n')out+='\n';else if(next==L'r')out+='\r';else if(next==L't')out+='\t';else out+=next;continue;}
        if(next==L'$'){++i;out+='$';continue;}
    }out+=c;}
    return out;
}
}
