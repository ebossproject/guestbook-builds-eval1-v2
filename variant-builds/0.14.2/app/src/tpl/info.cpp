#include "info.hpp"

#include "../assert.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>

#include <crow/json.h>
#include <crow/mustache.h>

using namespace tpl;

namespace
{
    // Helper function to read process start time from /proc
    std::string get_process_start_time(pid_t pid)
    {
        std::string path = "/proc/" + std::to_string(pid) + "/stat";
        std::ifstream stat_file(path);
        if (!stat_file.is_open())
        {
            return "Unknown";
        }

        std::string line;
        std::getline(stat_file, line);

        // Parse stat file to get start time (field 22)
        std::istringstream iss(line);
        std::string value;
        unsigned long long starttime = 0;

        for (int i = 1; i <= 22; i++)
        {
            if (!(iss >> value))
            {
                return "Unknown";
            }
            if (i == 22)
            {
                starttime = std::stoull(value);
            }
        }

        // Get system uptime
        std::ifstream uptime_file("/proc/uptime");
        double uptime = 0;
        if (uptime_file >> uptime)
        {
            // Convert clock ticks to seconds since boot
            long clock_ticks = sysconf(_SC_CLK_TCK);
            double seconds_since_boot = static_cast<double>(starttime) / clock_ticks;

            // Calculate start time
            time_t now = time(nullptr);
            time_t start = now - uptime + seconds_since_boot;

            char buffer[100];
            struct tm *tm_info = localtime(&start);
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
            return buffer;
        }

        return "Unknown";
    }

    // Helper function to get process memory usage
    std::string get_memory_usage(pid_t pid)
    {
        std::string path = "/proc/" + std::to_string(pid) + "/status";
        std::ifstream status_file(path);
        if (!status_file.is_open())
        {
            return "Unknown";
        }

        std::string line;
        while (std::getline(status_file, line))
        {
            if (line.substr(0, 6) == "VmRSS:")
            {
                return line.substr(6);
            }
        }

        return "Unknown";
    }
}

::crow::response Info::render()
{
    auto t = crow::mustache::load("info.html.mustache");
    crow::mustache::context ctx;

    // Get process information
    pid_t pid = getpid();
    pid_t ppid = getppid();

    // Add process info to context
    ctx["pid"] = std::to_string(pid);
    ctx["ppid"] = std::to_string(ppid);

    // Get process title (simplified - typically would use prctl)
    std::string process_title;
    std::ifstream cmdline_file("/proc/self/cmdline");
    if (cmdline_file.is_open())
    {
        std::getline(cmdline_file, process_title, '\0');
    }
    ctx["process_title"] = process_title.empty() ? "Unknown" : process_title;

    // Get process start time
    std::string start_time = get_process_start_time(pid);
    ctx["process_start_time"] = start_time;

    // Calculate uptime
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    struct tm time_info;
    localtime_r(&now_t, &time_info);
    char time_str[100];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &time_info);

    // Simplified uptime calculation
    ctx["process_uptime"] = "Since process start"; // Actual calculation would require tracking start time

    // Get memory usage
    ctx["process_memory_usage"] = get_memory_usage(pid);

    ctx["__dump_env__"] = "not implemented";

    return {t.render(ctx)};
}