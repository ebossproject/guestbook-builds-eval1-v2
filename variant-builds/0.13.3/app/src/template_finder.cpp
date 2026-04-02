#include <unistd.h>
#include <limits.h>
#include <fstream>
#include <filesystem>
#include <vector>

#include "template_finder.hpp"

std::string get_executable_path() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        result[count] = '\0';
        return std::string(result);
    }
    return "";
}

std::string template_finder::find_template_path() {
    if (const char *env = std::getenv("TEMPLATE_PATH")) {
        return std::string{env};
    }
    
    // Get the executable directory
    std::string exe_path = get_executable_path();
    if (!exe_path.empty()) {
        std::filesystem::path bin_path(exe_path);
        std::filesystem::path exe_dir = bin_path.parent_path();
        
        // Common installation layout: templates in ../share/guestbook/ relative to binary
        std::filesystem::path share_path = exe_dir.parent_path() / "share" / "guestbook";
        if (std::filesystem::exists(share_path / "index.html.mustache")) {
            return share_path.string() + "/";
        }
    }
    
    // Check standard installation locations
    std::vector<std::string> possible_paths = {
        "./views/",                       // Local development path
        "./templates/",                   // Current fallback
        "/usr/local/share/guestbook/",    // Standard local installation
        "/usr/share/guestbook/"           // System-wide installation
    };
    
    for (const auto& path : possible_paths) {
        // Check if a template file exists
        if (std::filesystem::exists(path + "index.html.mustache")) {
            return path;
        }
    }
    
    // Default fallback
    return "./templates/";
}
