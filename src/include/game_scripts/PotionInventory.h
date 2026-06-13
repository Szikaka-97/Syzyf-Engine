#pragma once

#include <PersistentData.h>
#include <game_scripts/crafting/CraftingTypes.h>

#include <string>
#include <vector>

namespace PotionInventory{
    inline const std::string PotionCountKey = "PotionInventory_Count";
    inline const std::string LastRecipeNameKey = "PotionInventory_LastRecipeName";
    inline const std::string LastEffectIdKey = "PotionInventory_LastEffectId";
    inline const std::string LastSecondaryEffectIdKey = "PotionInventory_LastSecondaryEffectId";
    inline const std::string LastQualityKey = "PotionInventory_LastQuality";
    inline const std::string LastRadiusKey = "PotionInventory_LastRadius";
    inline const std::string LastDurationKey = "PotionInventory_LastDuration";
    inline const std::string LastPowerKey = "PotionInventory_LastPower";
    inline const std::string LastMainEffectCountKey = "PotionInventory_LastMainEffectCount";
    inline const std::string LastModifierCountKey = "PotionInventory_LastModifierCount";
    inline const std::string FirstPotionCreatedKey = "Crafting_FirstPotionCreated";
    inline const std::string ShowTutorialFinishedMessageKey = "Crafting_ShowTutorialFinishedMessage";

    inline const std::string IngredientInventoryInitializedKey = "IngredientInventory_Initialized";

    inline const std::string IngredientSugarBurnKey = "IngredientInventory_SugarBurn";
    inline const std::string IngredientDriedBeetBurnKey = "IngredientInventory_DriedBeetBurn";
    inline const std::string IngredientHoneyRangeKey = "IngredientInventory_HoneyRange";
    inline const std::string IngredientRatTailRangeKey = "IngredientInventory_RatTailRange";
    inline const std::string IngredientWaterLightningKey = "IngredientInventory_WaterLightning";
    inline const std::string IngredientDriedPotatoLightningKey = "IngredientInventory_DriedPotatoLightning";
    inline const std::string IngredientBoneDurationKey = "IngredientInventory_BoneDuration";
    inline const std::string IngredientDeserterEarPowerKey = "IngredientInventory_DeserterEarPower";

    inline const std::string IngredientBurnKey = IngredientRatTailRangeKey;
    inline const std::string IngredientLightningKey = IngredientWaterLightningKey;
    inline const std::string IngredientRadiusKey = IngredientSugarBurnKey;
    inline const std::string IngredientDurationKey = IngredientBoneDurationKey;

    struct IngredientInventoryEntry{
        std::string inventoryKey;
        std::string displayName;
        std::string modelPath;
        Crafting::IngredientData data;
    };

    inline int GetPotionCount(){
        return PersistentData::Get<int>(PotionCountKey);
    }

    inline void SetPotionCount(int count){
        if (count < 0){
            count = 0;
        }

        PersistentData::Set<int>(PotionCountKey,count);
    }

    inline void AddPotions(int count){
        if (count <= 0){
            return;
        }

        SetPotionCount(GetPotionCount() + count);
    }

    inline bool HasPotion(){
        return GetPotionCount() > 0;
    }

    inline bool ConsumePotion(){
        int count = GetPotionCount();

        if (count <= 0){
            return false;
        }

        SetPotionCount(count - 1);
        return true;
    }

    inline void SaveLastCraftedPotion(
        const Crafting::CraftedPotionData& potionData,
        int bottleCount,
        bool countAsCraftedPotion = true
    ){
        AddPotions(bottleCount);

        PersistentData::Set<std::string>(LastRecipeNameKey,potionData.recipeName);
        PersistentData::Set<std::string>(LastEffectIdKey,potionData.primaryEffectId);
        PersistentData::Set<std::string>(LastSecondaryEffectIdKey,potionData.secondaryEffectId);
        PersistentData::Set<float>(LastQualityKey,potionData.qualityPercent);
        PersistentData::Set<float>(LastRadiusKey,potionData.radius);
        PersistentData::Set<float>(LastDurationKey,potionData.duration);
        PersistentData::Set<float>(LastPowerKey,potionData.power);
        PersistentData::Set<int>(LastMainEffectCountKey,potionData.mainEffectCount);
        PersistentData::Set<int>(LastModifierCountKey,potionData.modifierCount);

        if (countAsCraftedPotion && !PersistentData::Get<bool>(FirstPotionCreatedKey)){
            PersistentData::Set<bool>(FirstPotionCreatedKey,true);
            PersistentData::Set<bool>(ShowTutorialFinishedMessageKey,true);
        }
    }

    inline void SaveLastCraftedPotion(
        const std::string& recipeName,
        const std::string& effectId,
        float quality,
        int bottleCount,
        bool countAsCraftedPotion = true
    ){
        Crafting::CraftedPotionData potionData;

        potionData.recipeName = recipeName;
        potionData.primaryEffectId = effectId;
        potionData.secondaryEffectId = Crafting::EffectId::None;
        potionData.qualityPercent = quality;
        potionData.mainEffectCount = 1;
        potionData.modifierCount = 0;
        potionData.radius = 3.0f;
        potionData.duration = 4.0f;
        potionData.power = 25.0f;

        SaveLastCraftedPotion(potionData,bottleCount,countAsCraftedPotion);
    }

    inline Crafting::CraftedPotionData GetLastCraftedPotion(){
        Crafting::CraftedPotionData potionData;

        potionData.recipeName = PersistentData::Get<std::string>(LastRecipeNameKey);

        if (potionData.recipeName.empty()){
            potionData.recipeName = "Potion";
        }

        potionData.primaryEffectId = PersistentData::Get<std::string>(LastEffectIdKey);

        if (potionData.primaryEffectId.empty()){
            potionData.primaryEffectId = Crafting::EffectId::Explosion;
        }

        potionData.secondaryEffectId = PersistentData::Get<std::string>(LastSecondaryEffectIdKey);

        if (potionData.secondaryEffectId.empty()){
            potionData.secondaryEffectId = Crafting::EffectId::None;
        }

        potionData.qualityPercent = PersistentData::Get<float>(LastQualityKey);
        potionData.radius = PersistentData::Get<float>(LastRadiusKey);
        potionData.duration = PersistentData::Get<float>(LastDurationKey);
        potionData.power = PersistentData::Get<float>(LastPowerKey);
        potionData.mainEffectCount = PersistentData::Get<int>(LastMainEffectCountKey);
        potionData.modifierCount = PersistentData::Get<int>(LastModifierCountKey);

        if (potionData.radius <= 0.0f){
            potionData.radius = 3.0f;
        }

        if (potionData.duration <= 0.0f){
            potionData.duration = 4.0f;
        }

        if (potionData.power <= 0.0f){
            potionData.power = 25.0f;
        }

        if (potionData.mainEffectCount <= 0){
            potionData.mainEffectCount = 1;
        }

        return potionData;
    }

    inline std::string GetLastRecipeName(){
        return GetLastCraftedPotion().recipeName;
    }

    inline std::string GetLastEffectId(){
        return GetLastCraftedPotion().primaryEffectId;
    }

    inline float GetLastQuality(){
        return GetLastCraftedPotion().qualityPercent;
    }

    inline float GetLastRadius(){
        return GetLastCraftedPotion().radius;
    }

    inline float GetLastDuration(){
        return GetLastCraftedPotion().duration;
    }

    inline float GetLastPower(){
        return GetLastCraftedPotion().power;
    }

    inline void AddIngredient(const std::string& key, int count = 1){
        if (count <= 0){
            return;
        }

        PersistentData::Set<int>(
            key,
            PersistentData::Get<int>(key) + count
        );
    }

    inline void SetIngredientCount(const std::string& key, int count){
        if (count < 0){
            count = 0;
        }

        PersistentData::Set<int>(key,count);
    }

    inline int GetIngredientCount(const std::string& key){
        return PersistentData::Get<int>(key);
    }

    inline bool HasIngredient(const std::string& key){
        return GetIngredientCount(key) > 0;
    }

    inline Crafting::IngredientData CreateMainEffectIngredient(
        Crafting::IngredientType ingredientType,
        const std::string& displayName,
        const std::string& effectId,
        const glm::vec4& color
    ){
        Crafting::IngredientData data;

        data.type = ingredientType;
        data.displayName = displayName;
        data.role = Crafting::IngredientRole::MainEffect;
        data.effectId = effectId;
        data.modifierId = Crafting::ModifierId::None;
        data.value = 1.0f;
        data.color = color;

        return data;
    }

    inline Crafting::IngredientData CreateModifierIngredient(
        Crafting::IngredientType ingredientType,
        const std::string& displayName,
        const std::string& modifierId,
        float value,
        const glm::vec4& color
    ){
        Crafting::IngredientData data;

        data.type = ingredientType;
        data.displayName = displayName;
        data.role = Crafting::IngredientRole::Modifier;
        data.effectId = Crafting::EffectId::None;
        data.modifierId = modifierId;
        data.value = value;
        data.color = color;

        return data;
    }

    inline std::vector<IngredientInventoryEntry> GetAllIngredientDefinitions(){
        const glm::vec4 burnColor =
            glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);

        const glm::vec4 lightningColor =
            glm::vec4(1.0f, 1.0f, 0.1f, 1.0f);

        const glm::vec4 rangeColor =
            glm::vec4(0.1f, 0.8f, 0.2f, 1.0f);

        const glm::vec4 durationColor =
            glm::vec4(0.1f, 0.3f, 1.0f, 1.0f);

        const glm::vec4 powerColor =
            glm::vec4(0.9f, 0.25f, 0.15f, 1.0f);

        return {
            {
                IngredientSugarBurnKey,
                "Sugar Range",
                "./res/models/ingredients/sugar.glb",
                CreateModifierIngredient(
                    Crafting::IngredientType::Sugar,
                    "Sugar Range",
                    Crafting::ModifierId::Radius,
                    1.5f,
                    rangeColor
                )
            },
            {
                IngredientDriedBeetBurnKey,
                "Dried Beet Burn",
                "./res/models/ingredients/dried_beet.glb",
                CreateMainEffectIngredient(
                    Crafting::IngredientType::Sugar,
                    "Dried Beet Burn",
                    Crafting::EffectId::Burn,
                    burnColor
                )
            },
            {
                IngredientHoneyRangeKey,
                "Honey Range",
                "./res/models/ingredients/honey.glb",
                CreateModifierIngredient(
                    Crafting::IngredientType::Water,
                    "Honey Range",
                    Crafting::ModifierId::Radius,
                    1.5f,
                    rangeColor
                )
            },
            {
                IngredientRatTailRangeKey,
                "Rat Tail Burn",
                "./res/models/ingredients/rat_tail.glb",
                CreateMainEffectIngredient(
                    Crafting::IngredientType::Water,
                    "Rat Tail Burn",
                    Crafting::EffectId::Burn,
                    burnColor
                )
            },
            {
                IngredientWaterLightningKey,
                "Water Lightning",
                "./res/models/ingredients/water.glb",
                CreateMainEffectIngredient(
                    Crafting::IngredientType::Water,
                    "Water Lightning",
                    Crafting::EffectId::Lightning,
                    lightningColor
                )
            },
            {
                IngredientDriedPotatoLightningKey,
                "Dried Potato Lightning",
                "./res/models/ingredients/dried_potato.glb",
                CreateMainEffectIngredient(
                    Crafting::IngredientType::Water,
                    "Dried Potato Lightning",
                    Crafting::EffectId::Lightning,
                    lightningColor
                )
            },
            {
                IngredientBoneDurationKey,
                "Bone Duration",
                "./res/models/ingredients/bone.glb",
                CreateModifierIngredient(
                    Crafting::IngredientType::Sugar,
                    "Bone Duration",
                    Crafting::ModifierId::Duration,
                    2.5f,
                    durationColor
                )
            },
            {
                IngredientDeserterEarPowerKey,
                "Deserter Ear Power",
                "./res/models/ingredients/deserter_ear.glb",
                CreateModifierIngredient(
                    Crafting::IngredientType::Sugar,
                    "Deserter Ear Power",
                    Crafting::ModifierId::Power,
                    1.25f,
                    powerColor
                )
            }
        };
    }

    inline std::vector<IngredientInventoryEntry> GetOwnedIngredientDefinitions(){
        std::vector<IngredientInventoryEntry> ownedIngredients;

        std::vector<IngredientInventoryEntry> allIngredients =
            GetAllIngredientDefinitions();

        for (const IngredientInventoryEntry& ingredient : allIngredients){
            if (GetIngredientCount(ingredient.inventoryKey) > 0){
                ownedIngredients.push_back(ingredient);
            }
        }

        return ownedIngredients;
    }

    inline void EnsureStartingIngredients(){
        if (PersistentData::Get<bool>(IngredientInventoryInitializedKey)){
            return;
        }

        PersistentData::Set<bool>(
            IngredientInventoryInitializedKey,
            true
        );

        AddIngredient(IngredientRatTailRangeKey,1);
        AddIngredient(IngredientSugarBurnKey,1);
    }

    inline void GiveRatLoot(){
        EnsureStartingIngredients();
        AddIngredient(IngredientRatTailRangeKey,1);
        AddIngredient(IngredientSugarBurnKey,1);
    }
}