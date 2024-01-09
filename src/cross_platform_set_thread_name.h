// Set name of a thread on major OSes.
#pragma once

#include <thread>

#ifdef _WIN32
// https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2015/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2015&redirectedfrom=MSDN
//  
// Usage: SetThreadName ((DWORD)-1, "MainThread");  
//  
#include <windows.h>  
const DWORD MS_VC_EXCEPTION = 0x406D1388;  
#pragma pack(push,8)  
typedef struct tagTHREADNAME_INFO  
{  
    DWORD dwType; // Must be 0x1000.  
    LPCSTR szName; // Pointer to name (in user addr space).  
    DWORD dwThreadID; // Thread ID (-1=caller thread).  
    DWORD dwFlags; // Reserved for future use, must be zero.  
 } THREADNAME_INFO;  
#pragma pack(pop)  

void SetThreadName(DWORD dwThreadID, const char* threadName) {  
    THREADNAME_INFO info;  
    info.dwType = 0x1000;  
    info.szName = threadName;  
    info.dwThreadID = dwThreadID;  
    info.dwFlags = 0;  
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
    __try{  
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);  
    }  
    __except (EXCEPTION_EXECUTE_HANDLER){  
    }  
#pragma warning(pop)  
}

auto GetThreadName(DWORD dwThreadID) -> std::string {
    PWSTR* ppszThreadDescription = nullptr;
    auto hr = ::GetThreadDescription(dwThreadID, ppszThreadDescription);
    if (SUCCEEDED(hr)) {
        std::wstring output{ppszThreadDescription};
        output.shrink_to_fit();
        LocalFree(ppszThreadDescription);
        return std::string{output.begin(), output.end()};
    }
    return {};
}

void set_thread_name(const char* thread_name) {
    SetThreadName(::GetCurrentThreadId(), thread_name);
}

void set_thread_name(std::thread& thread, const char* thread_name) {
    SetThreadName(::GetThreadId(static_cast<HANDLE>(thread.native_handle())), thread_name);
}

std::string this_thread_name() {
    return GetThreadName(::GetCurrentThreadId());
}

#elif defined(__linux__)

#include <sys/prctl.h>

void set_thread_name(const char* thread_name) {
    ::prctl(PR_SET_NAME, thread_name, 0, 0, 0);
}
std::string this_thread_name(std::thread& thread) {
    std::string name(2 << 10, '\0');
    ::prctl(PR_GET_NAME, name.data(), 0, 0, 0);
    name.shrink_to_fit();
    return name;
}
#else

#include <pthread.h>
void set_thread_name(const char* thread_name) {
    ::pthread_setname_np(thread_name);
}
std::string this_thread_name() {
    std::string name(2 << 10, '\0');
    ::pthread_getname_np(pthread_self(), &name[0], name.length());
    name.shrink_to_fit();
    return name;
}
#endif