#include "GUID.hpp"
#include <chrono>  // std::chrono::system_clock::now
#include <iomanip> // std::setw
#include <random>  // std::random_device, std::mt19937, std::uniform_int_distribution
#include <sstream> // std::stringstream

using namespace ORION;

GUID GUID::Generate()
{
    // Get the current time as a timestamp
    const auto TIME_NOW          = std::chrono::system_clock::now().time_since_epoch();
    const auto TIME_SECONDS      = std::chrono::duration_cast<std::chrono::seconds>(TIME_NOW).count();
    const auto TIME_NANO_SECONDS = std::chrono::duration_cast<std::chrono::nanoseconds>(TIME_NOW).count();

    // Generate a random number
    std::random_device              RandomDevice;
    std::mt19937                    RandomGenerator(RandomDevice());
    std::uniform_int_distribution<> DISTRIBUTION(0, 0xFFFF);

    // UUID version 4 Variant 1
    std::stringstream GUIDStringStream;

    // 8-4-4-4-12
    GUIDStringStream << std::hex << std::setw(8) << TIME_SECONDS;
    GUIDStringStream << "-" << std::setw(4) << (TIME_SECONDS >> 32);
    GUIDStringStream << "-" << std::setw(4) << ((TIME_SECONDS >> 48) | 0x4000);          // The 4 in "4xxx" indicates the UUID version
    GUIDStringStream << "-" << std::setw(4) << (DISTRIBUTION(RandomGenerator) | 0x8000); // The 8 in "8xxx" indicates the variant
    GUIDStringStream << "-" << std::setw(12) << TIME_NANO_SECONDS;

    return {GUIDStringStream.str()};
}