#pragma once

#ifdef __SERIALIZER_RUNNING__
#define serialized __attribute__((annotate("__serialized__")))
#define not_serialized __attribute__((annotate("__not_serialized__")))
#else
#define serialized
#define not_serialized
#endif

#include <nlohmann/json_fwd.hpp>
#include <vector>

using json = nlohmann::json;

class DoNotSerialize { };

struct SerializedReference {
	void** field_ptr;
	char obj_uuid[36];
};
