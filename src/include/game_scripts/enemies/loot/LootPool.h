#pragma once
#include <vector>
#include <random>
#include "LootItem.h"

struct LootEntry {
    const LootItem* item;
    int count;        
    int originalCount;  
};

class LootPool {
public:
    void AddItem(const LootItem* item, int count) {
        m_Entries.push_back({item, count, count});
        m_TotalCount += count;
    }

    const LootItem* Draw() {
        if (m_TotalCount == 0) {
            Reset(); 
            if (m_TotalCount == 0) return nullptr;
        }

        int rnd = m_Rng() % m_TotalCount;
        int idx = 0;
        while (rnd >= m_Entries[idx].count) {
            rnd -= m_Entries[idx].count;
            ++idx;
        }
        const LootItem* result = m_Entries[idx].item;
        --m_Entries[idx].count;
        --m_TotalCount;
        return result;
    }

    void Reset() {
        for (auto& entry : m_Entries) {
            entry.count = entry.originalCount;
        }
        m_TotalCount = 0;
        for (auto& entry : m_Entries) m_TotalCount += entry.count;
    }

      static LootPool& GetSkeletonLootPool() {
    static LootPool pool = []() {
        LootPool p;
        p.AddItem(new LootBone(),20);
        return p;
    }();
    return pool;
}
      static LootPool& GetPotatoLootPool() {
    static LootPool pool = []() {
        LootPool p;
        p.AddItem(new LootPotato(),20);
        return p;
    }();
    return pool;
}
      static LootPool& GetBeetrootLootPool() {
    static LootPool pool = []() {
        LootPool p;
        p.AddItem(new LootBeetroot(),20);
        return p;
    }();
    return pool;
}
      
static LootPool& GetMeleeSkeletonLootPool() {
    static LootPool pool = []() {
        LootPool p;
        p.AddItem(new LootDeserterEar(),20);
        return p;
    }();
    return pool;
}

private:
    std::vector<LootEntry> m_Entries;
    int m_TotalCount = 0;
    std::mt19937 m_Rng{std::random_device{}()};
};