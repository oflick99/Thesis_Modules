#include <filesystem>
#include <gmock/gmock.h>
#include <testutils/testutils.h>
#include <opendaq/opendaq.h>

using namespace daq;
using PlaybackDeviceModuleTest = testing::Test;

TEST_F(PlaybackDeviceModuleTest, SucessPath)
{
    const auto instance = Instance();

    bool foundDeviceType = false;
    for (auto devTypes: instance.getAvailableDeviceTypes())
    {
        if (devTypes.first.toStdString().compare("daqpb") == 0)
            foundDeviceType = true; 
    }
    ASSERT_EQ(foundDeviceType, true);

    size_t foundDeviceCounter = 0;
    for (auto devInfo : instance.getAvailableDevices())
    {
        std::string compareString = "daqpb://device" + std::to_string(foundDeviceCounter);
        if (devInfo.getConnectionString().toStdString().compare(compareString) == 0)
            ++foundDeviceCounter;
    }
    ASSERT_EQ(foundDeviceCounter, 2);


    auto dev = instance.addDevice("daqpb://device0");
    for (auto chan : dev.getChannels())
    {
        chan.setPropertyValue("FilePath",
            (std::filesystem::current_path() / "modules/playback_device_module/tests/files/example_recording.csv")
                .lexically_normal()
                .string());
        chan.setPropertyValue("DataSource", 1);
        
        // Create some data
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // Ensure some data is generated
        for (auto sig : chan.getSignals())
        {
            ASSERT_STREQ(sig.getDomainSignal().getName().toStdString().c_str(), "AI0Time"); 
            ASSERT_STREQ(sig.getName().toStdString().c_str(), "AI0");  
            ASSERT_STREQ(sig.getDescriptor().getUnit().getSymbol().toStdString().c_str(),"V");
            ASSERT_EQ(sig.getDescriptor().getUnit().getId(), -1);
            ASSERT_STREQ(sig.getDescriptor().getUnit().getName().toStdString().c_str(), "volts");
            ASSERT_STREQ(sig.getDescriptor().getUnit().getQuantity().toStdString().c_str(), "voltage");  
            
            // Sine signal has values between -5 and 5 in csv file
            double lastValue = sig.getLastValue();
            ASSERT_LE(lastValue, 5.0);
            ASSERT_GE(lastValue, -5.0);

            // Switch to Array to check Zeroing
            chan.setPropertyValue("DataSource", 0);
            chan.setPropertyValue("ValueArray", List<IFloat>(2.0, 2.0, 2.0));
            
            // Wait some time, so that switch gets active and new values apply
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            FunctionPtr zeroFunc = chan.getPropertyValue("Zero");
            double zeroOffset = zeroFunc(0.0, 0);
            ASSERT_DOUBLE_EQ(zeroOffset, -2.0);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            zeroOffset = zeroFunc(1.0, 0);
            ASSERT_DOUBLE_EQ(zeroOffset, -1.0);
        }
        ASSERT_EQ(chan.getPropertyValue("ValueMetaData").asPtr<IDict>().get("SampleRate"), 1000);
        ASSERT_EQ(chan.getPropertyValue("ValueMetaData").asPtr<IDict>().get("UnitId"), -1);

        // Check if key type is string and value type is int
        auto prop = chan.getProperty("ValueMetaData");
        ASSERT_EQ(prop.getKeyType(), 3);
        ASSERT_EQ(prop.getValueType(), 5);

        // Create specific dict based on type
        DictPtr<IString, IInteger> dict = chan.getPropertyValue("ValueMetaData");
        dict.set("SampleRate", 500);
        dict.set("UnitId", 5);
        chan.setPropertyValue("ValueMetaData", dict);

        // Check if change applied
        ASSERT_EQ(chan.getPropertyValue("ValueMetaData").asPtr<IDict>().get("SampleRate"), 500);
        ASSERT_EQ(chan.getPropertyValue("ValueMetaData").asPtr<IDict>().get("UnitId"), 5);
        ASSERT_EQ(chan.getPropertyValue("ValueMetaData").asPtr<IDict>().getCount(), 2);

        
    }

}

TEST_F(PlaybackDeviceModuleTest, ErrorPath)
{
    const auto instance = Instance();

    auto dev = instance.addDevice("daqpb://device0");
    for (auto chan : dev.getChannels())
    {
        chan.setPropertyValue("DataSource", 1);
        chan.setPropertyValue("FilePath", "bla.csv");
        
        // Create some data
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ASSERT_EQ(chan.getSignals().empty(), true);
    }
}

TEST_F(PlaybackDeviceModuleTest, ChangePathAfterSucessfullCreation)
{
    const auto instance = Instance();
    auto dev = instance.addDevice("daqpb://device0");
    for (auto chan : dev.getChannels())
    {
        chan.setPropertyValue("DataSource", 1);
        chan.setPropertyValue("FilePath",
            (std::filesystem::current_path() / "modules/playback_device_module/tests/files/example_recording.csv")
                .lexically_normal()
                .string());
        
        // Create some data
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // Ensure some data is generated
        for (auto sig : chan.getSignals())
        {
            ASSERT_STREQ(sig.getDomainSignal().getName().toStdString().c_str(), "AI0Time"); 
            ASSERT_STREQ(sig.getName().toStdString().c_str(), "AI0");  
            ASSERT_STREQ(sig.getDescriptor().getUnit().getSymbol().toStdString().c_str(),"V");
            ASSERT_EQ(sig.getDescriptor().getUnit().getId(), -1);
            ASSERT_STREQ(sig.getDescriptor().getUnit().getName().toStdString().c_str(), "volts");
            ASSERT_STREQ(sig.getDescriptor().getUnit().getQuantity().toStdString().c_str(), "voltage");  
            

            FunctionPtr zeroFunc = chan.getPropertyValue("Zero");
            double zeroOffset = zeroFunc(0.0,0);
            ASSERT_LE(zeroOffset, 5.0);
            ASSERT_GE(zeroOffset, -5.0);
   
        }
        ASSERT_EQ(chan.getSignals().empty(), false);
        ASSERT_EQ(chan.getPropertyValue("ValueMetaData").asPtr<IDict>().get("SampleRate"), 1000);
        ASSERT_EQ(chan.getPropertyValue("ValueMetaData").asPtr<IDict>().get("UnitId"), -1);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (auto chan : dev.getChannels())
    {
        chan.setPropertyValue("FilePath", "bla.csv");
        
        // Create some data
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ASSERT_EQ(chan.getSignals().empty(), false);
    }

}

TEST_F(PlaybackDeviceModuleTest, ArrayTest)
{
    const auto instance = Instance();

    auto dev = instance.addDevice("daqpb://device0");
    for (auto chan : dev.getChannels())
    {
        auto prop = chan.getProperty("ValueArray");
        ASSERT_EQ(prop.getValueType(), 4); // Check if it is a list
        ASSERT_EQ(prop.getItemType(), 2); // Check if it is float

        ListPtr<IFloat> propList = chan.getPropertyValue("ValueArray");
        ASSERT_EQ(propList.getCount(), 5);
        double value = 1.0;
        for (auto item : propList)
        {
            ASSERT_DOUBLE_EQ(item, value);
            value += 1.0;
        }
        // set a new value
        chan.setPropertyValue("ValueArray", List<IFloat>(2.0, 2.0, 2.0));
        propList = chan.getPropertyValue("ValueArray");
        for (auto item : propList)
        {
            ASSERT_DOUBLE_EQ(item, 2.0);
        }

    }
}
