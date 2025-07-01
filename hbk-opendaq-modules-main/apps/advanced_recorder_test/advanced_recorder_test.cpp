#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include <opendaq/opendaq.h>

using namespace daq;
using namespace std::chrono_literals;

int main(int argc, char *argv[])
{
    std::cout << "Looking for modules in " << OPENDAQ_MODULES_DIR << std::endl;
    auto instance = InstanceBuilder()
        .addModulePath(OPENDAQ_MODULES_DIR)
        .build();

    PropertyObjectPtr config;

    std::cout << "Available Device Types:" << std::endl;
    for (const auto [deviceTypeId, deviceType] : instance.getAvailableDeviceTypes())
    {
        std::cout << "    "
            << deviceType.getConnectionStringPrefix() << ": "
            << deviceType.getId() << ": "
            << deviceType.getName() << " ("
            << deviceType.getDescription() << ")"
            << std::endl;
        if (deviceTypeId == "daqref")
            config = deviceType.createDefaultConfig();
    }
    std::cout << std::endl;

    std::cout << "Available Devices:" << std::endl;
    for (const auto deviceInfo : instance.getAvailableDevices())
        std::cout << "    "
            << deviceInfo.getConnectionString() << ": "
            << deviceInfo.getName()
            << std::endl;
    std::cout << std::endl;

    config.setPropertyValue("EnableCANChannel", True);
    auto device = instance.addDevice("daqref://device0", config);

    std::cout << "Device Signals:" << std::endl;
    for (const auto& signal : device.getSignalsRecursive())
        std::cout << "    - " << signal.getLocalId() << std::endl;
    std::cout << std::endl;

    std::cout << "Function Block Types:" << std::endl;
    for (const auto [fbTypeId, fbType] : device.getAvailableFunctionBlockTypes())
        std::cout << "    " << fbType.getId() << ": "
            << fbType.getName()
            << std::endl;
    std::cout << std::endl;

    auto fb = instance.addFunctionBlock("AdvancedRecorder");
    fb.setPropertyValue("Filename", (std::filesystem::current_path() / "test.sie").c_str());

    std::cout << "Function Block Inputs:" << std::endl;
    for (const auto inputPort : fb.getInputPorts())
        std::cout << "    " << inputPort.getLocalId()
            << std::endl;
    std::cout << std::endl;

    auto value1 = fb.getInputPorts(search::LocalId("Value1"))[0];
    auto ai0 = device.getSignalsRecursive(search::LocalId("AI0"))[0];
    value1.connect(ai0);

    auto value2 = fb.getInputPorts(search::LocalId("Value2"))[0];
    auto ai1 = device.getSignalsRecursive(search::LocalId("AI1"))[0];
    value2.connect(ai1);

    auto value3 = fb.getInputPorts(search::LocalId("Value3"))[0];
    auto can1 = device.getSignalsRecursive(search::LocalId("CAN"))[0];
    value3.connect(can1);

    auto recorder = fb.asPtr<IRecorder>(false);
    recorder.startRecording();

    std::this_thread::sleep_for(1s);

    recorder.stopRecording();

    return 0;
}
