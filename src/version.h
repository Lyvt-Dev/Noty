#pragma once
#include <array>
#include <charconv>
#include <optional>
#include <string_view>
namespace noty {
inline std::optional<std::array<int,3>> Version(std::string_view value) {
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
