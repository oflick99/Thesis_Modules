#include <filesystem>
#include <gmock/gmock.h>
#include <testutils/testutils.h>
#include <opendaq/opendaq.h>
#include <thread>

#include "basic_math_module/common.h"
#include "signal_helper.hpp"

using namespace daq;
using MathSumFbModuleTest = testing::Test;

// // Test 1: create fb and initialise it
// TEST_F(MathSumFbModuleTest, InitFb)
// {
//     const auto instance = Instance();
    
  
//     const auto fbTypes = instance.getAvailableFunctionBlockTypes();
//     std::cout << "Verfuegbare Funktionsbloecke:\n";
//     for (const auto& key : fbTypes.getKeys())
//     {
//         std::cout << " - " << key << "\n";
//     }

//     ASSERT_TRUE(instance.getAvailableFunctionBlockTypes().hasKey("RefFBModuleMathSum"));
    

//      //Add instances
//      instance.addFunctionBlock("RefFBModuleMathSum");
// }
// // Test 2: Basis Test - create fb and connect signals to ports
// TEST_F(MathSumFbModuleTest, BasicConnectionTest)
// {
//     const auto instance = Instance();
//     auto fb = instance.addFunctionBlock("RefFBModuleMathSum");
    
//     // Create Mock Signals for both Inputs
//     vecvec<Float> mockData1{ {1.0, 1.0, 1.0, 1.0, 1.0} };
//     vecvec<Float> mockData2{ {1.0, 1.0, 1.0, 1.0, 1.0} };
//     vecvec<Float> mockData3{ {0.1, 0.2, 0.3, 0.4, 0.5} };
    
//     auto measuringSignalHelper1 = SignalHelper<Float>(
//         instance.getContext(),
//         LinearDataRule(1, 0),  // Delta=1, Start=0
//         SampleTypeFromType<Float>::SampleType,
//         mockData1
//     );
    
//     auto measuringSignalHelper2 = SignalHelper<Float>(
//         instance.getContext(),
//         LinearDataRule(1, 0),  // Delta=1, Start=0
//         SampleTypeFromType<Float>::SampleType,
//         mockData2
//     );

//     auto measuringSignalHelper3 = SignalHelper<Float>(
//         instance.getContext(),
//         LinearDataRule(1, 0),  // Delta=1, Start=0
//         SampleTypeFromType<Float>::SampleType,
//         mockData3
//     );
    
//     // check number of Input Ports
//     ASSERT_EQ(fb.getInputPorts().getCount(), 1);
    
//     // connect the signals to the ports
//     fb.getInputPorts()[0].connect(measuringSignalHelper1.getSignal());  // Data1
//     fb.getInputPorts()[1].connect(measuringSignalHelper2.getSignal());      // Data2
//     fb.getInputPorts()[0].connect(measuringSignalHelper3.getSignal());  // Data3

//     // check number of Input Ports
//     ASSERT_EQ(fb.getInputPorts().getCount(), 3);
    
//     // check, if a output-signal exists
//     ASSERT_EQ(fb.getSignals().getCount(), 1);  // Sum
// }

// // Test 3:  summationtest with standard parameters
// TEST_F(MathSumFbModuleTest, SimpleAdditionTest)
// {
//     const auto instance = Instance();
//     auto fb = instance.addFunctionBlock("RefFBModuleMathSum");
    
//     // Test Data:
//     vecvec<Float> mockData1{ {1.0, 1.0, 1.0, 1.0, 1.0} };
//     vecvec<Float> mockData2{ {1.0, 1.0, 1.0, 1.0, 1.0} };
//     vecvec<Float> mockData3{ {0.1, 0.2, 0.3, 0.4, 0.5} };
//     std::vector<Float> expectedResult{ {2.1, 2.2, 2.3, 2.4, 2.5} };
    
//     auto measuringSignalHelper1 = SignalHelper<Float>(
//         instance.getContext(),
//         LinearDataRule(1, 0),  // Delta=1, Start=0
//         SampleTypeFromType<Float>::SampleType,
//         mockData1
//     );
    
//     auto measuringSignalHelper2 = SignalHelper<Float>(
//         instance.getContext(),
//         LinearDataRule(1, 0),  // Delta=1, Start=0
//         SampleTypeFromType<Float>::SampleType,
//         mockData2
//     );

//     auto measuringSignalHelper3 = SignalHelper<Float>(
//         instance.getContext(),
//         LinearDataRule(1, 0),  // Delta=1, Start=0
//         SampleTypeFromType<Float>::SampleType,
//         mockData3
//     );
    
//     // connect signals
//     fb.getInputPorts()[0].connect(measuringSignalHelper1.getSignal());  // Data1
//     fb.getInputPorts()[1].connect(measuringSignalHelper2.getSignal());      // Data2
//     fb.getInputPorts()[2].connect(measuringSignalHelper3.getSignal());  // Data3
    
//     // create reader for output signal
//     auto reader = PacketReader(fb.getSignals()[0]);  // CompensatedStrain Signal ([0] would be the domainsignal)
    
//     // send data
//     measuringSignalHelper1.sendPacket(0);
//     measuringSignalHelper2.sendPacket(0);
//     measuringSignalHelper3.sendPacket(0);
    
//     // wait for data processing
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
//     // read output packet
//     bool packetReceived = false;
    
//     SizeT cnt = reader.getAvailableCount();
//     ASSERT_EQ(cnt, 1);  // Expecting 2 packets to be available
//     std::cout << "teststelle 1" << std::endl;

//     auto receivedPacket = reader.read();
//     std::cout << "teststelle 2" << std::endl;

//     ASSERT_EQ(receivedPacket.getType(), PacketType::Event);
//     std::cout << "teststelle 3" << std::endl;

//     receivedPacket = reader.read();  // Read the packet again to ensure we get the correct one
//     std::cout << "teststelle 4" << std::endl;

//     ASSERT_EQ(receivedPacket.getType(), PacketType::Data);
//    std::cout << "teststelle 5" << std::endl;
   
//     if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
//     {
//         std::cout << "teststelle 6" << std::endl;
//         checkReceivedPacketEqualExpected<Float>(receivedPacket, expectedResult);
//         packetReceived = true;
//     }
//     std::cout << "teststelle 7" << std::endl;
//     ASSERT_TRUE(packetReceived);
// }
// TEST_F(TriggerFbModuleTest, CommonFunctions)
// {
//     auto timeA = milliSecondsSinceEpoch();
//     std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     auto timeB = milliSecondsSinceEpoch();
//     ASSERT_NEAR(timeB - timeA, 1000, 50);
// }

// TEST_F(TriggerFbModuleTest, TriggerFbGoodCase)
// {
//     const auto instance = Instance();
//     auto fb = instance.addFunctionBlock("HbkTriggerFb");

//     vecvec<Float> mockValuePackets{ {0.2, 0.8, 10.4, 2.1, 4.0, 7.0, 8.0, 9.5}, {1.2, 1.8, 1.4, 1.1, 1.45, 1.9, 1.3, 1.5}};
    
//     auto valueSignalEnableHelper = SignalHelper( instance.getContext(),
//                                     LinearDataRule(3,2),
//                                     SampleTypeFromType<Float>::SampleType,
//                                     mockValuePackets);

//     auto valueSignalDisableHelper = SignalHelper( instance.getContext(),
//                                     LinearDataRule(3,2),
//                                     SampleTypeFromType<Float>::SampleType,
//                                     mockValuePackets);


//     // Check default values:
//     ASSERT_EQ(fb.getPropertyValue("Enable.LogicType"), 1);
//     StructPtr prop = fb.getPropertyValue("Enable.Level");
//     ASSERT_FLOAT_EQ(prop.getFieldValues()[0], 2.0);
//     auto myStruct = StructBuilder(prop);
//     myStruct.set("Threshold",5.0);
//     fb.setPropertyValue("Enable.Level", myStruct.build());

//     ASSERT_EQ(fb.getPropertyValue("Disable.LogicType"), 0);
//     ASSERT_EQ(fb.getPropertyValue("Disable.Duration"), 1000);


//     ASSERT_EQ(fb.getInputPorts().getCount(), 2);
//     fb.getInputPorts()[0].connect(valueSignalEnableHelper.getSignal());
//     ASSERT_EQ(fb.getInputPorts().getCount(), 2);

//     auto reader = PacketReader(fb.getSignals()[0]);

//     valueSignalEnableHelper.sendPacket(0);

//     bool readPacket = true;
//     while (readPacket)
//     {
//         auto receivedPacket = reader.read();
//         if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
//         {
//             checkReceivedTriggerPacketEqualExpected(receivedPacket,1,8);
//             readPacket = false;
//         }
//     }
// }



// TEST_F(TriggerFbModuleTest, LogicFbGateGoodCase)
// {
//     const auto instance = Instance();

//     // Add instances
//     auto fb = instance.addFunctionBlock("HbkLogicGateFb");

//     ASSERT_EQ(fb.getPropertyValue("Type"), 0);
//     fb.setPropertyValue("Type", 1);

//     vecvec<uint8_t> mockPackets{{0}, {1}, {0}, {1}};
//     vecvec<Int> mockDomainPackets{{3}, {29}, {39}, {51}};
    
//     auto helper = SignalHelper( instance.getContext(),
//                                 ExplicitDataRule(),
//                                 SampleTypeFromType<uint8_t>::SampleType,
//                                 mockPackets,
//                                 mockDomainPackets);

//     ASSERT_EQ(fb.getInputPorts().getCount(), 1);
//     fb.getInputPorts()[0].connect(helper.getSignal());
//     ASSERT_EQ(fb.getInputPorts().getCount(), 2);

//     helper.sendPackets();

// }

// TEST_F(TriggerFbModuleTest, TirggerGateGoodCase)
// {
//     const auto instance = Instance();

//     // Add instances
//     auto fb = instance.addFunctionBlock("HbkTriggerGateFb");

//     vecvec<uint8_t> mockTriggerPackets{{0}, {1}, {0}, {1}, {0}, {1}};
//     vecvec<Int> mockTriggerDomainPackets{{3}, {29}, {39}, {51}, {73}, {85}};

//     vecvec<Float> mockValuePackets{ {0.2, 0.8, 10.4, 2.1, 4.0, 7.0, 8.0, 9.5}, {1.2, 1.8, 1.4, 1.1, 1.45, 1.9, 1.3, 1.5}};
    
//     auto triggerSignalHelper = SignalHelper( instance.getContext(),
//                                 ExplicitDataRule(),
//                                 SampleTypeFromType<uint8_t>::SampleType,
//                                 mockTriggerPackets,
//                                 mockTriggerDomainPackets);

//     auto valueSignalHelper = SignalHelper( instance.getContext(),
//                                 LinearDataRule(3,2),
//                                 SampleTypeFromType<Float>::SampleType,
//                                 mockValuePackets);

//     ASSERT_EQ(fb.getInputPorts().getCount(), 2);
    
//     fb.getInputPorts()[0].connect(triggerSignalHelper.getSignal());
//     ASSERT_EQ(fb.getInputPorts().getCount(), 2);

//     fb.getInputPorts()[1].connect(valueSignalHelper.getSignal());
//     ASSERT_EQ(fb.getInputPorts().getCount(), 3);

//     ASSERT_EQ(valueSignalHelper.getDomainSignal().getGlobalId(), fb.getSignals()[0].getDomainSignal().getGlobalId());

//     auto reader = PacketReader(fb.getSignals()[0]);

//     triggerSignalHelper.sendPacket(0);
//     valueSignalHelper.sendPacket(0);
//     size_t packetCount = 0;
//     auto packets = reader.readAll();
//     for (auto receivedPacket : packets)
//     {
//         if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
//         {
//             ASSERT_EQ(1,0);
//         }
//     }
 
//     triggerSignalHelper.sendPacket(1);
//     valueSignalHelper.sendPacket(1);

//     bool readPacket = true;
//     while (readPacket)
//     {
//         auto receivedPacket = reader.read();
//         if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
//         {
//             checkReceivedPacketEqualExpected(receivedPacket, mockValuePackets[1]);
//             readPacket = false;
//         }
//     }

//     // Disable trigger again
//     triggerSignalHelper.sendPacket(2);
//     valueSignalHelper.sendPacket(1);

//     // Reconfigure with Post
//     fb.setPropertyValue("ResponseType", 2);
//     fb.setPropertyValue("ImmediatelyWithPost", 1000);

//     // Enable trigger
//     triggerSignalHelper.sendPacket(3);
//     valueSignalHelper.sendPacket(1);

//     readPacket = true;
//     while (readPacket)
//     {
//         auto receivedPacket = reader.read();
//         if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
//         {
//             checkReceivedPacketEqualExpected(receivedPacket, mockValuePackets[1]);
//             readPacket = false;
//         }
//     }

//     // Disable trigger, but values is still going because of post trigger phase
//     triggerSignalHelper.sendPacket(4);
//     valueSignalHelper.sendPacket(1);

//     readPacket = true;
//     while (readPacket)
//     {
//         auto receivedPacket = reader.read();
//         if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
//         {
//             checkReceivedPacketEqualExpected(receivedPacket, mockValuePackets[1]);
//             readPacket = false;
//         }
//     }

//     // Sleep 1000ms
//     std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     valueSignalHelper.sendPacket(1);

//     // Package should be not anymore forwared.
//     packetCount = 0;
//     packets = reader.readAll();
//     for (auto receivedPacket : packets)
//     {
//         if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
//         {
//             ASSERT_EQ(1,0);
//         }
//     }

//     fb.getInputPorts()[1].disconnect();
//     ASSERT_EQ(fb.getInputPorts().getCount(), 2);
// }