#include "ui_text.h"
#include <iostream>
int main(){
    for(const char* text:{"Öffnen","Rückgängig","schließen","größer","Änderungen","Übersicht","Straße"}){
        auto value=wxString::FromUTF8(text);if(value.ToStdString(wxConvUTF8)!=text)return 1;
    }
    if("Öffnen"_n[0]!=wxUniChar(0x00d6))return 1;
    if("Rückgängig"_n[1]!=wxUniChar(0x00fc))return 1;
    if("Straße"_n[4]!=wxUniChar(0x00df))return 1;
    std::cout<<"German UI Unicode conversion passed\n";
}
