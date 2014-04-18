
#ifndef _NETWORK_OBJECTS_H_
#define _NETWORK_OBJECTS_H_

#include "brofiler_dyn_network/common.h"
#include "brofiler_dyn_network/NetworkMark.h"
#include "brofiler_dyn_network/NetworkPlot.h"
#include "brofiler_dyn_network/ActivityResult.h"
#include <vector>

struct Brofiler_Dyn_Network_EXPORT NetworkObjects
{
    std::vector<NetworkMark> Marks;
    std::vector<NetworkPlot> Plots;
    std::vector<ActivityResult> ActivityResults;

    void NetworkObjects::clear()
    {
        Marks.clear();
        Plots.clear();
        ActivityResults.clear();
    }
};

#endif //_NETWORK_OBJECTS_H_