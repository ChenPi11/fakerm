#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>

static void do_nothing(int sig)
{
    errno = sig;  // avoid unused variable warning.
    errno = 0;
    while (1)
    {
        sleep(100);
    }
}

static int startswith(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

char* get_self_parent_proc_name()
{
    pid_t ppid;

    ppid = getppid();
    char *comm_file = (char*)malloc(1024);
    snprintf(comm_file, 1024, "/proc/%d/comm", ppid);
    FILE *fp = fopen(comm_file, "r");
    if (fp == NULL)
    {
        free(comm_file);
        return NULL;
    }

    free(comm_file);

    char *proc_name = (char*)calloc(1024, 1);
    fgets(proc_name, 1024, fp);
    fclose(fp);
    return proc_name;
}

const char *get_current_user_name()
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw)
    {
        return pw->pw_name;
    }

    return "";
}


char* get_current_host_name()
{
    char *host_name = (char*)calloc(1024, 1);
    gethostname(host_name, 1024);
    return host_name;
}

int is_debian()
{
    FILE* fp;

    fp = fopen("/usr/bin/apt", "rb");
    if (fp == NULL)
    {
        return 0;
    }

    fclose(fp);

    return 1;
}

void fake_sh_prompt()
{
    const char *user_name = get_current_user_name();
    if (strcmp(user_name, "root") == 0)
    {
        fprintf(stderr, "# ");
    }
    else
    {
        fprintf(stderr, "$ ");
    }
}

void fake_bash_prompt()
{
    const char *user_name = get_current_user_name();
    char *host_name = get_current_host_name();
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL)
    {
        cwd = (char*)calloc(1, 1);
    }

    if (is_debian())
    {
        if (strcmp(user_name, "root") == 0)
        {
            fprintf(stderr, "root@%s:%s# ", host_name, cwd);
        }
        else
        {
            fprintf(stderr, "%s@%s:%s$ ", user_name, host_name, cwd);
        }
    }
    else
    {
        if (strcmp(user_name, "root") == 0)
        {
            fprintf(stderr, "[root@%s %s]# ", host_name, cwd);
        }
        else
        {
            fprintf(stderr, "[%s@%s %s]$ ", user_name, host_name, cwd);
        }
    }
    free(host_name);
    free(cwd);
}

void fake_zsh_prompt()
{
    const char *user_name = get_current_user_name();
    char *host_name = get_current_host_name();
    if (strcmp(user_name, "root") == 0)
    {
        fprintf(stderr, "%s# ", host_name);
    }
    else
    {
        fprintf(stderr, "%s%% ", host_name);
    }
}

enum ShellType
{
    SH,
    BASH,
    ZSH,
};

void fake_shell(int sig)
{
    errno = sig;  // avoid unused variable warning.
    errno = 0;

    enum ShellType shell_type = SH;
    char *parent_proc_name = get_self_parent_proc_name();
    if (parent_proc_name)
    {
        if (startswith(parent_proc_name, "bash"))
        {
            shell_type = BASH;
        }
        else if (startswith(parent_proc_name, "zsh"))
        {
            shell_type = ZSH;
        }
    }
    free(parent_proc_name);

    while (1)
    {
        char *cmd = NULL;
        size_t len = 0;
        ssize_t nread;

        switch (shell_type)
        {
        case SH:
            fake_sh_prompt();
            break;
        case BASH:
            fake_bash_prompt();
            break;
        case ZSH:
            fake_zsh_prompt();
            break;
        }

        nread = getline(&cmd, &len, stdin);
        if (nread == -1)
        {
            fprintf(stderr, "\n");
            signal(SIGINT, do_nothing);
            do_nothing(0);
        }
        if (cmd && nread > 1)
        {
            cmd[nread - 1] = '\0';
            char* first_space = strchr(cmd, ' ');
            if (first_space)
            {
                *first_space = '\0';
            }
            if (startswith(cmd, "cd"))
            {
                chdir("/");
                const char* path;
                if (first_space)
                {
                    path = first_space + 1;
                }
                else
                {
                    path = getenv("HOME");
                    if (path == NULL)
                    {
                        path = "/root";
                    }
                }
                if (strcmp(path, "/") != 0)
                {
                    switch (shell_type)
                    {
                    case SH:
                        fprintf(stderr, "sh: 1: cd: can't cd to %s\n", path);
                        break;
                    case BASH:
                        fprintf(stderr, "bash: cd: %s: No such file or directory\n", path);
                        break;
                    case ZSH:
                        fprintf(stderr, "cd: no such file or directory: %s\n", path);
                        break;
                    }
                }
            }
            else if (startswith(cmd, "wtf"))
            {
                fprintf(stderr, "jak\n");
                exit(EXIT_SUCCESS);
            }
            else
            {
                switch (shell_type)
                {
                case SH:
                    fprintf(stderr, "sh: 1: %s: not found\n", cmd);
                    break;
                case BASH:
                    fprintf(stderr, "%s: command not found\n", cmd);
                    break;
                case ZSH:
                    fprintf(stderr, "zsh: command not found: %s\n", cmd);
                    break;  
                }
            }
        }
        free(cmd);
    }
}

static int randint(int min, int max)
{
    return min + rand() % (max - min + 1);
}

static int rand_decision(int probability)
{
    return randint(0, 10) > probability;
}

static void visit(const char *dir)
{
    DIR *d;
    struct dirent *dir_info;
    char path[1024];

    if ((d = opendir(dir)) != NULL)
    {
        while ((dir_info = readdir(d)) != NULL)
        {
            if (strcmp(dir_info->d_name, ".") == 0 || strcmp(dir_info->d_name, "..") == 0)
            {
            }
            else if (dir_info->d_type == DT_DIR)
            {
                snprintf(path, sizeof(path), "%s/%s", dir, dir_info->d_name);
                visit(path);
            }
            else
            {
                snprintf(path, sizeof(path), "%s/%s", dir, dir_info->d_name);
                if (startswith(path, "/dev"))
                {
                    if (rand_decision(1))
                    {
                        fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(EBUSY));
                    }
                    else
                    {
                        fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(EPERM));
                    }
                }
                else if (startswith(path, "/sys"))
                {
                    fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(EPERM));
                }
                else if (startswith(path, "/proc"))
                {
                    if (rand_decision(3))
                    {
                        fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(EACCES));
                    }
                    else
                    {
                        fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(EPERM));
                    }
                }
            }
        }
        closedir(d);
    }
}

volatile int _nothing;

static void visit_do_nothing(const char *dir)
{
    DIR *d;
    struct dirent *dir_info;
    char path[1024];

    if ((d = opendir(dir)) != NULL)
    {
        while ((dir_info = readdir(d)) != NULL)
        {
            if (strcmp(dir_info->d_name, ".") == 0 || strcmp(dir_info->d_name, "..") == 0)
            {
            }
            else if (dir_info->d_type == DT_DIR)
            {
                snprintf(path, sizeof(path), "%s/%s", dir, dir_info->d_name);
                visit(path);
            }
        }
        closedir(d);
    }
}

int main()
{
    srand((unsigned int)time(NULL));

    signal(SIGINT, fake_shell);

    usleep(randint(5000, 10000));
    visit("/dev");
    sleep(2);
    visit("/proc");
    sleep(3);
    visit("/sys");

    visit_do_nothing("/");

    fake_shell(0);

    return EXIT_FAILURE;
}
