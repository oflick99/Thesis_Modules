#include <opendaq/function_block_ptr.h>
#include <opendaq/data_descriptor_ptr.h>

#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>

#include <opendaq/event_packet_params.h>
#include "hbk/opendaq/dispatch.h"
//#include <coreobjects/unit_factory.h>
//#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_factory.h>
//#include <opendaq/range_factory.h>
#include <opendaq/sample_type_traits.h>
//#include <coreobjects/eval_value_factory.h>
//#include <opendaq/reader_factory.h>
//#include <opendaq/reader_config_ptr.h>
#include "static_fatigue_module/sg_subtraction_fb.h"


BEGIN_NAMESPACE_STATIC_FATIGUE_MODULE

using namespace daq;

SgSubtractionFbImpl::SgSubtractionFbImpl(const FunctionBlockTypePtr& type, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
}


FunctionBlockTypePtr SgSubtractionFbImpl::CreateType()
{
    return nullptr;
}

bool SgSubtractionFbImpl::descriptorNotNull(const DataDescriptorPtr& descriptor)
{
    return false;
}

void SgSubtractionFbImpl::getDataDescriptors(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc, DataDescriptorPtr& domainDesc)
{
}

bool SgSubtractionFbImpl::getDataDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc)
{
    return false;
}

bool SgSubtractionFbImpl::getDomainDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& domainDesc)
{
    return false;
}

void SgSubtractionFbImpl::createInputPorts()
{
}

void SgSubtractionFbImpl::createReader()
{
}

void SgSubtractionFbImpl::createSignals()
{
}

RangePtr SgSubtractionFbImpl::getValueRange(const DataDescriptorPtr& voltageDataDescriptor, const DataDescriptorPtr& currentDataDescriptor)
{
    return nullptr;
}

void SgSubtractionFbImpl::onDataReceived()
{
}

void SgSubtractionFbImpl::checkPortConnections() const
{
}

void SgSubtractionFbImpl::onConnected(const InputPortPtr& inputPort)
{
}

void SgSubtractionFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
}

void SgSubtractionFbImpl::configure(const DataDescriptorPtr& domainDescriptor,
                                    const DataDescriptorPtr& voltageDescriptor,
                                    const DataDescriptorPtr& currentDescriptor)
{
}

void SgSubtractionFbImpl::initProperties()
{
}

void SgSubtractionFbImpl::propertyChanged(bool configure)
{
}

void SgSubtractionFbImpl::readProperties()
{
}

END_NAMESPACE_STATIC_FATIGUE_MODULE