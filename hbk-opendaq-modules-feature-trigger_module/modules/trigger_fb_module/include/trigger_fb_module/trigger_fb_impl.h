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

static const char TRIGGER_FB_MODULE_TRIGGER_STR[] = "HbkTriggerFb";

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

static const char TRIGGER_SETTINGS_STR[] = "Settings";
static const char ENABLE_SETTINGS_HOLD_OFF_TIME_STR[] = "HoldOffTime";
static const char DISABLE_MAX_WAITING_TIME_STR[] = "MaxWaitingTime";



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

enum TriggerPortType: bool {
    ENABLE_PORT = 0,
    DISABLE_PORT = 1,
};
public:
    explicit TriggerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~TriggerFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

private:
    InputPortPtr m_inputEnablePort;
    InputPortPtr m_inputDisablePort;

    DataDescriptorPtr m_inputEnableDataDescriptor;
    DataDescriptorPtr m_inputEnableDomainDataDescriptor;

    DataDescriptorPtr m_inputDisableDataDescriptor;
    DataDescriptorPtr m_inputDisableDomainDataDescriptor;

    DataDescriptorPtr m_outputDataDescriptor;
    DataDescriptorPtr m_outputDomainDataDescriptor;

    SampleType m_enableInputSampleType;
    SampleType m_disableInputSampleType;


    SignalConfigPtr m_outputSignal;
    SignalConfigPtr m_outputDomainSignal;

    TriggerLogic m_enableLogicType;
    TriggerLogic m_disableLogicType;

    Float m_enableThreshold;
    bool m_enableGreaterEqual;
    Float m_disableThreshold;
    bool m_disableGreaterEqual;

    Float m_enableEdgeThreshold;
    bool m_enableRising;
    Float m_disableEdgeThreshold;
    bool m_disableRising;
    Float m_enableEdgeLastValue;
    Float m_disableEdgeLastValue;

    int64_t m_triggerDurationElapsed;
    int64_t m_maxWaitingTimeElapsed;
    
    int64_t m_disableDuration;
    int64_t m_maxWaitingTime;

    bool m_state; // True mean trigger signal = 1
    PacketReadyNotification packetReadyNotification;

    void createInputPorts();
    void createSignals();

    void trigger(const DataPacketPtr& inputPacket, size_t triggerIndex);

    template <SampleType InputSampleType>
    void processEnableDataPacket(const DataPacketPtr& packet);

    template <SampleType InputSampleType>
    void processDisableDataPacket(const DataPacketPtr& packet);

    void processEventPacketEnable(const EventPacketPtr& packet);
    void processEventPacketDisable(const EventPacketPtr& packet);
    void onPacketReceived(const InputPortPtr& port) override;

    void configure(const DataDescriptorPtr& inputDataDescriptor, const DataDescriptorPtr& inputDomainDataDescriptor, const TriggerPortType triggerPortType);

    void addTypeDefinition();
    void initProperties(const daq::PropertyObjectPtr& config);
    void propertyChanged();
    void readProperties();

    bool evalLevelTrigger(const Float& value, const bool& greateEqual, const Float& threshold );
    bool evalEdgeTrigger(const Float& lastValue, const Float& value, const bool& rising, const Float& threshold);

};
 
 END_NAMESPACE_TRIGGER_FB_MODULE