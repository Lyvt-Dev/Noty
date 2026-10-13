#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <cstdint>
namespace noty {
enum class Encoding { Utf8, Utf8Bom, Utf16LE, Utf16BE, Windows1252, Latin1 };
struct Decoded { std::string utf8; Encoding encoding; };
inline std::vector<std::uint32_t> Codepoints(std::string_view s) {
    std::vector<std::uint32_t> out;
    for(std::size_t i=0;i<s.size();) {
        auto c=static_cast<unsigned char>(s[i++]); std::uint32_t cp=c; int n=0;
        if(c<128) n=0; else if(c>=0xc2 && c<=0xdf){cp=c&31;n=1;}
        else if(c>=0xe0 && c<=0xef){cp=c&15;n=2;} else if(c>=0xf0&&c<=0xf4){cp=c&7;n=3;}
        else throw std::runtime_error("Invalid UTF-8");
        if(i+n>s.size()) throw std::runtime_error("Truncated UTF-8");
        for(int k=0;k<n;++k){auto b=static_cast<unsigned char>(s[i++]);if((b&0xc0)!=0x80)throw std::runtime_error("Invalid UTF-8");cp=(cp<<6)|(b&63);}
        if((n==1&&cp<0x80)||(n==2&&cp<0x800)||(n==3&&cp<0x10000)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))throw std::runtime_error("Invalid Unicode");
        out.push_back(cp);
    }
    return out;
}
inline void AppendUtf8(std::string& out,std::uint32_t cp) {
    if(cp<0x80)out+=static_cast<char>(cp);
    else if(cp<0x800){out+=static_cast<char>(0xc0|(cp>>6));out+=static_cast<char>(0x80|(cp&63));}
    else if(cp<0x10000){out+=static_cast<char>(0xe0|(cp>>12));out+=static_cast<char>(0x80|((cp>>6)&63));out+=static_cast<char>(0x80|(cp&63));}
    else {out+=static_cast<char>(0xf0|(cp>>18));out+=static_cast<char>(0x80|((cp>>12)&63));out+=static_cast<char>(0x80|((cp>>6)&63));out+=static_cast<char>(0x80|(cp&63));}
}
inline constexpr std::uint32_t cp1252[32]={0x20ac,0x81,0x201a,0x192,0x201e,0x2026,0x2020,0x2021,0x2c6,0x2030,0x160,0x2039,0x152,0x8d,0x17d,0x8f,0x90,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x2dc,0x2122,0x161,0x203a,0x153,0x9d,0x17e,0x178};
inline Decoded Decode(std::string_view bytes) {
    Encoding e=Encoding::Utf8;std::size_t start=0;
    if(bytes.starts_with("\xef\xbb\xbf")){e=Encoding::Utf8Bom;start=3;}
    else if(bytes.starts_with("\xff\xfe")){e=Encoding::Utf16LE;start=2;}
    else if(bytes.starts_with("\xfe\xff")){e=Encoding::Utf16BE;start=2;}
    if(e==Encoding::Utf16LE||e==Encoding::Utf16BE) {
        if((bytes.size()-start)%2)throw std::runtime_error("Truncated UTF-16");
        std::string out;
        auto unit=[&](std::size_t i){auto a=static_cast<unsigned char>(bytes[i]);auto b=static_cast<unsigned char>(bytes[i+1]);return static_cast<std::uint32_t>(e==Encoding::Utf16LE?a|(b<<8):(a<<8)|b);};
        for(auto i=start;i<bytes.size();i+=2){auto cp=unit(i);if(cp>=0xd800&&cp<=0xdbff){if(i+3>=bytes.size())throw std::runtime_error("Invalid surrogate");auto low=unit(i+2);if(low<0xdc00||low>0xdfff)throw std::runtime_error("Invalid surrogate");cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);i+=2;}else if(cp>=0xdc00&&cp<=0xdfff)throw std::runtime_error("Invalid surrogate");AppendUtf8(out,cp);}
        return {out,e};
    }
    auto content=bytes.substr(start);
    try {Codepoints(content);return {std::string(content),e};} catch(...) {
        if(e==Encoding::Utf8Bom)throw;
        std::string out;for(unsigned char c:content)AppendUtf8(out,c>=0x80&&c<0xa0?cp1252[c-0x80]:c);
        return {out,Encoding::Windows1252};
    }
}
inline std::string Encode(std::string_view utf8,Encoding e) {
    auto points=Codepoints(utf8);std::string out;
    if(e==Encoding::Utf8)return std::string(utf8);
    if(e==Encoding::Utf8Bom)return std::string("\xef\xbb\xbf")+std::string(utf8);
    if(e==Encoding::Utf16LE||e==Encoding::Utf16BE) {
        out=e==Encoding::Utf16LE?std::string("\xff\xfe"):std::string("\xfe\xff");
        auto add=[&](std::uint32_t c){if(e==Encoding::Utf16LE){out+=static_cast<char>(c&255);out+=static_cast<char>(c>>8);}else{out+=static_cast<char>(c>>8);out+=static_cast<char>(c&255);}};
        for(auto cp:points){if(cp<=0xffff)add(cp);else{cp-=0x10000;add(0xd800+(cp>>10));add(0xdc00+(cp&1023));}}return out;
    }
    for(auto cp:points) {
        if(cp<128||(cp>=160&&cp<=255)||(e==Encoding::Latin1&&cp<=255)){out+=static_cast<char>(cp);continue;}
        bool found=false;if(e==Encoding::Windows1252)for(int i=0;i<32;++i)if(cp1252[i]==cp){out+=static_cast<char>(0x80+i);found=true;break;}
        if(!found)throw std::runtime_error("Character cannot be represented in target encoding");
    }
    return out;
}
}
