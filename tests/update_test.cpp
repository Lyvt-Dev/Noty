#include "version.h"
#include <picosha2.h>
#include <iostream>
int main() {
    auto require=[](bool condition) { if(!condition) {std::cerr<<"Update validation failed\\n";std::exit(1);} };
    require(noty::Version("v1.2.3")==std::array<int,3>{1,2,3});
    require(noty::Version("0.2.0").has_value());
    require(*noty::Version("1.10.0")>*noty::Version("1.2.9"));
    for(auto bad : {"", "v", "1.2", "-1.2.3", "1.2.3.4", "1.2.3evil", "999999999999.1.1"})
        require(!noty::Version(bad).has_value());
    require(picosha2::hash256_hex_string(std::string("abc"))==
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}
