#include "physics/PhysicsJolt.h"

#include <cstdarg>
#include <spdlog/spdlog.h>

// Callback for traces, connect this to your own trace function if you have one
void TraceImpl(const char *inFMT, ...) {
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    // Print to the TTY
    spdlog::info(buffer);
  }

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine) {
  // Print to the TTY
  spdlog::info("{}: {}: ({}) {}", inFile, inLine, inExpression, (inMessage != nullptr ? inMessage : ""));
  // Breakpoint
  return true;
};

#endif // JPH_ENABLE_ASSERTS
