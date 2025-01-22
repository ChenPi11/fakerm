#include "fake_shells.hpp"
#include "utils.hpp"

#include <csignal>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <system_error>
#include <unistd.h>

static void visit(const std::string dir)
{
    DIR *d;
    struct dirent *dir_info;
    std::string path;

    if ((d = opendir(dir.c_str())) != NULL)
    {
        while ((dir_info = readdir(d)) != NULL)
        {
            if (std::string(dir_info->d_name) == "." || std::string(dir_info->d_name) == "..")
            {
            }
            else if (dir_info->d_type == DT_DIR)
            {
                path = dir + "/" + dir_info->d_name;
                visit(path);
            }
            else
            {
                path = dir + "/" + dir_info->d_name;
                if (startswith(path, "/dev"))
                {
                    if (rand_decision(1))
                    {
                        std::fprintf(stderr, "rm: cannot remove '%s': %s\n", path.c_str(), std::strerror(EBUSY));
                    }
                    else
                    {
                        std::fprintf(stderr, "rm: cannot remove '%s': %s\n", path.c_str(), std::strerror(EPERM));
                    }
                }
                else if (startswith(path, "/sys"))
                {
                    std::fprintf(stderr, "rm: cannot remove '%s': %s\n", path.c_str(), strerror(EPERM));
                }
                else if (startswith(path, "/proc"))
                {
                    if (rand_decision(3))
                    {
                        std::fprintf(stderr, "rm: cannot remove '%s': %s\n", path.c_str(), strerror(EACCES));
                    }
                    else
                    {
                        std::fprintf(stderr, "rm: cannot remove '%s': %s\n", path.c_str(), strerror(EPERM));
                    }
                }
            }
        }
        closedir(d);
    }
}

void sig_handler(int sig)
{
    fake_shell();
}

int main(int argc, char *argv[])
{
    bool enter_shell = true;
    std::srand((unsigned int)std::time(NULL));
    std::signal(SIGINT, sig_handler);
    try
    {
        if (argc > 1)
        {
            if (std::string(argv[1]) == "--pkg-manager-mode")
            {
                enter_shell = false;
            }
        }

        usleep(randint(5000, 10000));
        visit("/dev");
        sleep(2);
        #if !_DIRTY_APPLE
        visit("/proc");
        sleep(3);
        visit("/sys");
        #endif

        if (enter_shell)
        {
            fake_shell();
        }

        return EXIT_SUCCESS;
    }
    catch (const std::system_error &e)
    {
        std::string msg = "fakrm: " + std::string(e.what()) + ": " + e.code().message();
        std::fprintf(stderr, "%s\n", msg.c_str());
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "fakrm: %s\n", e.what());
    }
    catch (...)
    {
        std::fprintf(stderr, "fakrm: unknown error\n");
    }

    return EXIT_FAILURE;
}
