#pragma once
#include "GameObject.h"

class PickableItem : public GameObject {
public:
    void OnPickUp() {
        spdlog::info("Picked up an item");
    }
};
