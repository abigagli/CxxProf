
#include "cxxprof_dyn_network/NetworkCxxProf.h"
#include "cxxprof_dyn_network/NetworkActivity.h"
#include "cxxprof_dyn_network/NetworkMark.h"
#include "cxxprof_dyn_network/NetworkPlot.h"
#include "cxxprof_dyn_network/Serializers.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>

#include <stdlib.h> //needed for rand() and srand()
#include <time.h>   //needed for time() -> used for seeding

//NOTE: We need to use the C-API of ZMQ here because Pluma and WinSock2.h both implement a connect() function.
//      Due to extern C in the WinSock part it is not possible to have both defined in the global space -.-
//
//Error Message when using zhelpers.hpp here:
//      C2733: 'connect': second C linkage of overloaded function not allowed
//
#include <zhelpers.h>

#include <iostream>
#include <sstream>

namespace CxxProf
{

    NetworkCxxProf::NetworkCxxProf() :
        actCounter_(1),
        profilingStart_(boost::posix_time::microsec_clock::local_time()),
        zmqContext_(zmq_ctx_new()),
        zmqSender_(zmq_socket(zmqContext_, ZMQ_PUSH)),
        isSending_(false)
    {
        //Initialize the AppInfo, this is needed to identify this application in the data
        //The Starttime is needed to determine when this application has been started. It is important
        //to know when all our events started happening in relation to other applications
        //TODO: How to get a good base value?
        //      - Using the microseconds since 1970 would be a too big value
        //      - We need to have the starttime in microseconds reolution, like the rest
        //      - Perhaps we can assume that the test is started at the same day or something like that...
        info_.Starttime = 0; //profilingStart_

        //The Appname is important for identifying this specific application
        info_.Name = getAppname("App");

        //Connect the sender, this will send every message to the below address
        zmq_connect(zmqSender_, "tcp://localhost:15232");

        //Start the sending Thread. This thread will send the received data every x milliseconds
        sendThread_ = boost::thread(boost::bind(&NetworkCxxProf::sendLoop, this));
    }

    NetworkCxxProf::~NetworkCxxProf()
    {
        isSending_ = false;
        zmq_close(zmqSender_);
        zmq_ctx_destroy(zmqContext_);
    }

    std::string NetworkCxxProf::getAppname(const std::string& appName)
    {
        //The following just generates a random number between 0 and 9999. This is probably
        //good enough to not have the same application number twice.
        //TODO: We should find a better (portable!) way to identify the current application.

        //get the current time via boost (needed to get a unique seed)
        boost::posix_time::ptime time_t_epoch(boost::gregorian::date(2014, 1, 1));
        boost::posix_time::ptime currentTime = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration diff = time_t_epoch - currentTime;
        srand( diff.total_microseconds() );
        unsigned int randomInt = rand() % 9999;
        std::string returnName = appName + "(" + boost::lexical_cast<std::string>(randomInt)+")";
        return returnName;
    }

    boost::shared_ptr<IActivity> NetworkCxxProf::createActivity(const std::string& name)
    {
        unsigned int newActId = actCounter_++;

        //get the current Thread Id, create it if the Thread has no ID yet
        unsigned int threadId = checkThreads();
        
        boost::shared_ptr<NetworkActivity> newAct(new NetworkActivity(name,
            newActId,
            threadId,
            0, //we're setting the parentId later
            profilingStart_,
            boost::bind(&NetworkCxxProf::addResult, this, _1)));

        //check if this is the newest Activity
        //set the ParentId then
        if (activeActivity_.empty())
        {
            newAct->setParentId(0);
        }
        else
        {
            newAct->setParentId(activeActivity_.top());
        }

        //make the new activity the active one
        newAct->start();
        activeActivity_.push(newActId);

        boost::weak_ptr<NetworkActivity> weakAct(newAct);
        activities_[newActId] = weakAct;

        return newAct;
    }

    unsigned int NetworkCxxProf::checkThreads()
    {
        unsigned int threadId = 0;

        //check if the ThreadId already exists
        std::vector<boost::thread::id>::iterator resultIter = std::find(knownThreads_.begin(), knownThreads_.end(), boost::this_thread::get_id());
        if (resultIter == knownThreads_.end()) //if not, add it to the list of known IDs
        {
            knownThreads_.push_back(boost::this_thread::get_id());
            threadId = knownThreads_.size() - 1;

            //also add it to the ThreadIdentifiers
            info_.ThreadAliases[threadId] = "Thread #" + boost::lexical_cast<std::string>(threadId);
        }
        else //if the Id exists, add it as threadId
        {
            //NOTE: See http://stackoverflow.com/questions/2152986/best-way-to-get-the-index-of-an-iterator
            //      on why we do not use std::distance here
            threadId = resultIter - knownThreads_.begin();
        }

        return threadId;
    }

    void NetworkCxxProf::addMark(const std::string& name)
    {
        //Build the NetworkMark
        NetworkMark newMark;
        newMark.Name = name;
        newMark.Timestamp = (boost::posix_time::microsec_clock::local_time() - profilingStart_).total_microseconds();

        //Protect the SendObjects_
        boost::mutex::scoped_lock lock(sendMutex_);
        sendObjects_.Marks.push_back(newMark);
    }
    void NetworkCxxProf::addPlotValue(const std::string& name, double value)
    {
        //Build the NetworkMark
        NetworkPlot newPlot;
        newPlot.Name = name;
        newPlot.Timestamp = (boost::posix_time::microsec_clock::local_time() - profilingStart_).total_microseconds();
        newPlot.Value = value;

        //Protect the SendObjects_
        boost::mutex::scoped_lock lock(sendMutex_);
        sendObjects_.Plots.push_back(newPlot);
    }
    void NetworkCxxProf::addResult(const ActivityResult& result)
    {
        //the mutex protects from different activities calling this callback at once
        boost::mutex::scoped_lock callbackLock(callbackMutex_);

        activeActivity_.pop();
        activities_.erase(result.ActId);

        //Protect the SendObjects_
        boost::mutex::scoped_lock sendLock(sendMutex_);
        sendObjects_.ActivityResults.push_back(result);
    }
    void NetworkCxxProf::setProcessAlias(const std::string name)
    {
        //TODO: It should be checked if there has already been an info sent, if that is the case we shouldn't
        //      change the name anymore.
        //      This can be checked by seeing if addMark, addPlot or addActivity has been called before
        //NOTE: The method getAppName suffixes the given name with a random identifier, just in case that two instances
        //      of the same App are talking to the server
        info_.Name = getAppname(name);
    }
    void NetworkCxxProf::setThreadAlias(const std::string name)
    {
        //TODO: Ensure that the ThreadAlias is only set once, it would get quite complicated
        //      if it changes during we're collecting data.

        //first ensure that the current Thread is known
        unsigned int threadId = checkThreads();

        //then set the alias to the given value
        info_.ThreadAliases[threadId] = name;
    }

    void NetworkCxxProf::shutdown()
    {
        //shutdown all active Activities
        std::map<unsigned int, boost::weak_ptr<NetworkActivity> >::iterator actIter = activities_.begin();
        for (; actIter != activities_.end(); ++actIter)
        {
            boost::shared_ptr<NetworkActivity> lockerAct = actIter->second.lock();
            lockerAct->shutdown();
        }

        //Send the objects
        sendObjects();
    }

    void NetworkCxxProf::sendLoop()
    {
        isSending_ = true;
        while (isSending_)
        {
            //sleep a bit and wait for more objects
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

            //Protect the SendObjects_
            {
                boost::mutex::scoped_lock lock(sendMutex_);

                //let's just check if there is anything to send...
                if (sendObjects_.size() == 0)
                {
                    continue;
                }
            }

            //send everything we have
            sendObjects();
        }
    }

    void NetworkCxxProf::sendObjects()
    {
        //serialize the result
        std::ostringstream serializeStream;

        //Protect the SendObjects_
        {
            boost::mutex::scoped_lock lock(sendMutex_);

            //Put the AppInfo into each packet we're sending
            //This is not the most performant way to do this, but it ensures that the server gets our data 
            sendObjects_.Info = info_;

            boost::archive::text_oarchive oa(serializeStream);
            oa << sendObjects_;
            sendObjects_.clear();
        }

        //send the activity data via network
        std::string serializeString = serializeStream.str();
        zmq_send(zmqSender_, serializeString.c_str(), serializeString.size(), 0);
    }

    std::string NetworkCxxProf::toString() const
    {
        return "NetworkCxxProf";
    }

}
