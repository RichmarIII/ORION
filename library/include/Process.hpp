#pragma once

namespace ORION
{
    class Process
    {
    public:
        /**
         * @brief Executes a process and waits for it to finish
         *
         * @param pPath The path to the executable
         * @param pArgs The arguments to pass to the executable
         */
        static int Execute(const char* pPath, const char* pArgs);

    private:
    };
} // namespace ORION
