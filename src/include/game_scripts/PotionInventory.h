#pragma once

#include <PersistentData.h>
#include <game_scripts/crafting/CraftingTypes.h>

#include <cmath>
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
    inline const std::string LastOptionalIngredientsTextKey = "PotionInventory_LastOptionalIngredientsText";
    inline const std::string FirstPotionCreatedKey = "Crafting_FirstPotionCreated";
    inline const std::string ShowTutorialFinishedMessageKey = "Crafting_ShowTutorialFinishedMessage";
    inline const std::string SelectedPotionSlotIndexKey = "PotionInventory_SelectedPotionSlotIndex";

    inline const std::string IngredientInventoryInitializedKey = "IngredientInventory_Initialized";

    inline const std::string IngredientSugarExplosionKey = "IngredientInventory_SugarExplosion";
    inline const std::string IngredientDriedBeetBurnKey = "IngredientInventory_DriedBeetBurn";
    inline const std::string IngredientBonePetrifyKey = "IngredientInventory_BonePetrify";
    inline const std::string IngredientWaterTornadoKey = "IngredientInventory_WaterTornado";
    inline const std::string IngredientRatTailConfuseKey = "IngredientInventory_RatTailConfuse";
    inline const std::string IngredientHoneyRadiusKey = "IngredientInventory_HoneyRadius";
    inline const std::string IngredientDriedPotatoDurationKey = "IngredientInventory_DriedPotatoDuration";
    inline const std::string IngredientDeserterEarPowerKey = "IngredientInventory_DeserterEarPower";

    inline const std::string IngredientSugarBurnKey = IngredientSugarExplosionKey;
    inline const std::string IngredientHoneyRangeKey = IngredientHoneyRadiusKey;
    inline const std::string IngredientRatTailRangeKey = IngredientRatTailConfuseKey;
    inline const std::string IngredientWaterLightningKey = IngredientWaterTornadoKey;
    inline const std::string IngredientDriedPotatoLightningKey = IngredientDriedPotatoDurationKey;
    inline const std::string IngredientBoneDurationKey = IngredientBonePetrifyKey;

    inline const std::string IngredientExplosionKey = IngredientSugarExplosionKey;
    inline const std::string IngredientFireKey = IngredientDriedBeetBurnKey;
    inline const std::string IngredientBurnKey = IngredientDriedBeetBurnKey;
    inline const std::string IngredientPetrifyKey = IngredientBonePetrifyKey;
    inline const std::string IngredientTornadoKey = IngredientWaterTornadoKey;
    inline const std::string IngredientConfuseKey = IngredientRatTailConfuseKey;
    inline const std::string IngredientRadiusKey = IngredientHoneyRadiusKey;
    inline const std::string IngredientDurationKey = IngredientDriedPotatoDurationKey;

    inline constexpr int MaxPotionInventorySlots = 128;

    struct IngredientInventoryEntry{
        std::string inventoryKey;
        std::string displayName;
        std::string modelPath;
        Crafting::IngredientData data;
    };

    struct PotionInventoryEntry{
        int slotIndex = -1;
        int count = 0;
        Crafting::CraftedPotionData data;
    };

    inline std::string PotionSlotKey(int slotIndex, const std::string& field){
        return "PotionInventory_Slot_" + std::to_string(slotIndex) + "_" + field;
    }

    inline Crafting::CraftedPotionData NormalizePotionData(
        Crafting::CraftedPotionData potionData
    ){
        if (potionData.recipeName.empty()){
            potionData.recipeName = "Potion";
        }

        if (potionData.primaryEffectId.empty()){
            potionData.primaryEffectId = Crafting::EffectId::Explosion;
        }

        if (potionData.secondaryEffectId.empty()){
            potionData.secondaryEffectId = Crafting::EffectId::None;
        }

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

    inline Crafting::CraftedPotionData ReadLastCraftedPotionData(){
        Crafting::CraftedPotionData potionData;

        potionData.recipeName = PersistentData::Get<std::string>(LastRecipeNameKey);
        potionData.primaryEffectId = PersistentData::Get<std::string>(LastEffectIdKey);
        potionData.secondaryEffectId = PersistentData::Get<std::string>(LastSecondaryEffectIdKey);
        potionData.qualityPercent = PersistentData::Get<float>(LastQualityKey);
        potionData.radius = PersistentData::Get<float>(LastRadiusKey);
        potionData.duration = PersistentData::Get<float>(LastDurationKey);
        potionData.power = PersistentData::Get<float>(LastPowerKey);
        potionData.mainEffectCount = PersistentData::Get<int>(LastMainEffectCountKey);
        potionData.modifierCount = PersistentData::Get<int>(LastModifierCountKey);
        potionData.optionalIngredientsText = PersistentData::Get<std::string>(LastOptionalIngredientsTextKey);

        return NormalizePotionData(potionData);
    }

    inline bool SamePotionData(
        const Crafting::CraftedPotionData& first,
        const Crafting::CraftedPotionData& second
    ){
        constexpr float epsilon = 0.001f;

        return
            first.recipeName == second.recipeName &&
            first.primaryEffectId == second.primaryEffectId &&
            first.secondaryEffectId == second.secondaryEffectId &&
            first.mainEffectCount == second.mainEffectCount &&
            first.modifierCount == second.modifierCount &&
            std::abs(first.qualityPercent - second.qualityPercent) <= epsilon &&
            std::abs(first.radius - second.radius) <= epsilon &&
            std::abs(first.duration - second.duration) <= epsilon &&
            std::abs(first.power - second.power) <= epsilon &&
            first.optionalIngredientsText == second.optionalIngredientsText;
    }

    inline int GetPotionSlotCount(int slotIndex){
        return PersistentData::Get<int>(PotionSlotKey(slotIndex,"Count"));
    }

    inline bool IsPotionSlotUsed(int slotIndex){
        return GetPotionSlotCount(slotIndex) > 0;
    }

    inline Crafting::CraftedPotionData GetPotionSlotData(int slotIndex){
        Crafting::CraftedPotionData potionData;

        potionData.recipeName = PersistentData::Get<std::string>(PotionSlotKey(slotIndex,"RecipeName"));
        potionData.primaryEffectId = PersistentData::Get<std::string>(PotionSlotKey(slotIndex,"EffectId"));
        potionData.secondaryEffectId = PersistentData::Get<std::string>(PotionSlotKey(slotIndex,"SecondaryEffectId"));
        potionData.qualityPercent = PersistentData::Get<float>(PotionSlotKey(slotIndex,"Quality"));
        potionData.radius = PersistentData::Get<float>(PotionSlotKey(slotIndex,"Radius"));
        potionData.duration = PersistentData::Get<float>(PotionSlotKey(slotIndex,"Duration"));
        potionData.power = PersistentData::Get<float>(PotionSlotKey(slotIndex,"Power"));
        potionData.mainEffectCount = PersistentData::Get<int>(PotionSlotKey(slotIndex,"MainEffectCount"));
        potionData.modifierCount = PersistentData::Get<int>(PotionSlotKey(slotIndex,"ModifierCount"));
        potionData.optionalIngredientsText = PersistentData::Get<std::string>(PotionSlotKey(slotIndex,"OptionalIngredientsText"));

        return NormalizePotionData(potionData);
    }

    inline void SetPotionSlotData(
        int slotIndex,
        const Crafting::CraftedPotionData& potionData,
        int count
    ){
        Crafting::CraftedPotionData normalizedPotionData =
            NormalizePotionData(potionData);

        if (count < 0){
            count = 0;
        }

        PersistentData::Set<int>(PotionSlotKey(slotIndex,"Count"),count);
        PersistentData::Set<std::string>(PotionSlotKey(slotIndex,"RecipeName"),normalizedPotionData.recipeName);
        PersistentData::Set<std::string>(PotionSlotKey(slotIndex,"EffectId"),normalizedPotionData.primaryEffectId);
        PersistentData::Set<std::string>(PotionSlotKey(slotIndex,"SecondaryEffectId"),normalizedPotionData.secondaryEffectId);
        PersistentData::Set<float>(PotionSlotKey(slotIndex,"Quality"),normalizedPotionData.qualityPercent);
        PersistentData::Set<float>(PotionSlotKey(slotIndex,"Radius"),normalizedPotionData.radius);
        PersistentData::Set<float>(PotionSlotKey(slotIndex,"Duration"),normalizedPotionData.duration);
        PersistentData::Set<float>(PotionSlotKey(slotIndex,"Power"),normalizedPotionData.power);
        PersistentData::Set<int>(PotionSlotKey(slotIndex,"MainEffectCount"),normalizedPotionData.mainEffectCount);
        PersistentData::Set<int>(PotionSlotKey(slotIndex,"ModifierCount"),normalizedPotionData.modifierCount);
        PersistentData::Set<std::string>(PotionSlotKey(slotIndex,"OptionalIngredientsText"),normalizedPotionData.optionalIngredientsText);
    }

    inline void ClearPotionSlot(int slotIndex){
        PersistentData::Set<int>(PotionSlotKey(slotIndex,"Count"),0);
        PersistentData::Clear<std::string>(PotionSlotKey(slotIndex,"RecipeName"));
        PersistentData::Clear<std::string>(PotionSlotKey(slotIndex,"EffectId"));
        PersistentData::Clear<std::string>(PotionSlotKey(slotIndex,"SecondaryEffectId"));
        PersistentData::Clear<float>(PotionSlotKey(slotIndex,"Quality"));
        PersistentData::Clear<float>(PotionSlotKey(slotIndex,"Radius"));
        PersistentData::Clear<float>(PotionSlotKey(slotIndex,"Duration"));
        PersistentData::Clear<float>(PotionSlotKey(slotIndex,"Power"));
        PersistentData::Clear<int>(PotionSlotKey(slotIndex,"MainEffectCount"));
        PersistentData::Clear<int>(PotionSlotKey(slotIndex,"ModifierCount"));
        PersistentData::Clear<std::string>(PotionSlotKey(slotIndex,"OptionalIngredientsText"));
    }

    inline bool IsTutorialPotionData(const Crafting::CraftedPotionData& potionData){
        Crafting::CraftedPotionData normalizedPotionData =
            NormalizePotionData(potionData);

        return
            normalizedPotionData.recipeName == "Basic Potion" &&
            normalizedPotionData.primaryEffectId == Crafting::EffectId::Explosion;
    }

    inline void ClearLastCraftedPotion(){
        PersistentData::Clear<std::string>(LastRecipeNameKey);
        PersistentData::Clear<std::string>(LastEffectIdKey);
        PersistentData::Clear<std::string>(LastSecondaryEffectIdKey);
        PersistentData::Clear<float>(LastQualityKey);
        PersistentData::Clear<float>(LastRadiusKey);
        PersistentData::Clear<float>(LastDurationKey);
        PersistentData::Clear<float>(LastPowerKey);
        PersistentData::Clear<int>(LastMainEffectCountKey);
        PersistentData::Clear<int>(LastModifierCountKey);
        PersistentData::Clear<std::string>(LastOptionalIngredientsTextKey);
    }

    inline int CountPotionStacks(){
        int total = 0;

        for (int i = 0; i < MaxPotionInventorySlots; i++){
            total += GetPotionSlotCount(i);
        }

        return total;
    }

    inline void ClearTutorialPotions(){
        for (int i = 0; i < MaxPotionInventorySlots; i++){
            if (!IsPotionSlotUsed(i)){
                continue;
            }

            if (IsTutorialPotionData(GetPotionSlotData(i))){
                ClearPotionSlot(i);
            }
        }

        if (IsTutorialPotionData(ReadLastCraftedPotionData())){
            ClearLastCraftedPotion();
        }

        PersistentData::Set<int>(PotionCountKey,CountPotionStacks());
    }

    inline void SynchronizePotionCountFromStacks(){
        int stackedPotionCount = CountPotionStacks();

        if (stackedPotionCount > 0 || PersistentData::Get<int>(PotionCountKey) <= 0){
            PersistentData::Set<int>(PotionCountKey,stackedPotionCount);
        }
    }

    inline int GetPotionCount(){
        int stackedPotionCount = CountPotionStacks();

        if (stackedPotionCount > 0){
            PersistentData::Set<int>(PotionCountKey,stackedPotionCount);
            return stackedPotionCount;
        }

        return PersistentData::Get<int>(PotionCountKey);
    }

    inline void SetPotionCount(int count){
        if (count < 0){
            count = 0;
        }

        PersistentData::Set<int>(PotionCountKey,count);
    }

    inline std::vector<PotionInventoryEntry> GetPotionInventory(){
        std::vector<PotionInventoryEntry> result;

        for (int i = 0; i < MaxPotionInventorySlots; i++){
            int count = GetPotionSlotCount(i);

            if (count <= 0){
                continue;
            }

            PotionInventoryEntry entry;
            entry.slotIndex = i;
            entry.count = count;
            entry.data = GetPotionSlotData(i);
            result.push_back(entry);
        }

        return result;
    }

    inline int FindMatchingPotionSlot(const Crafting::CraftedPotionData& potionData){
        Crafting::CraftedPotionData normalizedPotionData = NormalizePotionData(potionData);

        for (int i = 0; i < MaxPotionInventorySlots; i++){
            if (!IsPotionSlotUsed(i)){
                continue;
            }

            if (SamePotionData(GetPotionSlotData(i),normalizedPotionData)){
                return i;
            }
        }

        return -1;
    }

    inline int FindFreePotionSlot(){
        for (int i = 0; i < MaxPotionInventorySlots; i++){
            if (!IsPotionSlotUsed(i)){
                return i;
            }
        }

        return -1;
    }

    inline void AddPotionStack(
        const Crafting::CraftedPotionData& potionData,
        int count
    ){
        if (count <= 0){
            return;
        }

        Crafting::CraftedPotionData normalizedPotionData = NormalizePotionData(potionData);
        int slotIndex = FindMatchingPotionSlot(normalizedPotionData);

        if (slotIndex < 0){
            slotIndex = FindFreePotionSlot();
        }

        if (slotIndex < 0){
            return;
        }

        SetPotionSlotData(
            slotIndex,
            normalizedPotionData,
            GetPotionSlotCount(slotIndex) + count
        );

        PersistentData::Set<int>(PotionCountKey,CountPotionStacks());
    }

    inline void AddPotions(int count){
        AddPotionStack(ReadLastCraftedPotionData(),count);
    }

    inline bool HasPotion(){
        return GetPotionCount() > 0;
    }

    inline std::string PotionDisplayName(
        const Crafting::CraftedPotionData& potionData
    ){
        Crafting::CraftedPotionData normalizedPotionData =
            NormalizePotionData(potionData);

        if (!normalizedPotionData.recipeName.empty() &&
            normalizedPotionData.recipeName != "Potion"){
            return normalizedPotionData.recipeName;
        }

        std::string label = normalizedPotionData.primaryEffectId;

        if (!normalizedPotionData.secondaryEffectId.empty() &&
            normalizedPotionData.secondaryEffectId != Crafting::EffectId::None){
            label += " + " + normalizedPotionData.secondaryEffectId;
        }

        if (label.empty() || label == Crafting::EffectId::None){
            return "Potion";
        }

        return label + " Potion";
    }

    inline std::string PotionBottleModelPath(
        const Crafting::CraftedPotionData& potionData
    ){
        Crafting::CraftedPotionData normalizedPotionData =
            NormalizePotionData(potionData);

        const std::string& effectId = normalizedPotionData.primaryEffectId;

        if (effectId == Crafting::EffectId::Fire ||
            effectId == Crafting::EffectId::Burn){
            return "./res/models/bottles/fire_bottle.glb";
        }

        if (effectId == Crafting::EffectId::Petrify){
            return "./res/models/bottles/petrify_bottle.glb";
        }

        if (effectId == Crafting::EffectId::Tornado){
            return "./res/models/bottles/tornado_bottle.glb";
        }

        if (effectId == Crafting::EffectId::Confuse){
            return "./res/models/bottles/confuse_bottle.glb";
        }

        return "./res/models/bottles/explode_bottle.glb";
    }

    inline std::string PotionIconPath(
        const Crafting::CraftedPotionData& potionData
    ){
        Crafting::CraftedPotionData normalizedPotionData =
            NormalizePotionData(potionData);

        const std::string& effectId = normalizedPotionData.primaryEffectId;

        if (effectId == Crafting::EffectId::Fire ||
            effectId == Crafting::EffectId::Burn){
            return "./res/textures/ui/2d/item_icons/fire_bottle.png";
        }

        if (effectId == Crafting::EffectId::Petrify){
            return "./res/textures/ui/2d/item_icons/petrify_bottle.png";
        }

        if (effectId == Crafting::EffectId::Tornado){
            return "./res/textures/ui/2d/item_icons/tornado_bottle.png";
        }

        if (effectId == Crafting::EffectId::Confuse){
            return "./res/textures/ui/2d/item_icons/confuse_bottle.png";
        }

        return "./res/textures/ui/2d/item_icons/explode_bottle.png";
    }

    inline void SetSelectedPotionSlotIndex(int slotIndex){
        PersistentData::Set<int>(SelectedPotionSlotIndexKey,slotIndex);
    }

    inline int GetSelectedPotionSlotIndex(){
        int selectedSlotIndex = PersistentData::Get<int>(SelectedPotionSlotIndexKey);

        if (selectedSlotIndex >= 0 && selectedSlotIndex < MaxPotionInventorySlots &&
            IsPotionSlotUsed(selectedSlotIndex)){
            return selectedSlotIndex;
        }

        for (int i = 0; i < MaxPotionInventorySlots; i++){
            if (IsPotionSlotUsed(i)){
                SetSelectedPotionSlotIndex(i);
                return i;
            }
        }

        SetSelectedPotionSlotIndex(-1);
        return -1;
    }

    inline bool GetSelectedPotion(PotionInventoryEntry* selectedPotion){
        int selectedSlotIndex = GetSelectedPotionSlotIndex();

        if (selectedSlotIndex < 0){
            return false;
        }

        if (selectedPotion){
            selectedPotion->slotIndex = selectedSlotIndex;
            selectedPotion->count = GetPotionSlotCount(selectedSlotIndex);
            selectedPotion->data = GetPotionSlotData(selectedSlotIndex);
        }

        return true;
    }

    inline bool ConsumePotionSlot(
        int slotIndex,
        Crafting::CraftedPotionData* consumedPotionData
    ){
        if (slotIndex < 0 || slotIndex >= MaxPotionInventorySlots){
            return false;
        }

        int count = GetPotionSlotCount(slotIndex);

        if (count <= 0){
            return false;
        }

        Crafting::CraftedPotionData potionData = GetPotionSlotData(slotIndex);

        if (consumedPotionData){
            *consumedPotionData = potionData;
        }

        if (count == 1){
            ClearPotionSlot(slotIndex);
        }
        else{
            SetPotionSlotData(slotIndex,potionData,count - 1);
        }

        PersistentData::Set<int>(PotionCountKey,CountPotionStacks());
        GetSelectedPotionSlotIndex();
        return true;
    }

    inline bool ConsumeSelectedPotion(Crafting::CraftedPotionData* consumedPotionData){
        int selectedSlotIndex = GetSelectedPotionSlotIndex();

        if (ConsumePotionSlot(selectedSlotIndex,consumedPotionData)){
            return true;
        }

        for (int i = 0; i < MaxPotionInventorySlots; i++){
            if (ConsumePotionSlot(i,consumedPotionData)){
                return true;
            }
        }

        int legacyPotionCount = PersistentData::Get<int>(PotionCountKey);

        if (legacyPotionCount <= 0){
            return false;
        }

        if (consumedPotionData){
            *consumedPotionData = ReadLastCraftedPotionData();
        }

        SetPotionCount(legacyPotionCount - 1);
        return true;
    }

    inline bool ConsumePotion(Crafting::CraftedPotionData* consumedPotionData){
        return ConsumeSelectedPotion(consumedPotionData);
    }

    inline bool ConsumePotion(){
        return ConsumePotion(nullptr);
    }

    inline void SaveLastCraftedPotion(
        const Crafting::CraftedPotionData& potionData,
        int bottleCount,
        bool countAsCraftedPotion = true
    ){
        Crafting::CraftedPotionData normalizedPotionData = NormalizePotionData(potionData);

        AddPotionStack(normalizedPotionData,bottleCount);

        PersistentData::Set<std::string>(LastRecipeNameKey,normalizedPotionData.recipeName);
        PersistentData::Set<std::string>(LastEffectIdKey,normalizedPotionData.primaryEffectId);
        PersistentData::Set<std::string>(LastSecondaryEffectIdKey,normalizedPotionData.secondaryEffectId);
        PersistentData::Set<float>(LastQualityKey,normalizedPotionData.qualityPercent);
        PersistentData::Set<float>(LastRadiusKey,normalizedPotionData.radius);
        PersistentData::Set<float>(LastDurationKey,normalizedPotionData.duration);
        PersistentData::Set<float>(LastPowerKey,normalizedPotionData.power);
        PersistentData::Set<int>(LastMainEffectCountKey,normalizedPotionData.mainEffectCount);
        PersistentData::Set<int>(LastModifierCountKey,normalizedPotionData.modifierCount);
        PersistentData::Set<std::string>(LastOptionalIngredientsTextKey,normalizedPotionData.optionalIngredientsText);

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
        return ReadLastCraftedPotionData();
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
        if (count <= 0 || key.empty()){
            return;
        }

        PersistentData::Set<int>(
            key,
            PersistentData::Get<int>(key) + count
        );
    }

    inline void SetIngredientCount(const std::string& key, int count){
        if (key.empty()){
            return;
        }

        if (count < 0){
            count = 0;
        }

        PersistentData::Set<int>(key,count);
    }

    inline int GetIngredientCount(const std::string& key){
        if (key.empty()){
            return 0;
        }

        return PersistentData::Get<int>(key);
    }

    inline bool HasIngredient(const std::string& key){
        return GetIngredientCount(key) > 0;
    }

    inline bool ConsumeIngredient(const std::string& key, int count = 1){
        if (count <= 0){
            return true;
        }

        int currentCount = GetIngredientCount(key);

        if (currentCount < count){
            return false;
        }

        SetIngredientCount(key,currentCount - count);
        return true;
    }

    inline void RefundIngredient(const std::string& key, int count = 1){
        AddIngredient(key,count);
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
        const glm::vec4 explosionColor =
            glm::vec4(0.95f, 0.85f, 0.35f, 1.0f);

        const glm::vec4 burnColor =
            glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);

        const glm::vec4 petrifyColor =
            glm::vec4(0.45f, 0.55f, 0.65f, 1.0f);

        const glm::vec4 tornadoColor =
            glm::vec4(0.35f, 0.75f, 1.0f, 1.0f);

        const glm::vec4 confuseColor =
            glm::vec4(0.75f, 0.25f, 0.95f, 1.0f);

        const glm::vec4 rangeColor =
            glm::vec4(0.1f, 0.8f, 0.2f, 1.0f);

        const glm::vec4 durationColor =
            glm::vec4(0.1f, 0.3f, 1.0f, 1.0f);

        const glm::vec4 powerColor =
            glm::vec4(0.9f, 0.25f, 0.15f, 1.0f);

        std::vector<IngredientInventoryEntry> definitions = {
            {
                IngredientSugarExplosionKey,
                "Sugar Explosion",
                "./res/models/ingredients/sugar.glb",
                CreateMainEffectIngredient(
                    Crafting::IngredientType::Sugar,
                    "Sugar Explosion",
                    Crafting::EffectId::Explosion,
                    explosionColor
                )
            },
            {
                IngredientDriedBeetBurnKey,
                "Dried Beet Fire",
                "./res/models/ingredients/dried_beet.glb",
                CreateMainEffectIngredient(
                    Crafting::IngredientType::Sugar,
                    "Dried Beet Fire",
                    Crafting::EffectId::Fire,
                    burnColor
                )
            },
            {
                IngredientBonePetrifyKey,
                "Bone Petrify",
                "./res/models/ingredients/bone.glb",
                CreateMainEffectIngredient(
                    Crafting::IngredientType::Sugar,
                    "Bone Petrify",
                    Crafting::EffectId::Petrify,
                    petrifyColor
                )
            },
            {
                IngredientWaterTornadoKey,
                "Water Tornado",
                "./res/models/ingredients/water.glb",
                CreateMainEffectIngredient(
                    Crafting::IngredientType::Water,
                    "Water Tornado",
                    Crafting::EffectId::Tornado,
                    tornadoColor
                )
            },
            {
                IngredientRatTailConfuseKey,
                "Rat Tail Confuse",
                "./res/models/ingredients/rat_tail.glb",
                CreateMainEffectIngredient(
                    Crafting::IngredientType::Water,
                    "Rat Tail Confuse",
                    Crafting::EffectId::Confuse,
                    confuseColor
                )
            },
            {
                IngredientHoneyRadiusKey,
                "Honey Radius",
                "./res/models/ingredients/honey.glb",
                CreateModifierIngredient(
                    Crafting::IngredientType::Water,
                    "Honey Radius",
                    Crafting::ModifierId::Radius,
                    1.5f,
                    rangeColor
                )
            },
            {
                IngredientDriedPotatoDurationKey,
                "Dried Potato Duration",
                "./res/models/ingredients/dried_potato.glb",
                CreateModifierIngredient(
                    Crafting::IngredientType::Water,
                    "Dried Potato Duration",
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

        for (IngredientInventoryEntry& definition : definitions){
            definition.data.inventoryKey = definition.inventoryKey;
        }

        return definitions;
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

    inline std::vector<IngredientInventoryEntry> GetMainEffectIngredientDefinitions(){
        std::vector<IngredientInventoryEntry> mainEffectIngredients;

        std::vector<IngredientInventoryEntry> allIngredients =
            GetAllIngredientDefinitions();

        for (const IngredientInventoryEntry& ingredient : allIngredients){
            if (ingredient.data.role == Crafting::IngredientRole::MainEffect){
                mainEffectIngredients.push_back(ingredient);
            }
        }

        return mainEffectIngredients;
    }

    inline void AddOneOfEachMainEffectIngredient(){
        std::vector<IngredientInventoryEntry> mainEffectIngredients =
            GetMainEffectIngredientDefinitions();

        for (const IngredientInventoryEntry& ingredient : mainEffectIngredients){
            AddIngredient(ingredient.inventoryKey,1);
        }
    }

    inline void EnsureStartingIngredients(){
        if (PersistentData::Get<bool>(IngredientInventoryInitializedKey)){
            return;
        }

        PersistentData::Set<bool>(
            IngredientInventoryInitializedKey,
            true
        );

        AddIngredient(IngredientSugarExplosionKey,1);
        AddIngredient(IngredientDriedBeetBurnKey,1);
    }

    inline void GiveRatLoot(){
        AddOneOfEachMainEffectIngredient();
    }
}
