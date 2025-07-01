/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* vim: set ts=4 et sw=4 tw=80: */
/*
 * Copyright (C) 2020 HBK – Hottinger Brüel & Kjær
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

 #pragma once
 #include <playback_device_module/common.h>
 #include <opendaq/channel_ptr.h>
 #include <opendaq/device_impl.h>
 #include <opendaq/logger_ptr.h>
 #include <opendaq/logger_component_ptr.h>
 #include <chrono>
 #include <thread>
 #include <condition_variable>
 #include <opendaq/log_file_info_ptr.h>
 
 BEGIN_NAMESPACE_PLAYBACK_DEVICE_MODULE
 
 class PlaybackDeviceImpl final : public Device
 {
 public:
     explicit PlaybackDeviceImpl(size_t id, const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const StringPtr& name = nullptr);
     ~PlaybackDeviceImpl() override;
 
     static DeviceInfoPtr CreateDeviceInfo(size_t id, const StringPtr& serialNumber = nullptr);
     static DeviceTypePtr CreateType();
 
     // IDevice
     DeviceInfoPtr onGetInfo() override;
     uint64_t onGetTicksSinceOrigin() override;
 
     bool allowAddDevicesFromModules() override;
     bool allowAddFunctionBlocksFromModules() override;
 
 protected:
     ListPtr<ILogFileInfo> onGetLogFileInfos() override;
     StringPtr onGetLog(const StringPtr& id, Int size, Int offset) override;
     std::set<OperationModeType> onGetAvailableOperationModes() override;
  
 private:
     void createSignals();
     void initClock();
     void initIoFolder();
     void initProperties(const PropertyObjectPtr& config);
     void acqLoop();
     void updateNumberOfChannels();
     void updateAcqLoopTime();
     void configureTimeSignal();
     void enableGlobalFileUsage();
     void enableLogging();
     std::chrono::microseconds getMicroSecondsSinceDeviceStart() const;
     PropertyObjectPtr createProtectedObject() const;
 
     size_t id;
     StringPtr serialNumber;
 
     std::thread acqThread;
     std::condition_variable cv;
 
     std::chrono::steady_clock::time_point startTime;
     std::chrono::microseconds startTimeInMs;
     std::chrono::microseconds lastCollectTime;
     std::chrono::microseconds microSecondsFromEpochToDeviceStart;
     
     std::vector<ChannelPtr> channels;
     size_t acqLoopTime;
     bool stopAcq;
 
     FolderConfigPtr aiFolder;
 
     LoggerPtr logger;
     LoggerComponentPtr loggerComponent;
 
     bool loggingEnabled;
     StringPtr loggingPath;
     SignalConfigPtr timeSignal;
     StringPtr refDomainId;
     uint64_t samplesGenerated;
     uint64_t deltaT;
 };
 
 END_NAMESPACE_PLAYBACK_DEVICE_MODULE