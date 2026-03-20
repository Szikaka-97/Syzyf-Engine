#pragma once
#include <Jolt/Jolt.h>

namespace Physics {
void TraceImpl(const char *inFMT, ...);

bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, JPH::uint inLine);
}
