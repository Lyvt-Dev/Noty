#pragma once
#include <wx/string.h>
// UTF-8-Quelltexte niemals von der Windows-ANSI-Codepage abhängig machen.
inline wxString operator""_n(const char* value, std::size_t length) {
    return wxString::FromUTF8(value, length);
}
