#pragma once

#include <string>

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)
#define _DIRTY_APPLE 1
#else
#define _DIRTY_APPLE 0
#endif

bool startswith(const std::string &str, const std::string &prefix);

bool is_debian();

std::string strip(const std::string& str);

std::string str_format(std::string_view fmt, ...);

std::string get_self_parent_proc_name();

std::string get_current_user_name();

std::string get_current_host_name();

std::string get_current_working_directory();

int randint(int min, int max);

int rand_decision(int probability);
