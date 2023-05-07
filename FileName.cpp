#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <process.h>

class Process {
private:
    DWORD pid{};
    HANDLE handle{};

public:
    Process(const std::string& name) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 process;
            process.dwSize = sizeof(process);
            if (Process32First(snapshot, &process)) {
                do {
                    if (_wcsicmp(process.szExeFile, std::wstring(name.begin(), name.end()).c_str()) == 0) {
                        pid = process.th32ProcessID;
                        handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid);
                        break;
                    }
                } while (Process32Next(snapshot, &process));
            }
            CloseHandle(snapshot);
        }
    }

    ~Process() {
        if (handle != nullptr) {
            CloseHandle(handle);
        }
    }

    DWORD get_pid() const {
        return pid;
    }

    HANDLE get_handle() const {
        return handle;
    }
    HANDLE open_by_name(const std::string& name) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 process;
            process.dwSize = sizeof(process);
            if (Process32First(snapshot, &process)) {
                do {
                    if (_wcsicmp(process.szExeFile, std::wstring(name.begin(), name.end()).c_str()) == 0) {
                        pid = process.th32ProcessID;
                        handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid);
                        break;
                    }
                } while (Process32Next(snapshot, &process));
            }
            CloseHandle(snapshot);
        }
        return handle;
    }

};
class ResourceUsage {
private:
    HANDLE process;
    FILETIME last_time;
    ULONGLONG last_total_time;

public:
    ResourceUsage(HANDLE process) : process(process), last_time({ 0, 0 }), last_total_time(0) {}

    ~ResourceUsage() {
        CloseHandle(process);
    }

    void print_memory_usage() const {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            std::cout << "Memory usage: " << pmc.WorkingSetSize / 1024 << " KB" << std::endl;
        }
        else {
            std::cerr << "Error getting memory usage. Error code: " << GetLastError() << std::endl;
        }
    }

    void print_cpu_usage()
    {
        FILETIME idle_time, kernel_time, user_time;
        ULARGE_INTEGER idle_time_li, kernel_time_li, user_time_li;

        if (!GetSystemTimes(&idle_time, &kernel_time, &user_time))
        {
            std::cerr << "GetSystemTimes() failed with error code " << GetLastError() << std::endl;
            return;
        }

        memcpy(&idle_time_li, &idle_time, sizeof(FILETIME));
        memcpy(&kernel_time_li, &kernel_time, sizeof(FILETIME));
        memcpy(&user_time_li, &user_time, sizeof(FILETIME));

        static ULARGE_INTEGER last_idle_time_li = { 0 };
        static ULARGE_INTEGER last_kernel_time_li = { 0 };
        static ULARGE_INTEGER last_user_time_li = { 0 };

        double idle_time_diff = double(idle_time_li.QuadPart - last_idle_time_li.QuadPart);
        double kernel_time_diff = double(kernel_time_li.QuadPart - last_kernel_time_li.QuadPart);
        double user_time_diff = double(user_time_li.QuadPart - last_user_time_li.QuadPart);

        double total_cpu_time_diff = kernel_time_diff + user_time_diff;
        double idle_time_fraction = idle_time_diff / total_cpu_time_diff;

        last_idle_time_li = idle_time_li;
        last_kernel_time_li = kernel_time_li;
        last_user_time_li = user_time_li;

        std::cout << "CPU usage: " << (1.0 - idle_time_fraction) * 100.0 << "%" << std::endl;
    }


        
};
int main()
{

    std::string process_name;
    std::cout << "Enter the name of the process you want to monitor: ";
    std::getline(std::cin, process_name);

    Process process(process_name);
    DWORD pid = process.get_pid();

    if (pid == 0) {
        std::cerr << "Could not find process " << process_name << std::endl;
        return 1;
    }

    HANDLE handle = process.open_by_name(process_name);

    if (handle == nullptr) {
        std::cerr << "Could not open handle to process " << process_name << ". Error code: " << GetLastError() << std::endl;
        return 1;
    }

    ResourceUsage resource_usage(handle);

    while (true) {
        resource_usage.print_memory_usage();
        resource_usage.print_cpu_usage();
        Sleep(1000); // Delay for 1 second
    }

    CloseHandle(handle);

    return 0;
}


