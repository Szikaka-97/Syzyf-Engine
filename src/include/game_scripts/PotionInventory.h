#pragma once

#include <PersistentData.h>
#include <game_scripts/crafting/CraftingTypes.h>

#include <string>

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

    inline const std::string IngredientBurnKey = "IngredientInventory_Burn";
    inline const std::string IngredientLightningKey = "IngredientInventory_Lightning";
    inline const std::string IngredientRadiusKey = "IngredientInventory_Radius";
    inline const std::string IngredientDurationKey = "IngredientInventory_Duration";

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
            potionData.primaryEffectId = Crafting::EffectId::Burn;
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

    inline int GetIngredientCount(const std::string& key){
        return PersistentData::Get<int>(key);
    }

    inline void GiveRatLoot(){
        AddIngredient(IngredientBurnKey,1);
        AddIngredient(IngredientRadiusKey,1);
    }
}
