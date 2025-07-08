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

static const char TRIGGER_FB_MODULE_SCALING_STR[] = "HbkTriggerFb";

static const char LEVEL_ENABLE_ENABLED_STR[] = "LevelEnableEnabled";
static const char DIG_IO_ENABLE_ENABLED_STR[] = "DigIOEnableEnabled";
static const char EDGE_ENABLE_ENABLED_STR[] = "EdgeEnableEnabled";
static const char DATE_ENABLE_ENABLED_STR[] = "DateEnableEnabled";
static const char TIME_OFF_DAY_ENABLE_ENABLED_STR[] = "TimeOffDayEnableEnabled";

static const char ENABLE_STR[] = "Enable";
static const char DISABLE_STR[] = "Disable";

static const char LOGIC_TYPE_STR[] = "LogicType";

static const char LOGIC_LEVEL_STR[] = "Level";
static const char LOGIC_LEVEL_THRESHOLD_STR[] = "Threshold";
static const char LOGIC_LEVEL_LOGIC_STR[] = "Logic";

static const char LOGIC_DIGIO_STR[] = "DigitalIO";
static const char LOGIC_DIGIO_NEGATED_STR[] = "Negated";

static const char LOGIC_EDGE_STR[] = "Edge";
static const char LOGIC_EDGE_THRESHOLD_STR[] = "Threshold";
static const char LOGIC_EDGE_LOGIC_STR[] = "Logic";

static const char LOGIC_DATE_STR[] = "Date";

static const char LOGIC_TIME_OF_DATE_STR[] = "TimeOfDay";

static const char LOGIC_DURATION_STR[] = "Duration";

static const char SETTINGS_STR[] = "Settings";
static const char ENABLE_SETTINGS_HOLD_OFF_TIME_STR[] = "HoldOffTime";

static const char DISABLE_MAX_WAITING_TIME_STR[] = "MaxWaitingTime";
static const char DISABLE_RE_TRIGGER_STR[] = "ReTrigger";



enum TriggerLogic: int {
    TRIGGER_LOGIC_DURATION = 0,
    TRIGGER_LOGIC_LEVEL = 1,
    TRIGGER_LOGIC_DIGIO = 2,
    TRIGGER_LOGIC_EDGE = 3,
    TRIGGER_LOGIC_DATE = 4,
    TRIGGER_LOGIC_TIME_OF_DAY = 5,
};

enum TriggerState: bool {
    TRIGGER_INACTIVE = 0,
    TRIGGER_ACTIVE = 1,
};
 
class TriggerFbImpl final : public FunctionBlock
{
public:
    explicit TriggerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~TriggerFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

private:
    InputPortPtr m_inputEnablePort;
    InputPortPtr m_inputDisablePort;


    DataDescriptorPtr inputDataDescriptor;
    DataDescriptorPtr inputDomainDataDescriptor;

    DataDescriptorPtr outputDataDescriptor;
    DataDescriptorPtr outputDomainDataDescriptor;

    SampleType inputSampleType;

    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    Float threshold;

    bool m_state; // True mean trigger signal = 1
    PacketReadyNotification packetReadyNotification;

    void createInputPorts();
    void createSignals();

    void trigger(const DataPacketPtr& inputPacket, size_t triggerIndex);

    template <SampleType InputSampleType>
    void processEnableDataPacket(const DataPacketPtr& packet);

    template <SampleType InputSampleType>
    void processDisableDataPacket(const DataPacketPtr& packet);

    void processEventPacket(const EventPacketPtr& packet);
    void onPacketReceived(const InputPortPtr& port) override;

    void processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor, const DataDescriptorPtr& inputDomainDataDescriptor);

    void configure();

    void addTypeDefinition();
    void initProperties(const daq::PropertyObjectPtr& config);
    void propertyChanged();
    void readProperties();
};
 
 END_NAMESPACE_TRIGGER_FB_MODULE