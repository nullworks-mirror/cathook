#include "common.hpp"
#include "soundcache.hpp"
std::map<int, SoundStruct> sound_cache;

namespace soundcache
{
constexpr unsigned int EXPIRETIME = 10000;

static void CreateMove()
{
    if (CE_BAD(LOCAL_E))
        return;
    CUtlVector<SndInfo_t> sound_list;
    g_ISoundEngine->GetActiveSounds(sound_list);
    for (auto i : sound_list)
    {
        sound_cache[i.m_nSoundSource].sound.m_pOrigin = *i.m_pOrigin;
        sound_cache[i.m_nSoundSource].last_update.update();
    }

    for (auto it = sound_cache.cbegin(); it != sound_cache.cend();)
    {
        if (it->second.last_update.check(EXPIRETIME))

            it = sound_cache.erase(it);
        else
            ++it;
    }
}

std::optional<Vector> GetSoundLocation(int entid)
{
    auto it = sound_cache.find(entid);
    if (it == sound_cache.end())
        return std::nullopt;
    return it->second.sound.m_pOrigin;
}

class SoundCacheEventListener : public IGameEventListener2
{
    virtual void FireGameEvent(IGameEvent *event)
    {
        if (!isHackActive())
            return;
        // Find userid in map
        auto it = sound_cache.find(event->GetInt("userid"));
        // No action if it doesn't exist
        if (it == sound_cache.end())
            return;
        // Erase it from the map
        sound_cache.erase(it);
    }
};

SoundCacheEventListener &listener()
{
    static SoundCacheEventListener object{};
    return object;
}

static InitRoutine init([]() {
    EC::Register(EC::CreateMove, CreateMove, "CM_SoundCache");
    EC::Register(
        EC::LevelInit, []() { sound_cache.clear(); }, "soundcache_levelinit");
    g_IEventManager2->AddListener(&listener(), "player_death", false);
    EC::Register(
        EC::Shutdown, []() { g_IEventManager2->RemoveListener(&listener()); }, "event_shutdown");
});
} // namespace soundcache
