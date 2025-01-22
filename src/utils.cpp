#include "utils.hpp"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <pwd.h>
#include <system_error>
#include <unistd.h>
#include <stdarg.h>

#if _DIRTY_APPLE
#include <libproc.h>
#endif

bool startswith(const std::string &str, const std::string &prefix)
{
    if (str.size() < prefix.size())
    {
        return false;
    }

    for (std::size_t i = 0; i < prefix.size(); i++)
    {
        if (str[i] != prefix[i])
        {
            return false;
        }
    }

    return true;
}

bool is_debian()
{
    bool ret;
    // Check if the `/etc/debian_version file` exists.
    ret = access("/etc/debian_version", F_OK) == 0;

    // Check if the `/usr/bin/apt` exists.
    ret |= access("/usr/bin/apt", F_OK) == 0;

    return ret;
}

std::string strip(const std::string &str)
{
    std::string ret = str;
    ret.erase(ret.find_last_not_of(" \n\r\t") + 1);
    ret.erase(0, ret.find_first_not_of(" \n\r\t"));
    return ret;
}

std::string str_format(std::string_view fmt, ...)
{
    int size = snprintf(nullptr, 0, fmt.data()) + 1; // NOLINT: Allow non string literal formatting.
    if (size <= 0)
    {
        return "";
    }

    std::unique_ptr<char[]> buf(new char[size]);
    memset(buf.get(), 0, size);

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf.get(), size, fmt.data(), args);
    va_end(args);

    return std::string(buf.get(), buf.get() + size - 1);
}

// We trust we have enough space or the file is small.
std::string readall(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs)
    {
        throw std::system_error(errno, std::generic_category(), "Failed to open file: " + path);
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    return content;
}

std::string get_self_parent_proc_name()
{
    #if _DIRTY_APPLE
    struct proc_bsdinfo info;
    int st = proc_pidinfo(getppid(), PROC_PIDTBSDINFO, 0, &info, PROC_PIDTBSDINFO_SIZE);
    if (st != PROC_PIDTBSDINFO_SIZE)
    {
        return "/bin/sh";
    }

    return { info.pbi_name };
    #else
    return strip(readall(str_format("/proc/%d/comm", getppid())));
    #endif
}

std::string get_current_user_name()
{
    struct passwd *pw = getpwuid(geteuid());
    if (pw)
    {
        return pw->pw_name;
    }

    return "root";
}

std::string get_current_host_name()
{
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));
    return hostname;
}

std::string get_current_working_directory()
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr)
    {
        return cwd;
    }

    return "/";
}

int randint(int min, int max)
{
    return min + rand() % (max - min + 1);
}

int rand_decision(int probability)
{
    return randint(0, 10) > probability;
}
