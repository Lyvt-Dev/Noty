#include "search.h"
#include "ui_text.h"
#include <cstdlib>
#include <iostream>
int main(){
    auto require=[](bool ok){if(!ok){std::cerr<<"Unicode regex test failed\n";std::exit(1);}};
    auto text="Grüße Straße 😀"_n;int length=static_cast<int>(text.utf8_str().length());
    auto hit=noty::RegexFind(text,"Stra.e",0,length,true,false);require(hit.has_value());require(hit->groups[0]=="Straße"_n);
    hit=noty::RegexFind(text,"(?<=Stra)ße"_n,0,length,true,false);require(hit&&hit->groups[0]=="ße"_n);
    hit=noty::RegexFind(text,"(Gr)(üße)"_n,0,length,true,false);require(hit&&noty::RegexReplacement("\\2 \\1",hit->groups)=="üße Gr"_n);
    hit=noty::RegexFind("😀"_n,".",0,4,true,false);require(hit&&hit->start==0&&hit->end==4);
    hit=noty::RegexFind(text,"$",0,length,true,false);require(hit&&hit->start==length&&hit->end==length);
    hit=noty::RegexFind("a b a","a",5,0,true,false);require(hit&&hit->start==4);
    bool bad=false;try{noty::RegexFind(text,"[",0,length,true,false);}catch(...){bad=true;}require(bad);
}
