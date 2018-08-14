#include "common.hpp"
namespace nav {
    CNavFile navfile(0);
    CNavArea LocalNav;
    std::vector<nav::singleNode *> areas;
    std::vector<CNavArea> SniperAreas;
    void Init(const char *lvlname)
    {
        for (auto &it : areas)
            it = {};
        navfile = CNavFile(lvlname);
        if (!navfile.m_isOK)
            logging::Info("Invalid Nav File");
        else
        {
            for (int i = 0; i < navfile.m_areas.size() - 1 + 100; i++)
                areas.push_back({});
            for (auto area : navfile.m_areas)
            {
                nav::singleNode node;
                node.pos = area.m_center;
                node.id = area.m_id;
                areas[node.id] = &node;
                if (area.m_hidingSpots.size())
                    SniperAreas.push_back(area);

            }
            for (auto area : navfile.m_areas)
            {
                for (auto node : areas)
                {
                    if (area.m_id != node->id)
                        continue;
                    for (auto connect : area.m_connections)
                        node->addChildren(areas[connect.area->m_id]);
                }
            }
        }
    }
    Timer cd{};
    void CreateMove()
    {
        if (navfile.m_isOK)
        {
            if (cd.test_and_set(300)) {
                for (auto i : navfile.m_areas) {
		    Vector vec = LOCAL_E->m_vecOrigin();
                    if (i.Contains(vec)) {
                        LocalNav = i;
                        break;
                    }
                }
            }
            if (SniperAreas.size()) {
                auto res = areas[LocalNav.m_id]->FindPath(areas[SniperAreas[0].m_id]);
                for (auto r : res)
                    logging::Info("%f, %f, %f", r->pos.x, r->pos.y, r->pos.z);
            }
        }
    }
}
