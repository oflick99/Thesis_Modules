#include <camera_device_module/camera_channel_impl.h>
#include <opendaq/signal_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/binary_data_packet_factory.h>

BEGIN_NAMESPACE_CAMERA_DEVICE_MODULE
    CameraChannelImpl::CameraChannelImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : ChannelImpl(FunctionBlockType("CameraChannel", "Channel", ""), ctx, parent, localId)
{
    m_outputSignal = createAndAddSignal("Value");
}

CameraChannelImpl::~CameraChannelImpl() = default;

/**
 * Defines the data descriptor. This are explicit signals.
 * Camera data is binary data. 
 * If you read out "normal" measurement data you can set it 
 * to Float, int, etc. as well.
 */
void CameraChannelImpl::configure(const SignalPtr& timeSignal)
{
    auto meta = Dict<IString, IString>();
    meta["encoding"] = "jpeg";

    auto dataDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Binary)
                               .setMetadata(meta)
                               .setName("Channel").build();

    auto lock = getRecursiveConfigLock();

    // The data signals needs a domain signal to set it in time. 
    m_outputSignal.setDomainSignal(timeSignal);
    m_outputSignal.setDescriptor(dataDescriptor);
}

/**
 * Is called from the device.
 */
void CameraChannelImpl::addData(const DataPacketPtr& domainPacket, const void* data, size_t sampleCount)
{
    // The Data packet to be created with the data you read. 
    auto dataPacket = BinaryDataPacket(domainPacket, m_outputSignal.getDescriptor(), sampleCount * sizeof(uint8_t));

    auto packetData = dataPacket.getRawData();
    std::memcpy(packetData, data, sampleCount * sizeof(uint8_t));

    // Actually the package which sends data. You are done. 
    m_outputSignal.sendPacket(dataPacket);
}

END_NAMESPACE_CAMERA_DEVICE_MODULE
