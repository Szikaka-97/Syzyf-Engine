#pragma once
#include "GameObject.h"

class PickableItem : public GameObject {
public:
    virtual void OnPickUp() {}
};
