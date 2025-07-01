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
#include <opendaq/channel_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <optional>
#include <random>
#include <fstream>
#include <iostream>
 
BEGIN_NAMESPACE_PLAYBACK_DEVICE_MODULE
  
DECLARE_OPENDAQ_INTERFACE(IPlaybackChannel, IBaseObject)
{
    virtual void collectSamples(std::chrono::microseconds curTime) = 0;
};
 
struct PlaybackChannelInit
{
    size_t index;
    std::chrono::microseconds startTime;
    std::chrono::microseconds microSecondsFromEpochToStartTime;
    StringPtr referenceDomainId;
};
 
class PlaybackChannelImpl final : public ChannelImpl<IPlaybackChannel>
{
public:
    explicit PlaybackChannelImpl(const ContextPtr& context,
                            const ComponentPtr& parent,
                            const StringPtr& localId,
                            const PlaybackChannelInit& init);
 
    // IPlaybackChannel
    void collectSamples(std::chrono::microseconds curTime) override;

protected:
    void endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating) override;
 
private:
    size_t index;
    std::ifstream file;
    std::string filePath = "";
    uint64_t samplesGenerated = 0;
    uint64_t deltaT = 1000;
    uint64_t sampleRate = 10;
    RatioPtr resolution = Ratio(1,1000'000);
    double lastValue = 0;
    double zeroOffset = 0;
    std::string epoch = "1970-01-01T00:00:00+00:00";
    std::string timeSignalName = "AI1Time";
    std::string valueSignalName = "AI1";
    UnitPtr timeUnit = Unit("s", -1, "seconds", "time");
    UnitPtr valueUnit = Unit("n/a", -1, "not defined", "n/a");

    std::chrono::microseconds startTime;
    std::chrono::microseconds microSecondsFromEpochToStartTime;
    std::chrono::microseconds lastCollectTime;
    SignalConfigPtr valueSignal;
    SignalConfigPtr timeSignal;

    StringPtr playbackerenceDomainId;
    std::map<std::string, std::function<void()>> fileReaderMap;

    std::vector<double> fileDataBuffer;
    std::vector<double> valueArrayBuffer;
    std::vector<double> dataBuffer;
    uint64_t dataBufferIndex = 0;
 
    void initProperties();
    void filePathChanged();
    void valueDataArrayChanged();
    void dataSourceChanged();
    void openCSVSignal();
    uint64_t getSamplesSinceStart(std::chrono::microseconds time) const;
    void createSignals();
    void removeChannelSignals();
    std::tuple<PacketPtr, PacketPtr> generateSamples(int64_t curTime, uint64_t newSamples);
    void buildSignalDescriptors();
};
 
END_NAMESPACE_PLAYBACK_DEVICE_MODULE