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
#include <opendaq/function_block_impl.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ptr.h>

#include "trigger_fb_module/common.h"
 
BEGIN_NAMESPACE_TRIGGER_FB_MODULE

static const char TRIGGER_FB_MODULE_TIGGER_GATE_STR[] = "HbkTriggerGateFb";

static const char IMMEDIATLEY_ENABLED_STR[] = "ImmediatelyEnabled";
static const char PRE_ENABLED_STR[] = "PreEnabled";
static const char POST_ENABLED_STR[] = "PostEnabled";
static const char PRE_AND_POST_ENABLED_STR[] = "PreAndPostEnabled";

static const char TRIGGER_GATE_SETTINGS_STR[] = "Settings";
static const char SETTINGS_RE_TRIGGER_STR[] = "ReTrigger";

static const char RESPONSE_TYPE_STR[] = "ResponseType";
static const char IMMEDIATLEY_RESPONSE_TYPE_STR[] = "Immediately";
static const char PRE_RESPONSE_TYPE_STR[] = "Pre";
static const char POST_RESPONSE_TYPE_STR[] = "ImmediatelyWithPost";
static const char PRE_AND_POST_RESPONSE_TYPE_STR[] = "PreAndPost";

static const char PRE_TIME_STR[] = "PreTime";
static const char POST_TIME_STR[] = "PostTime";

enum ResponseType: int {
    RESPONSE_TYPE_IMMEDIATLEY = 0,
    RESPONSE_TYPE_PRE = 1,
    RESPONSE_TYPE_POST = 2,
    RESPONSE_TYPE_PRE_AND_POST = 3,
};

struct GateContext
{
    size_t index;
    InputPortConfigPtr inputPort;

    SampleType inputSampleType;
    size_t inputSampleDimension;
    uint8_t buffer[TRIGGER_GATE_BUFFER_SIZE];

    DataDescriptorPtr outputDataDescriptor;
    DataDescriptorPtr outputDomainDataDescriptor;
 
    SignalConfigPtr outputSignal;
};


class TriggerGateFbImpl final : public FunctionBlock
{

public:

    enum GateState: int {
        OFF = 0,
        PRE = 1, // Storing to buffer not sending
        PRE_ACTIVE = 2, //Active but sending data still from the buffers
        ACTIVE = 3, // Sending without buffering
        POST_ACTIVE = 4, // Sending data until post time is expired
    };

    explicit TriggerGateFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~TriggerGateFbImpl() override = default;
 
    static FunctionBlockTypePtr CreateType();

    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;
 
private:
    InputPortPtr m_triggerInput;

    std::vector<GateContext> m_gateContexts;
    size_t m_portCount;
  
    GateState m_gateState;
    ResponseType m_responseType;
    int64_t m_postTimeInMs;
    int64_t m_retriggerTimeInMs;
    int64_t m_postTimeElapsedInMs;
    bool m_postTriggerActive;


    DataDescriptorPtr m_inputTriggerDataDescriptor;
    DataDescriptorPtr m_inputTriggerDomainDataDescriptor;

    PacketReadyNotification packetReadyNotification;
 
    void createTriggerPort();
    void updateValueInputPorts();
  
    void onPacketReceived(const InputPortPtr& port) override;

    void processTriggerPackage(const DataPacketPtr& packet);
    template <SampleType InputSampleType>
    void processDataPacket(const DataPacketPtr& packet);
 
    void processTriggerEventPacket(const EventPacketPtr& packet);
    void processValueEventPacket(const EventPacketPtr& packet, GateContext& gateContext);

    void processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor, const DataDescriptorPtr& inputDomainDataDescriptor);
 
    void configure();
 
    void initProperties(const PropertyObjectPtr& config);
    void propertyChanged();
    void readProperties();
};
 
END_NAMESPACE_TRIGGER_FB_MODULE