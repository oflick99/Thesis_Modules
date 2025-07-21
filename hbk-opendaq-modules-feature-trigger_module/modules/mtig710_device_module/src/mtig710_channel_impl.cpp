#include <mtig710_device_module/mtig710_channel_impl.h>
#include <opendaq/signal_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>

BEGIN_NAMESPACE_MTIG710_DEVICE_MODULE

Mtig710ChannelImpl::Mtig710ChannelImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : ChannelImpl(FunctionBlockType("Mtig710Channel", "Channel", ""), ctx, parent, localId)
{
    m_outputSignal = createAndAddSignal("Value");
}

Mtig710ChannelImpl::~Mtig710ChannelImpl() = default;

/**
 * Defines the data descriptor. This are explicit signals.
 * Camarera data is binarry data. 
 * If you read out "normal" measurement data you can set it 
 * to Float, int, etc. as well.
 */
void Mtig710ChannelImpl::configure(const SignalPtr& timeSignal)
{
    auto dataDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Float64)
                               .setValueRange(Range(-1.0, 1.0))
                               .setName("Channel").build();

    auto lock = getRecursiveConfigLock();

    // The data signals needs a domain signal to set it in time. 
    m_outputSignal.setDomainSignal(timeSignal);
    m_outputSignal.setDescriptor(dataDescriptor);
}

/**
 * Is called from the device.
 */
void Mtig710ChannelImpl::addData(const DataPacketPtr& domainPacket, const void* data, size_t sampleCount)
{
    // The Data packet to be created with the data you read. 
    auto dataPacket = DataPacketWithDomain(domainPacket, m_outputSignal.getDescriptor(), sampleCount);

    auto packetData = dataPacket.getRawData();
    std::memcpy(packetData, data, sampleCount * sizeof(float));

    // Actually the package which sents data. You are done. 
    m_outputSignal.sendPacket(dataPacket);
}

END_NAMESPACE_MTIG710_DEVICE_MODULE
