#include "codec.h"
#include <iostream>
#include <cstdlib>
int main(){
    auto require=[](bool ok){if(!ok){std::cerr<<"Encoding test failed\n";std::exit(1);}};
    std::string german="Öffnen · Rückgängig · schließen · größer · Straße";
    std::string unicode=german+" 😀 東京";
    for(auto e:{noty::Encoding::Utf8,noty::Encoding::Utf8Bom,noty::Encoding::Utf16LE,noty::Encoding::Utf16BE})
        require(noty::Decode(noty::Encode(unicode,e)).utf8==unicode);
    require(noty::Decode(noty::Encode(german,noty::Encoding::Windows1252)).utf8==german);
    bool rejected=false;try{noty::Encode(unicode,noty::Encoding::Windows1252);}catch(...){rejected=true;}require(rejected);
    rejected=false;try{noty::Decode(std::string("\xff\xfe\x00\xd8",4));}catch(...){rejected=true;}require(rejected);
    rejected=false;try{noty::Codepoints(std::string("\xc0\x80",2));}catch(...){rejected=true;}require(rejected);
    require(noty::Encode("a\r\nb\n",noty::Encoding::Utf8)=="a\r\nb\n");
}
