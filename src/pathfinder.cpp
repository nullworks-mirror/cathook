#include "common.hpp"
#include "external/TF2_NavFile_Reader/CNavFile.h"
#include "external/PathFinder/src/PathFinder.h"
#include "external/PathFinder/src/AStar.h"

struct navNode : public AStarNode
{
    navNode()
    {
    }

    ~navNode()
    {
    }

    Vector vecLoc;
};

std::unique_ptr<CNavFile> navData = nullptr;
std::vector<navNode *> nodes;
Timer pathtimer{};
settings::Bool enabled{"pathing.enabled", 0};

bool init()
{
    if(!enabled)
        return false;
    logging::Info("Pathing: Initiating path...");

    // This will not work, please fix
    std::string levelName = g_IEngine->GetLevelName();
    navData = std::make_unique<CNavFile>(CNavFile(levelName.c_str()));
    if (!navData->m_isOK)
    {
        navData = nullptr;
        logging::Info("Pathing: Failed to parse nav file!");

        return false;
    }
    std::vector<CNavArea> *areas = &navData->m_areas;
    int nodeCount                = areas->size();

    nodes.clear();
    nodes.reserve(nodeCount);

    // register nodes
    for (int i = 0; i < nodeCount; i++)
    {
        navNode *node{};
        //node->setPosition(areas->at(i).m_center.x, areas->at(i).m_center.y);
        node->vecLoc = areas->at(i).m_center;
        nodes.push_back(node);
    }

    for (int i = 0; i < nodeCount; ++i)
    {
        std::vector<NavConnect> *connections = &areas->at(i).m_connections;
        int childCount                       = connections->size();
        navNode *currNode                    = nodes.at(i);
        for (int j = 0; j < childCount; j++)
        {
            currNode->addChild(nodes.at(connections->at(j).id), 1.0f);
        }
    }
    logging::Info("Path init successful");
    return true;
}

int findClosestNavSquare(Vector vec)
{
    float bestDist = 999999.0f;
    int bestSquare = -1;
    for (int i = 0; i < nodes.size(); i++)
    {
        float dist = nodes.at(i)->vecLoc.DistTo(vec);
        if(dist < bestDist)
        {
            bestDist = dist;
            bestSquare = i;
        }
    }
    return bestSquare;
}

std::vector<Vector> findPath(Vector loc, Vector dest)
{
    if (nodes.empty())
        return {};

    int node_loc = findClosestNavSquare(loc);
    int node_dest = findClosestNavSquare(dest);

    PathFinder<navNode> Finder;
    Finder.setStart(*nodes.at(node_loc));
    Finder.setGoal(*nodes.at(node_dest));

    std::vector<navNode> pathNodes;
    //bool result = Finder.findPath<AStar>(pathNodes);
    std::vector<Vector> path;

}
