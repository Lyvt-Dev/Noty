#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <array>
#include <cerrno>
#include <picosha2.h>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif
namespace fs=std::filesystem;
std::string Hash(const fs::path& path){std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Update file missing");picosha2::hash256_one_by_one hash;std::array<unsigned char,65536> buffer;while(file){file.read(reinterpret_cast<char*>(buffer.data()),buffer.size());hash.process(buffer.begin(),buffer.begin()+file.gcount());}if(!file.eof())throw std::runtime_error("Update read failed");hash.finish();return picosha2::get_hash_hex_string(hash);}
#ifdef _WIN32
std::wstring Quote(const std::wstring& value){std::wstring out=L"\"";unsigned slashes=0;for(wchar_t c:value){if(c==L'\\'){++slashes;continue;}if(c==L'"')out.append(slashes*2+1,L'\\');else out.append(slashes,L'\\');slashes=0;out+=c;}out.append(slashes*2,L'\\');return out+L'"';}
bool Run(const fs::path& program,const std::vector<std::wstring>& args,bool wait=true,bool nsis=false){std::wstring command=Quote(program.wstring());for(auto& a:args)command+=L" "+(nsis?a:Quote(a));STARTUPINFOW startup{sizeof(startup)};startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=wait?SW_HIDE:SW_SHOWNORMAL;PROCESS_INFORMATION process{};if(!CreateProcessW(program.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&process)){
    if(GetLastError()!=ERROR_ELEVATION_REQUIRED)return false;std::wstring parameters;for(auto& a:args){if(!parameters.empty())parameters+=L' ';parameters+=(nsis?a:Quote(a));}SHELLEXECUTEINFOW shell{sizeof(shell)};shell.fMask=SEE_MASK_NOCLOSEPROCESS;shell.lpVerb=L"runas";shell.lpFile=program.c_str();shell.lpParameters=parameters.c_str();shell.nShow=wait?SW_HIDE:SW_SHOWNORMAL;if(!ShellExecuteExW(&shell))return false;process.hProcess=shell.hProcess;
}if(process.hThread)CloseHandle(process.hThread);bool ok=true;if(wait){ok=WaitForSingleObject(process.hProcess,300000)==WAIT_OBJECT_0;DWORD code=1;if(ok)GetExitCodeProcess(process.hProcess,&code);ok=ok&&code==0;}CloseHandle(process.hProcess);return ok;}
#else
bool Run(const fs::path& program,const std::vector<std::string>& args,bool wait=true){pid_t child=fork();if(child<0)return false;if(child==0){std::vector<char*> argv;auto name=program.string();argv.push_back(name.data());for(auto& a:args)argv.push_back(const_cast<char*>(a.c_str()));argv.push_back(nullptr);execvp(name.c_str(),argv.data());_exit(127);}if(!wait)return true;int status=0;while(waitpid(child,&status,0)<0){if(errno!=EINTR)return false;}return WIFEXITED(status)&&WEXITSTATUS(status)==0;}
#endif
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--self-test"){
#ifdef _WIN32
        if(Quote(L"C:\\a b\\")!=L"\"C:\\a b\\\\\""||Quote(L"a\"b")!=L"\"a\\\"b\"")return 1;
#endif
        return 0;
    }
    fs::path package,executable;std::string digest,mode;unsigned long pid=0;
#ifdef _WIN32
    int count=0;LPWSTR* arguments=CommandLineToArgvW(GetCommandLineW(),&count);if(count!=6){LocalFree(arguments);return 2;}pid=std::stoul(arguments[1]);package=arguments[2];auto wide=std::wstring(arguments[3]);digest=std::string(wide.begin(),wide.end());executable=arguments[4];wide=arguments[5];mode=std::string(wide.begin(),wide.end());LocalFree(arguments);
#else
    if(argc!=6)return 2;pid=std::stoul(argv[1]);package=fs::u8path(argv[2]);digest=argv[3];executable=fs::u8path(argv[4]);mode=argv[5];
#endif
    try{
        if(!package.is_absolute()||!executable.is_absolute()||digest.size()!=64||(mode!="restart"&&mode!="close"))throw std::runtime_error("Invalid update arguments");
        if(Hash(package)!=digest)throw std::runtime_error("Update checksum mismatch");
#ifdef _WIN32
        HANDLE parent=OpenProcess(SYNCHRONIZE,FALSE,pid);if(parent){auto result=WaitForSingleObject(parent,120000);CloseHandle(parent);if(result!=WAIT_OBJECT_0)throw std::runtime_error("Application still running");}
        if(executable.filename()!=L"Noty.exe"||executable.parent_path().filename()!=L"bin")throw std::runtime_error("Unknown installation layout");
        auto root=executable.parent_path().parent_path();if(!Run(package,{L"/S",L"/D="+root.wstring()},true,true))throw std::runtime_error("Installer failed");
#elif defined(__APPLE__)
        for(int i=0;i<1200&&kill(pid,0)==0;i++)std::this_thread::sleep_for(std::chrono::milliseconds(100));if(kill(pid,0)==0)throw std::runtime_error("Application still running");
        auto app=executable.parent_path().parent_path().parent_path();if(app.filename()!="Noty.app")throw std::runtime_error("Unknown application layout");auto mount=package.parent_path()/"mounted-update";fs::create_directories(mount);if(!Run("/usr/bin/hdiutil",{"attach",package.string(),"-nobrowse","-readonly","-mountpoint",mount.string()}))throw std::runtime_error("DMG mount failed");
        auto pending=app;pending+=".new";auto backup=app;backup+=".previous";try{if(fs::exists(pending)||fs::exists(backup))throw std::runtime_error("Previous update needs recovery");fs::copy(mount/"Noty.app",pending,fs::copy_options::recursive);fs::rename(app,backup);try{fs::rename(pending,app);}catch(...){fs::rename(backup,app);throw;}Run("/usr/bin/hdiutil",{"detach",mount.string()});std::error_code cleanup;fs::remove_all(backup,cleanup);}catch(...){Run("/usr/bin/hdiutil",{"detach",mount.string()});throw;}
#else
        for(int i=0;i<1200&&kill(pid,0)==0;i++)std::this_thread::sleep_for(std::chrono::milliseconds(100));if(kill(pid,0)==0)throw std::runtime_error("Application still running");
        if(executable!=fs::path("/usr/bin/Noty"))throw std::runtime_error("Automatic DEB updates require a system installation at /usr/bin/Noty");
        if(!(geteuid()==0?Run("/usr/bin/dpkg",{"-i",package.string()}):Run("pkexec",{"/usr/bin/dpkg","-i",package.string()})))throw std::runtime_error("Package installation failed or authorization canceled");
#endif
        if(mode=="restart"){
#ifdef _WIN32
            if(!Run(executable,{},false))throw std::runtime_error("Restart failed");
#elif defined(__APPLE__)
            if(!Run("/usr/bin/open",{"-n",executable.parent_path().parent_path().parent_path().string()},false))throw std::runtime_error("Restart failed");
#else
            if(!Run(executable,{},false))throw std::runtime_error("Restart failed");
#endif
        }
        fs::remove(package);std::ofstream(package.parent_path()/"update-result.txt")<<"PASS: installed verified update; user data preserved\n";return 0;
    }catch(const std::exception& error){std::ofstream(package.parent_path()/"update-result.txt")<<"FAIL: "<<error.what()<<"\n";
#ifdef _WIN32
        MessageBoxW(nullptr,L"Das Update konnte nicht installiert werden. Die bisherige Version bleibt erreichbar. Details: updates\\update-result.txt",L"Noty Update",MB_OK|MB_ICONWARNING);
#endif
        return 1;
    }
}
