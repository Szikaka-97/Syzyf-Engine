#pragma once
#include <Jolt/Jolt.h>

void TraceImpl(const char *inFMT, ...);

bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, JPH::uint inLine);
