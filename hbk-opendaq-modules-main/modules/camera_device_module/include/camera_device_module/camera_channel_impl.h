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
#include <camera_device_module/common.h>

#include <opendaq/channel_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/sample_type.h>
#include <optional>
#include <random>
#include <opendaq/data_packet_ptr.h>
#include <mutex>

BEGIN_NAMESPACE_CAMERA_DEVICE_MODULE

DECLARE_OPENDAQ_INTERFACE(ICameraChannel, IBaseObject)
{
    virtual void configure(const SignalPtr& timeSignal) = 0;
    virtual void addData(const DataPacketPtr& domainPacket, const void* data, size_t sampleCount) = 0;
};

class CameraChannelImpl final : public ChannelImpl<ICameraChannel>
{
public:
    explicit CameraChannelImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~CameraChannelImpl() override;

    void configure(const SignalPtr& timeSignal) override;
    void addData(const DataPacketPtr& domainPacket, const void* data, size_t sampleCount) override;

private:
    SignalConfigPtr m_outputSignal;
};

END_NAMESPACE_CAMERA_DEVICE_MODULE
