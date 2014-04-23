
#ifndef _NETWORK_BROFILER_H_
#define _NETWORK_BROFILER_H_

#include <stack>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include "brofiler_dyn_network/NetworkActivity.h"
#include "brofiler_dyn_network/NetworkObjects.h"
#include "brofiler_dyn_network/common.h"
#include "brofiler_static/IDynBrofiler.h"


namespace Brofiler
{

    class Brofiler_Dyn_Network_EXPORT NetworkBrofiler : public IDynBrofiler
    {
    public:
        NetworkBrofiler();
        virtual ~NetworkBrofiler();

        boost::shared_ptr<IActivity> createActivity(const std::string& name);
        void addMark(const std::string& name);
        void addPlotValue(const std::string& name, double value);
        void shutdown();

        std::string toString() const;

    private:
        void addResult(const ActivityResult& result);
        boost::mutex callbackMutex_;

        bool isSending_;
        boost::thread sendThread_;
        boost::mutex sendMutex_;
        void sendLoop();
        void sendObjects();

        unsigned int actCounter_;
        std::stack<unsigned int> activeActivity_;
        std::map<unsigned int, boost::weak_ptr<NetworkActivity> > activities_;
        boost::posix_time::ptime profilingStart_;
        std::vector<boost::thread::id> knownThreads_;

        NetworkObjects sendObjects_;

        //NOTE: Tried shared-ptrs here, which should work but did not...
        void* zmqContext_;
        void* zmqSender_;
    };

}

#endif //_NETWORK_BROFILER_H_
