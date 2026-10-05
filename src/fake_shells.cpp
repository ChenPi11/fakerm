#include "fake_shells.hpp"
#include "utils.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

constexpr const char *EXIT_FILE = "/tmp/.fsh_exit";

// 检测输入中是否包含会导致多命令执行的分隔符
static bool has_compound_separator(const std::string &input, std::size_t &pos)
{
    pos = std::string::npos;
    const char *seps = ";|&`";
    std::size_t p1 = input.find_first_of(seps);
    std::size_t p2 = input.find("$(");
    if (p1 != std::string::npos) pos = p1;
    if (p2 != std::string::npos && (pos == std::string::npos || p2 < pos)) pos = p2;
    return pos != std::string::npos;
}

// 执行第一个命令，并对后续命令显示 not found
static void exec_first_only(const std::string &input, ShellType st)
{
    std::size_t pos = std::string::npos;
    if (!has_compound_separator(input, pos))
    {
        std::system(input.c_str());
        return;
    }

    // 只执行分隔符之前的部分
    std::string first_cmd = input.substr(0, pos);
    std::system(first_cmd.c_str());

    // 提取分隔符之后的下一个命令名，显示 not found
    std::size_t start = input.find_first_not_of(" \t;|&`$(", pos);
    if (start == std::string::npos) return;

    std::size_t end = input.find_first_of(" \t;|&`$(", start);
    std::string next_cmd = input.substr(start, end - start);
    if (next_cmd.empty()) return;

    if (st == SHELLTYPE_SH)
        std::fprintf(stderr, "sh: %s: not found\n", next_cmd.c_str());
    else
        std::fprintf(stderr, "%s: command not found\n", next_cmd.c_str());
}

// 输入 wtf 时直接退出整个程序
static void exit_on_wtf()
{
    std::printf("just a joke.\n");
    std::printf("exit scheduled.\n");
    std::fflush(stdout);
    std::exit(EXIT_SUCCESS);
}

void deal_export(const std::string &var, ShellType st, std::size_t line_count = -1)
{
    if (var.find('=') != std::string::npos)
    {
        std::string name = var.substr(0, var.find('='));
        std::string value = var.substr(var.find('=') + 1);
        setenv(name.c_str(), value.c_str(), 1);
    }
    else if (!var.empty())
    {
        for (char **env = environ; *env != nullptr; env++)
        {
            char *env_value = getenv(*env);
            if (env_value)
            {
                if (std::string(env_value).find(' ') != std::string::npos)
                {
                    std::fprintf(stdout, "export %s=\'%s\'\n", *env, env_value);
                }
                else
                {
                    std::fprintf(stdout, "export %s=%s\n", *env, env_value);
                }
            }
            else
            {
                std::fprintf(stdout, "export %s\n", *env);
            }
        }
    }
    else
    {
        if (st == SHELLTYPE_SH)
        {
            std::fprintf(stderr, "sh: %zu: export: `%s': bad variable name\n", line_count, var.c_str());
        }
        else if (st == SHELLTYPE_BASH)
        {
            std::fprintf(stderr, "bash: export: `%s': not a valid identifier\n", var.c_str());
        }
        else if (st == SHELLTYPE_ZSH)
        {
            std::fprintf(stderr, "zsh: bad assignment\n");
        }
        else
        {
            std::abort();
        }
    }
}

void fake_sh(const std::string &ps1)
{
    std::size_t line_count = 0;
    while (true)
    {
        std::fprintf(stderr, "%s", ps1.c_str());
        std::string input;
        std::getline(std::cin, input);
        std::string command = input.substr(0, input.find(' '));
        setenv("_", command.c_str(), 1);
        if (command == "command")
        {
            if (input.find(' ') == std::string::npos)
            {
                command = "";
            }
            else
            {
                command = input.substr(input.find(' ') + 1);
            }
        }
        command = strip(command);
        line_count++;
        if (command == "wtf")
        {
            exit_on_wtf();
        }
        else if (command == "alias" || command == "echo")
        {
            exec_first_only(input, SHELLTYPE_SH);
        }
        else if (command == "bg")
        {
            std::fprintf(stderr, "sh: %zu: bg: No current job\n", line_count);
        }
        else if (command == "fg")
        {
            std::fprintf(stderr, "sh: %zu: fg: No current job\n", line_count);
        }
        else if (command == "cd")
        {
            std::string arg = strip(input.substr(input.find(' ') + 1));
            if (input.find(' ') == std::string::npos)
            {
                arg = "";
            }
            std::filesystem::path path;
            if (arg.empty())
            {
                char *home_env = getenv("home");
                std::string home;
                if (home_env)
                {
                    home = home_env;
                }
                else
                {
                    home = "/root";
                }
                path = std::filesystem::path(home).make_preferred();
            }
            else
            {
                path = std::filesystem::path(arg).make_preferred();
            }
            if (path == std::filesystem::path("/"))
            {
                chdir("/");
            }
            else
            {
                std::fprintf(stderr, "sh: %zu: cd: can't cd to %s\n", line_count, path.c_str());
            }
        }
        else if (command == "export")
        {
            deal_export(strip(input.substr(input.find(' ') + 1)), SHELLTYPE_SH, line_count);
        }
        else if (command == "unset")
        {
            unsetenv(strip(input.substr(input.find(' ') + 1)).c_str());
        }
        else if (command == "pwd")
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr)
            {
                std::printf("%s\n", cwd);
            }
            else
            {
                chdir("/");
                std::printf("/\n");
            }
        }
        else if (command == "exit" || command == "logout" || std::cin.eof())
        {
            break;
        }
        else if (command.empty())
        {
            line_count--;
            continue;
        }
        else
        {
            std::fprintf(stderr, "sh: %zu: %s: not found\n", line_count, command.c_str());
        }
    }
    std::printf("\n");
}

void fake_bash_sh()
{
    fake_sh("sh-5.2# ");
}

void fake_bash()
{
    while (true)
    {
        if (is_debian())
        {
            std::string user = get_current_user_name();
            std::string host = get_current_host_name();
            std::string cwd = get_current_working_directory();
            std::fprintf(stderr, "%s@%s:%s# ", user.c_str(), host.c_str(), cwd.c_str());
        }
        else
        {
            std::string user = get_current_user_name();
            std::string host = get_current_host_name();
            std::string cwd = get_current_working_directory();
            std::fprintf(stderr, "[%s@%s %s]# ", user.c_str(), host.c_str(), cwd.c_str());
        }
        std::string input;
        std::getline(std::cin, input);
        std::string command = input.substr(0, input.find(' '));
        setenv("_", command.c_str(), 1);
        if (command == "command")
        {
            if (input.find(' ') == std::string::npos)
            {
                command = "";
            }
            else
            {
                command = input.substr(input.find(' ') + 1);
            }
        }
        command = strip(command);
        if (command == "wtf")
        {
            exit_on_wtf();
        }
        else if (command == "alias" || command == "echo")
        {
            exec_first_only(input, SHELLTYPE_BASH);
        }
        else if (command == "bg")
        {
            std::fprintf(stderr, "bash: bg: current: no such job\n");
        }
        else if (command == "fg")
        {
            std::fprintf(stderr, "bash: fg: current: no such job\n");
        }
        else if (command == "cd")
        {
            std::string arg = strip(input.substr(input.find(' ') + 1));
            if (input.find(' ') == std::string::npos)
            {
                arg = "";
            }
            std::filesystem::path path;
            if (arg.empty())
            {
                char *home_env = getenv("home");
                std::string home;
                if (home_env)
                {
                    home = home_env;
                }
                else
                {
                    home = "/root";
                }
                path = std::filesystem::path(home).make_preferred();
            }
            else
            {
                path = std::filesystem::path(arg).make_preferred();
            }
            if (path == std::filesystem::path("/"))
            {
                chdir("/");
            }
            else
            {
                std::fprintf(stderr, "bash: cd: %s: No such file or directory\n", path.c_str());
            }
        }
        else if (command == "export")
        {
            deal_export(strip(input.substr(input.find(' ') + 1)), SHELLTYPE_BASH);
        }
        else if (command == "unset")
        {
            unsetenv(strip(input.substr(input.find(' ') + 1)).c_str());
        }
        else if (command == "clear")
        {
            std::printf("\033[H\033[J");
            std::fflush(stdout);
        }
        else if (command == "pwd")
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr)
            {
                std::printf("%s\n", cwd);
            }
            else
            {
                chdir("/");
                std::printf("/\n");
            }
        }
        else if (command == "help")
        {
            exec_first_only(input, SHELLTYPE_BASH);
        }
        else if (command == "exit" || command == "logout" || std::cin.eof())
        {
            break;
        }
        else if (command.empty())
        {
            continue;
        }
        else
        {
            std::fprintf(stderr, "%s: command not found\n", command.c_str());
        }
    }
    std::printf("\n");
}

void fake_zsh()
{
    while (true)
    {
        std::string host = get_current_host_name();
        std::fprintf(stderr, "%s# ", host.c_str());
        std::string input;
        std::getline(std::cin, input);
        std::string command = input.substr(0, input.find(' '));
        setenv("_", command.c_str(), 1);
        if (command == "command")
        {
            if (input.find(' ') == std::string::npos)
            {
                command = "";
            }
            else
            {
                command = input.substr(input.find(' ') + 1);
            }
        }
        command = strip(command);
        if (command == "wtf")
        {
            exit_on_wtf();
        }
        else if (command == "alias" || command == "echo")
        {
            exec_first_only(input, SHELLTYPE_ZSH);
        }
        else if (command == "bg")
        {
            std::fprintf(stderr, "bg: no current job\n");
        }
        else if (command == "fg")
        {
            std::fprintf(stderr, "fg: no current job\n");
        }
        else if (command == "cd")
        {
            std::string arg = strip(input.substr(input.find(' ') + 1));
            if (input.find(' ') == std::string::npos)
            {
                arg = "";
            }
            std::filesystem::path path;
            if (arg.empty())
            {
                char *home_env = getenv("home");
                std::string home;
                if (home_env)
                {
                    home = home_env;
                }
                else
                {
                    home = "/root";
                }
                path = std::filesystem::path(home).make_preferred();
            }
            else
            {
                path = std::filesystem::path(arg).make_preferred();
            }
            if (path == std::filesystem::path("/"))
            {
                chdir("/");
            }
            else
            {
                std::fprintf(stderr, "cd: no such file or directory: %s\n", path.c_str());
            }
        }
        else if (command == "export")
        {
            deal_export(strip(input.substr(input.find(' ') + 1)), SHELLTYPE_ZSH);
        }
        else if (command == "unset")
        {
            unsetenv(strip(input.substr(input.find(' ') + 1)).c_str());
        }
        else if (command == "clear")
        {
            std::printf("\033[H\033[J");
            std::fflush(stdout);
        }
        else if (command == "pwd")
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr)
            {
                std::printf("%s\n", cwd);
            }
            else
            {
                chdir("/");
                std::printf("/\n");
            }
        }
        else if (command == "exit" || command == "logout" || std::cin.eof())
        {
            break;
        }
        else if (command.empty())
        {
            continue;
        }
        else
        {
            std::fprintf(stderr, "%s: command not found\n", command.c_str());
        }
    }
    std::printf("\n");
}

void fake_shell()
{
    std::signal(SIGINT, SIG_IGN);
    std::string parent_proc = get_self_parent_proc_name();
    if (startswith(parent_proc, "sh"))
    {
        fake_sh();
    }
    else if (startswith(parent_proc, "bash"))
    {
        fake_bash();
    }
    else if (startswith(parent_proc, "zsh"))
    {
        fake_zsh();
    }
    else if (getenv("SHELL"))
    {
        std::string shell = getenv("SHELL");
        if (startswith(shell, "/bin/sh") || startswith(shell, "/usr/bin/sh") || startswith(shell, "/bin/dash"))
        {
            fake_sh();
        }
        else if (startswith(shell, "/bin/bash") || startswith(shell, "/usr/bin/bash") ||
                 startswith(shell, "/usr/local/bin/bash"))
        {
            fake_bash();
        }
        else if (startswith(shell, "/bin/zsh") || startswith(shell, "/usr/bin/zsh") ||
                 startswith(shell, "/usr/local/bin/zsh"))
        {
            fake_zsh();
        }
        else
        {
            fake_sh();
        }
    }
    else
    {
        fake_sh();
    }

    // 兜底：如果假 shell 是通过 exit / logout / EOF 退出的，
    // 这里会继续等待 EXIT_FILE（目前不会被创建），保持原有行为不变。
    while (1)
    {
        usleep(3000000);
        if (access(EXIT_FILE, F_OK) == 0)
        {
            break;
        }
        std::printf("/usr/sbin/login: %s\n", std::strerror(ENOENT));
    }
    std::remove(EXIT_FILE);
    std::printf("\n");
    std::exit(EXIT_SUCCESS);
}
