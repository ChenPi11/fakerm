#pragma once

#include <string>

enum ShellType
{
    SHELLTYPE_UNKNOWN,
    SHELLTYPE_SH,
    SHELLTYPE_BASH,
    SHELLTYPE_ZSH,
};

void fake_sh(const std::string& ps1 = "# "); // Like dash, ash, hush, etc.

void fake_bash_sh(); // Bash alias to `sh`.

void fake_bash();

void fake_zsh();

void fake_shell();
