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
#include "static_fatigue_module/common.h"
#include <opendaq/function_block_impl.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/multi_reader_ptr.h>
#include <opendaq/signal_config_ptr.h>

BEGIN_NAMESPACE_STATIC_FATIGUE_MODULE

static const char STATIC_FATIGUE_MODULE_SG_SUBTRACTION_STR[] = "HbkSgSubtractionFb";

class SgSubtractionFbImpl final : public FunctionBlock
{
public:
    explicit SgSubtractionFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~SgSubtractionFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

    static bool descriptorNotNull(const DataDescriptorPtr& descriptor);
    static void getDataDescriptors(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc, DataDescriptorPtr& domainDesc);
    static bool getDataDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc);
    static bool getDomainDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& domainDesc);
    
private:
    // Input ports
    InputPortPtr measuringDmsInputPort;
    InputPortPtr dummyDmsInputPort;

    // Data descriptors
    DataDescriptorPtr measuringDmsDescriptor;
    DataDescriptorPtr dummyDmsDescriptor;
    DataDescriptorPtr domainDescriptor;

    DataDescriptorPtr compensatedDataDescriptor;
    DataDescriptorPtr compensatedDomainDataDescriptor;

    // Output signals
    SignalConfigPtr compensatedSignal;
    SignalConfigPtr compensatedDomainSignal;

    // Properties
    Float measuringDmsScale;
    Float measuringDmsOffset;
    Float dummyDmsScale;
    Float dummyDmsOffset;
    Float compensatedHighValue;
    Float compensatedLowValue;
    Bool useCustomOutputRange;
    std::chrono::milliseconds tickOffsetToleranceUs;

    MultiReaderPtr reader;

    // Methods
    void createInputPorts();
    void createReader();
    void createSignals();
    static RangePtr getValueRange(const DataDescriptorPtr& measuringDescriptor, const DataDescriptorPtr& dummyDescriptor);
    void onDataReceived();

    void checkPortConnections() const;
    void onConnected(const InputPortPtr& inputPort) override;
    void onDisconnected(const InputPortPtr& inputPort) override;

    void configure(const DataDescriptorPtr& domainDescriptor,
                   const DataDescriptorPtr& measuringDescriptor,
                   const DataDescriptorPtr& dummyDescriptor);

    void initProperties();
    void propertyChanged(bool configure);
    void readProperties();
};

END_NAMESPACE_STATIC_FATIGUE_MODULE