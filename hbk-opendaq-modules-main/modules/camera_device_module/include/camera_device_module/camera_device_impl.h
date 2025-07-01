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

#pragma once
#include <thread>

#include <camera_device_module/common.h>
#include <opencv2/core/hal/interface.h>

#include <opendaq/channel_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>

BEGIN_NAMESPACE_CAMERA_DEVICE_MODULE

class CameraDeviceImpl final : public Device
{
public:
    explicit CameraDeviceImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~CameraDeviceImpl() override;

    static DeviceInfoPtr CreateDeviceInfo();
    static std::string getIdFromConnectionString(std::string connectionString);
    static std::string getConnectionStringFromId(std::string id);
    static DeviceTypePtr createType();

    // IDevice
    DeviceInfoPtr onGetInfo() override;
    uint64_t onGetTicksSinceOrigin() override;

private:
    bool m_started;
    LoggerPtr m_logger;
    LoggerComponentPtr m_loggerComponent;
    uint32_t m_sampleRate;
    uint64_t m_samplesCaptured;

    SignalConfigPtr m_timeSignal;

    std::atomic_bool m_quitFlag;
    std::vector<std::thread> m_threads;

    struct CameraInfo
    {
        int index;
        std::string description;
    };

    void initProperties();
    void start();
    void stop();
    void readProperties();
    void createCameraChannels(const std::vector<CameraInfo>& cameras);
    void propertyChanged();
    void configureTimeSignal();
    void configure();

    static std::vector<CameraInfo> getAvailableCameras();

    struct VideoChannelInfo
    {
        int cameraIndex;
        std::string filename;
        ChannelPtr channel;
    };

    std::vector<VideoChannelInfo> m_videoChannels;

    void cameraDataThreadFunction(const std::atomic_bool& quit_flag, const VideoChannelInfo& video_channel, unsigned interval_milliseconds);
    void addData(const ChannelPtr& channel, const uchar* data, size_t sampleCount);
};

END_NAMESPACE_CAMERA_DEVICE_MODULE
