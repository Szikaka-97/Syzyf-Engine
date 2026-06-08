#pragma once

#include <PersistentData.h>

#include <string>

namespace PotionInventory{
    inline const std::string PotionCountKey = "PotionInventory_Count";
    inline const std::string LastRecipeNameKey = "PotionInventory_LastRecipeName";
    inline const std::string LastEffectIdKey = "PotionInventory_LastEffectId";
    inline const std::string LastQualityKey = "PotionInventory_LastQuality";
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
        const std::string& recipeName,
        const std::string& effectId,
        float quality,
        int bottleCount
    ){
        AddPotions(bottleCount);

        PersistentData::Set<std::string>(LastRecipeNameKey,recipeName);
        PersistentData::Set<std::string>(LastEffectIdKey,effectId);
        PersistentData::Set<float>(LastQualityKey,quality);

        if (!PersistentData::Get<bool>(FirstPotionCreatedKey)){
            PersistentData::Set<bool>(FirstPotionCreatedKey,true);
            PersistentData::Set<bool>(ShowTutorialFinishedMessageKey,true);
        }
    }

    inline std::string GetLastRecipeName(){
        std::string recipeName = PersistentData::Get<std::string>(LastRecipeNameKey);

        if (recipeName.empty()){
            return "Potion";
        }

        return recipeName;
    }

    inline std::string GetLastEffectId(){
        return PersistentData::Get<std::string>(LastEffectIdKey);
    }

    inline float GetLastQuality(){
        return PersistentData::Get<float>(LastQualityKey);
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
