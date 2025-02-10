#pragma once

#include "Jolt/Jolt.h"

#include <iostream>

namespace mk
{

#ifdef JPH_ENABLE_ASSERTS

    // Callback for asserts, connect this to your own assert handler if you have one
    static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, JPH::uint inLine)
    {
        // Print to the TTY
        std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;

        // Breakpoint
        return true;
    };

#endif // JPH_ENABLE_ASSERTS

}