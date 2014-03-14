
#ifndef _I_DYN_BROFILER_H_
#define _I_DYN_BROFILER_H_

#include "brofiler/IActivity.h"
#include <boost/shared_ptr.hpp>

class IDynBrofiler
{
public:
    virtual ~IDynBrofiler() {};
    virtual boost::shared_ptr<IActivity> createActivity(const std::string& name) const = 0;
};

#endif //_I_DYN_BROFILER_H_
