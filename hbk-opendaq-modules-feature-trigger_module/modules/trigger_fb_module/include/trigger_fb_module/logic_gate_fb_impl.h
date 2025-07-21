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

static const char TRIGGER_FB_MODULE_LOGIC_GATE_STR[] = "HbkLogicGateFb";

static const char LOGIC_GATE_TYPE_STR[] = "Type";

static const char LOGIC_GATE_AND_STR[] = "AND";
static const char LOGIC_GATE_OR_STR[] = "OR";
static const char LOGIC_GATE_XOR_STR[] = "XOR";

struct LogicGateContext
{
    size_t index;
    InputPortConfigPtr inputPort;
    bool active;
    bool connected;
};

enum LogicGateType: int {
    LOGIC_GATE_TYPE_AND = 0,
    LOGIC_GATE_TYPE_OR = 1,
    LOGIC_GATE_TYPE_XOR = 2,
};

class LogicGateFbImpl final : public FunctionBlock
{
public:
    explicit LogicGateFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~LogicGateFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;

private:

    LogicGateType m_logicGateType;
    size_t m_portCount;

    bool m_logicState;

    std::vector<LogicGateContext> m_logicGateContexts;

    DataDescriptorPtr m_outputDataDescriptor;
    DataDescriptorPtr m_outputDomainDataDescriptor;

    SignalConfigPtr m_outputSignal;
    SignalConfigPtr m_outputDomainSignal;

    bool state;
    PacketReadyNotification packetReadyNotification;

    void updateInputPorts();
    void createSignals();

    void processDataPacket(const DataPacketPtr& packet, LogicGateContext& context);

    void processEventPacket(const EventPacketPtr& packet);
    void onPacketReceived(const InputPortPtr& port) override;

    void processSignalDescriptorChanged();

    void configure();

    void initProperties();
    void propertyChanged();
    void readProperties();
    void checkSignalForTriggerInputValid(const DataDescriptorPtr& dataDescriptor);

};

END_NAMESPACE_TRIGGER_FB_MODULE