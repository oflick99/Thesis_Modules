#include <filesystem>
#include <gmock/gmock.h>
#include <testutils/testutils.h>
#include <opendaq/opendaq.h>
#include <thread>

#include "trigger_fb_module/common.h"
#include "signal_helper.hpp"

using namespace daq;
using TriggerFbModuleTest = testing::Test;

TEST_F(TriggerFbModuleTest, InitFb)
{
    const auto instance = Instance();

    
    ASSERT_TRUE(instance.getAvailableFunctionBlockTypes().hasKey("HbkTriggerFb"));
    ASSERT_TRUE(instance.getAvailableFunctionBlockTypes().hasKey("HbkLogicGateFb"));
    ASSERT_TRUE(instance.getAvailableFunctionBlockTypes().hasKey("HbkTriggerGateFb"));

    // Add instances
    instance.addFunctionBlock("HbkTriggerFb");
    instance.addFunctionBlock("HbkLogicGateFb");
    instance.addFunctionBlock("HbkTriggerGateFb");
}

TEST_F(TriggerFbModuleTest, CommonFunctions)
{
    auto timeA = milliSecondsSinceEpoch();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto timeB = milliSecondsSinceEpoch();
    ASSERT_NEAR(timeB - timeA, 1000, 50);
}

TEST_F(TriggerFbModuleTest, TriggerFbGoodCase)
{
    const auto instance = Instance();
    auto fb = instance.addFunctionBlock("HbkTriggerFb");

    vecvec<Float> mockValuePackets{ {0.2, 0.8, 10.4, 2.1, 4.0, 7.0, 8.0, 9.5}, {1.2, 1.8, 1.4, 1.1, 1.45, 1.9, 1.3, 1.5}};
    
    auto valueSignalEnableHelper = SignalHelper( instance.getContext(),
                                    LinearDataRule(3,2),
                                    SampleTypeFromType<Float>::SampleType,
                                    mockValuePackets);

    auto valueSignalDisableHelper = SignalHelper( instance.getContext(),
                                    LinearDataRule(3,2),
                                    SampleTypeFromType<Float>::SampleType,
                                    mockValuePackets);


    // Check default values:
    ASSERT_EQ(fb.getPropertyValue("Enable.LogicType"), 1);
    StructPtr prop = fb.getPropertyValue("Enable.Level");
    ASSERT_FLOAT_EQ(prop.getFieldValues()[0], 2.0);
    auto myStruct = StructBuilder(prop);
    myStruct.set("Threshold",5.0);
    fb.setPropertyValue("Enable.Level", myStruct.build());

    ASSERT_EQ(fb.getPropertyValue("Disable.LogicType"), 0);
    ASSERT_EQ(fb.getPropertyValue("Disable.Duration"), 1000);


    ASSERT_EQ(fb.getInputPorts().getCount(), 2);
    fb.getInputPorts()[0].connect(valueSignalEnableHelper.getSignal());
    ASSERT_EQ(fb.getInputPorts().getCount(), 2);

    auto reader = PacketReader(fb.getSignals()[0]);

    valueSignalEnableHelper.sendPacket(0);

    bool readPacket = true;
    while (readPacket)
    {
        auto receivedPacket = reader.read();
        if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
        {
            checkReceivedTriggerPacketEqualExpected(receivedPacket,1,8);
            readPacket = false;
        }
    }
}



TEST_F(TriggerFbModuleTest, LogicFbGateGoodCase)
{
    const auto instance = Instance();

    // Add instances
    auto fb = instance.addFunctionBlock("HbkLogicGateFb");

    ASSERT_EQ(fb.getPropertyValue("Type"), 0);
    fb.setPropertyValue("Type", 1);

    vecvec<uint8_t> mockPackets{{0}, {1}, {0}, {1}};
    vecvec<Int> mockDomainPackets{{3}, {29}, {39}, {51}};
    
    auto helper = SignalHelper( instance.getContext(),
                                ExplicitDataRule(),
                                SampleTypeFromType<uint8_t>::SampleType,
                                mockPackets,
                                mockDomainPackets);

    ASSERT_EQ(fb.getInputPorts().getCount(), 1);
    fb.getInputPorts()[0].connect(helper.getSignal());
    ASSERT_EQ(fb.getInputPorts().getCount(), 2);

    helper.sendPackets();

}

TEST_F(TriggerFbModuleTest, TirggerGateGoodCase)
{
    const auto instance = Instance();

    // Add instances
    auto fb = instance.addFunctionBlock("HbkTriggerGateFb");

    vecvec<uint8_t> mockTriggerPackets{{0}, {1}, {0}, {1}, {0}, {1}};
    vecvec<Int> mockTriggerDomainPackets{{3}, {29}, {39}, {51}, {73}, {85}};

    vecvec<Float> mockValuePackets{ {0.2, 0.8, 10.4, 2.1, 4.0, 7.0, 8.0, 9.5}, {1.2, 1.8, 1.4, 1.1, 1.45, 1.9, 1.3, 1.5}};
    
    auto triggerSignalHelper = SignalHelper( instance.getContext(),
                                ExplicitDataRule(),
                                SampleTypeFromType<uint8_t>::SampleType,
                                mockTriggerPackets,
                                mockTriggerDomainPackets);

    auto valueSignalHelper = SignalHelper( instance.getContext(),
                                LinearDataRule(3,2),
                                SampleTypeFromType<Float>::SampleType,
                                mockValuePackets);

    ASSERT_EQ(fb.getInputPorts().getCount(), 2);
    
    fb.getInputPorts()[0].connect(triggerSignalHelper.getSignal());
    ASSERT_EQ(fb.getInputPorts().getCount(), 2);

    fb.getInputPorts()[1].connect(valueSignalHelper.getSignal());
    ASSERT_EQ(fb.getInputPorts().getCount(), 3);

    ASSERT_EQ(valueSignalHelper.getDomainSignal().getGlobalId(), fb.getSignals()[0].getDomainSignal().getGlobalId());

    auto reader = PacketReader(fb.getSignals()[0]);

    triggerSignalHelper.sendPacket(0);
    valueSignalHelper.sendPacket(0);
    size_t packetCount = 0;
    auto packets = reader.readAll();
    for (auto receivedPacket : packets)
    {
        if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
        {
            ASSERT_EQ(1,0);
        }
    }
 
    triggerSignalHelper.sendPacket(1);
    valueSignalHelper.sendPacket(1);

    bool readPacket = true;
    while (readPacket)
    {
        auto receivedPacket = reader.read();
        if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
        {
            checkReceivedPacketEqualExpected(receivedPacket, mockValuePackets[1]);
            readPacket = false;
        }
    }

    // Disable trigger again
    triggerSignalHelper.sendPacket(2);
    valueSignalHelper.sendPacket(1);

    // Reconfigure with Post
    fb.setPropertyValue("ResponseType", 2);
    fb.setPropertyValue("ImmediatelyWithPost", 1000);

    // Enable trigger
    triggerSignalHelper.sendPacket(3);
    valueSignalHelper.sendPacket(1);

    readPacket = true;
    while (readPacket)
    {
        auto receivedPacket = reader.read();
        if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
        {
            checkReceivedPacketEqualExpected(receivedPacket, mockValuePackets[1]);
            readPacket = false;
        }
    }

    // Disable trigger, but values is still going because of post trigger phase
    triggerSignalHelper.sendPacket(4);
    valueSignalHelper.sendPacket(1);

    readPacket = true;
    while (readPacket)
    {
        auto receivedPacket = reader.read();
        if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
        {
            checkReceivedPacketEqualExpected(receivedPacket, mockValuePackets[1]);
            readPacket = false;
        }
    }

    // Sleep 1000ms
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    valueSignalHelper.sendPacket(1);

    // Package should be not anymore forwared.
    packetCount = 0;
    packets = reader.readAll();
    for (auto receivedPacket : packets)
    {
        if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
        {
            ASSERT_EQ(1,0);
        }
    }

    fb.getInputPorts()[1].disconnect();
    ASSERT_EQ(fb.getInputPorts().getCount(), 2);
}