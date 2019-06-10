#include "common.hpp"
#include "soundcache.hpp"
std::map<int, SoundStruct> sound_cache;

namespace soundcache
{

void CreateMove()
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
}

class SoundCacheEventListener : public IGameEventListener2
{
    virtual void FireGameEvent(IGameEvent *event)
    {
        if (!isHackActive())
            return;
        sound_cache[event->GetInt("userid")].sound.m_pOrigin = Vector(0.0f);
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
