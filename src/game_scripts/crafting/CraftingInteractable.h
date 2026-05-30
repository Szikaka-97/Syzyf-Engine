#pragma once

#include "GameObject.h"

#include <cstdint>

#include <glm/glm.hpp>

namespace Crafting{
    enum class CraftingInteractionType : std::uint32_t{
        None       = 0,
        Ingredient = 1 << 0,
        Lid        = 1 << 1,
        Blower     = 1 << 2,
        Door       = 1 << 3
    };

    using CraftingInteractionMask = std::uint32_t;

    inline CraftingInteractionMask ToMask(CraftingInteractionType type){
        return static_cast<CraftingInteractionMask>(type);
    }

    inline CraftingInteractionMask operator|(
        CraftingInteractionType left,
        CraftingInteractionType right
    ){
        return ToMask(left) | ToMask(right);
    }

    inline CraftingInteractionMask operator|(
        CraftingInteractionMask left,
        CraftingInteractionType right
    ){
        return left | ToMask(right);
    }

    inline bool HasInteraction(
        CraftingInteractionMask mask,
        CraftingInteractionType type
    ){
        return (mask & ToMask(type)) != 0;
    }

    class CraftingInteractable : public GameObject{
    public:
        CraftingInteractionType type = CraftingInteractionType::None;
        bool interactionEnabled = true;
        glm::vec3 fallbackHalfExtents = glm::vec3(0.35f, 0.35f, 0.35f);

        bool IsInteractionEnabled() const{
            return interactionEnabled;
        }

        void SetInteractionEnabled(bool enabled){
            interactionEnabled = enabled;
        }

        bool MatchesMask(CraftingInteractionMask mask) const{
            return interactionEnabled && HasInteraction(mask,type);
        }
    };
}