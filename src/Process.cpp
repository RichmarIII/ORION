#include "Process.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace ORION;

#if _WIN32
    #include <Windows.h>
#else  // _WIN32
    #include <sys/wait.h>
    #include <unistd.h>
#endif // _WIN32

int Process::Execute(const char* pPath, const char* pArgs)
{
#if _WIN32
#else  // _WIN32
    // Create a child process to execute the command and wait for it to finish
    if (const pid_t PROCESS_ID = fork(); PROCESS_ID == 0)
    {
        // execute the command with sh -c
        execl("/bin/sh", "sh", "-c", pPath, pArgs, nullptr);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        int Status;
        waitpid(PROCESS_ID, &Status, 0);

        if (WIFEXITED(Status))
        {
            // Child process exited normally
            if (const int EXIT_CODE = WEXITSTATUS(Status); EXIT_CODE != 0)
            {
                // Child process exited with an error
                std::cerr << "Child process exited with code " << EXIT_CODE << std::endl;

                return EXIT_CODE;
            }
            else
            {
                std::cout << "Child process exited normally" << std::endl;

                return EXIT_CODE;
            }
        }

        // Child process did not exit normally
        std::cerr << "Child process did not exit normally" << std::endl;

        return EXIT_FAILURE;
    }
#endif // _WIN32
}
