#pragma once

#include <opendaq/opendaq.h>

#include <advanced_recorder_module/common.h>

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

struct SignalHandler
{
    virtual void onDataPacketReceived(const DataPacketPtr& packet) = 0;

    virtual ~SignalHandler()
    {
    }
};

END_NAMESPACE_ADVANCED_RECORDER_MODULE
