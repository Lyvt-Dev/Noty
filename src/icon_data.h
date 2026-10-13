#pragma once
#include <string_view>
namespace noty {
inline const char* IconSvg(std::string_view name){
if(name=="note")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>note</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M7 3 H15 L19 7 V21 H5 V3 Z M15 3 V8 H19 M8 12 H16 M8 16 H14" /></svg>)svg";
if(name=="add")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>add</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M12 5 V19 M5 12 H19" /></svg>)svg";
if(name=="search")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>search</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M15 15 L21 21" /><circle fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" cx="10" cy="10" r="6" /></svg>)svg";
if(name=="save")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>save</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M5 3 H17 L21 7 V21 H3 V3 Z M7 3 V9 H16 V3 M7 21 V14 H17 V21" /></svg>)svg";
if(name=="settings")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>settings</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M12 3 V5 M12 19 V21 M3 12 H5 M19 12 H21 M5.6 5.6 L7 7 M17 17 L18.4 18.4 M5.6 18.4 L7 17 M17 7 L18.4 5.6" /><circle fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" cx="12" cy="12" r="5" /></svg>)svg";
if(name=="fullscreen")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>fullscreen</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M8 3 H3 V8 M16 3 H21 V8 M3 16 V21 H8 M21 16 V21 H16" /></svg>)svg";
if(name=="minimize")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>minimize</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M5 12 H19" /></svg>)svg";
if(name=="maximize")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>maximize</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M5 5 H19 V19 H5 Z" /></svg>)svg";
if(name=="restore")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>restore</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M8 5 V3 H21 V16 H19 M3 8 H16 V21 H3 Z" /></svg>)svg";
if(name=="close")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>close</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M6 6 L18 18 M18 6 L6 18" /></svg>)svg";
if(name=="menu")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>menu</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M4 6 H20 M4 12 H20 M4 18 H20" /></svg>)svg";
if(name=="folder")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>folder</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M3 6 H9 L11 9 H21 V20 H3 Z" /></svg>)svg";
if(name=="restart")return R"svg(<?xml version='1.0' encoding='utf-8'?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="24px" height="24px" viewBox="0 0 24 24" version="1.1"><title>restart</title><path fill="none" stroke="#687181" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" d="M20 8 A8 8 0 1 0 20 16 M20 3 V9 H14" /></svg>)svg";
return "";
}
}
