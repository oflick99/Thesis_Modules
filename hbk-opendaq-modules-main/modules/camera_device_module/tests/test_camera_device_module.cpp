/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* vim: set ts=4 et sw=4 tw=80: */
/*
 * Copyright (C) 2024 HBK – Hottinger Brüel & Kjær
 * Skodsborgvej 307
 * DK-2850 Nærum
 * Denmark
 * http://www.hbkworld.com
 * All rights reserved
 *
 * The copyright to the computer program(s) herein is the property of
 * HBK – Hottinger Brüel & Kjær (HBK), Denmark. The program(s)
 * may be used and/or copied only with the written permission of HBM
 * or in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 * This copyright notice must not be removed.
 *
 * This Software is licenced by the
 * "General supply and license conditions for software"
 * which is part of the standard terms and conditions of sale from HBM.
 */

#include <thread>
#include <chrono>
#include <filesystem>

#include "camera_device_module_factories.h"

using CameraDeviceModule = testing::Test;

using namespace daq;
using namespace lanfactory;

TEST_F(CameraDeviceModule, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(CameraDeviceModule, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "CameraDeviceModule");
}

TEST_F(CameraDeviceModule, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(CameraDeviceModule, EnumerateDevices)
{
    auto module = CreateModule();

    ListPtr<IDeviceInfo> deviceInfoDict;
    ASSERT_NO_THROW(deviceInfoDict = module.getAvailableDevices());
    ASSERT_EQ(deviceInfoDict.getCount(), 1u);

    ASSERT_EQ(deviceInfoDict[0].getConnectionString(), "camera://cameraABC");
    ASSERT_EQ(deviceInfoDict[0].getModel(), "123");
    ASSERT_EQ(deviceInfoDict[0].getDeviceClass(), "");
    ASSERT_EQ(deviceInfoDict[0].getManufacturer(), "");
    ASSERT_EQ(deviceInfoDict[0].getSdkVersion(), "3.19.3_9403b394");
    ASSERT_EQ(deviceInfoDict[0].getLocation(), "");
    ASSERT_EQ(deviceInfoDict[0].getSystemType(), "");
    ASSERT_EQ(deviceInfoDict[0].getSystemUuid(), "");
}

TEST_F(CameraDeviceModule, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);

    for (const auto deviceType : deviceTypes)
    {
        ASSERT_EQ(deviceType.first, "Camera");
        ASSERT_EQ(deviceType.second.getId(), "Camera");
        ASSERT_EQ(deviceType.second.getName(), "Camera device");
        ASSERT_EQ(deviceType.second.getDescription(), "A camera device which es able to read cameras connected to the PC.");
        ASSERT_EQ(deviceType.second.getConnectionStringPrefix(), "camera");
    }

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_TRUE(functionBlockTypes.assigned());
    ASSERT_EQ(functionBlockTypes.getCount(), 0u);
}

TEST_F(CameraDeviceModule, AddDevice)
{    
    auto path = std::filesystem::current_path();
    auto opendaqInstance = daq::Instance(path.string());
    opendaqInstance.setRootDevice("camera://cameraABC");
    daq::DevicePtr device = opendaqInstance.getRootDevice();

    //ASSERT_EQ(device.getChannels().getCount(), 1);      // at home
    //ASSERT_EQ(device.getChannels().getCount(), 2);      // in the office

    //ASSERT_EQ(channel.getPropertyValue("aaa"), 10000);
    //device.setPropertyValue("aaa", 100);
}