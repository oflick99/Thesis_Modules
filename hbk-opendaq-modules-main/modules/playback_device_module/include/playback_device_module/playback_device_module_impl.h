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
 #include <opendaq/module_impl.h>
 
 BEGIN_NAMESPACE_PLAYBACK_DEVICE_MODULE
 
 class PlaybackDeviceModule final : public Module
 {
 public:
     explicit PlaybackDeviceModule(ContextPtr context);
 
     ListPtr<IDeviceInfo> onGetAvailableDevices() override;
     DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
     DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;
 
 private:
     std::vector<WeakRefPtr<IDevice>> devices;
     std::mutex sync;
     size_t maxNumberOfDevices;
 
     size_t getIdFromConnectionString(const std::string& connectionString) const;
     void clearRemovedDevices();
 };
 
 END_NAMESPACE_PLAYBACK_DEVICE_MODULE