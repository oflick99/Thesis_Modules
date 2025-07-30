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

// Identifier string for the SG subtraction function block
static const char STATIC_FATIGUE_MODULE_SG_SUBTRACTION_STR[] = "RefFBModuleSgSubtraction";

/**
 * @brief Function block for strain gauge temperature compensation using a dummy-SG.
 *
 * This block subtracts the signal of a dummy strain gauge
 * from a measuring strain gauge signal. The result is a compensated output signal.
 */

class SgSubtractionFbImpl final : public FunctionBlock
{
public:
    /**
     * @brief Constructs the SG subtraction function block.
     *
     * @param ctx The DAQ context.
     * @param parent The parent component.
     * @param localId The local identifier of the function block.
     * @param config Configuration properties for the function block.
     */
    explicit SgSubtractionFbImpl(const ContextPtr &ctx, const ComponentPtr &parent, const StringPtr &localId, const PropertyObjectPtr &config);
    ~SgSubtractionFbImpl() override = default;

    /**
     * @brief Creates the function block type descriptor.
     *
     * @return A shared pointer to the function block type.
     */
    static FunctionBlockTypePtr CreateType();

    /**
     * @brief Checks if a data descriptor is not null.
     *
     * @param descriptor The data descriptor to check.
     * @return True if the descriptor is valid, false otherwise.
     */
    static bool descriptorNotNull(const DataDescriptorPtr &descriptor);

    /**
     * @brief Extracts value and domain descriptors from an event packet.
     *
     * @param eventPacket The event packet.
     * @param valueDesc Output: The value descriptor.
     * @param domainDesc Output: The domain descriptor.
     */
    static void getDataDescriptors(const EventPacketPtr &eventPacket, DataDescriptorPtr &valueDesc, DataDescriptorPtr &domainDesc);

    /**
     * @brief Extracts the value descriptor from an event packet.
     *
     * @param eventPacket The event packet.
     * @param valueDesc Output: The value descriptor.
     * @return True if successful, false otherwise.
     */
    static bool getDataDescriptor(const EventPacketPtr &eventPacket, DataDescriptorPtr &valueDesc);

    /**
     * @brief Extracts the domain descriptor from an event packet.
     *
     * @param eventPacket The event packet.
     * @param domainDesc Output: The domain descriptor.
     * @return True if successful, false otherwise.
     */
    static bool getDomainDescriptor(const EventPacketPtr &eventPacket, DataDescriptorPtr &domainDesc);

private:
    // Input ports for measuring and dummy strain gauges
    InputPortPtr measuringDmsInputPort;
    InputPortPtr dummyDmsInputPort;

    // Data descriptors for input and output signals
    DataDescriptorPtr measuringDmsDescriptor;
    DataDescriptorPtr dummyDmsDescriptor;
    DataDescriptorPtr domainDescriptor;

    DataDescriptorPtr compensatedDataDescriptor;
    DataDescriptorPtr compensatedDomainDataDescriptor;

    // Output signals for compensated values
    SignalConfigPtr compensatedSignal;
    SignalConfigPtr compensatedDomainSignal;

    // Configuration properties
    Float measuringDmsScale;
    Float measuringDmsOffset;
    Float dummyDmsScale;
    Float dummyDmsOffset;
    Float compensatedHighValue;
    Float compensatedLowValue;
    Bool useCustomOutputRange;
    std::chrono::milliseconds tickOffsetToleranceUs;

    // Reader for synchronized input data
    MultiReaderPtr reader;

    /**
     * @brief Initializes the input ports.
     */
    void createInputPorts();

    /**
     * @brief Initializes the multi-reader for synchronized data access.
     */
    void createReader();

    /**
     * @brief Initializes the output signals.
     */
    void createSignals();

    /**
     * @brief Computes the output value range based on input descriptors.
     *
     * @param measuringDescriptor Descriptor of the measuring SG.
     * @param dummyDescriptor Descriptor of the dummy SG.
     * @return A shared pointer to the resulting value range.
     */
    static RangePtr getValueRange(const DataDescriptorPtr &measuringDescriptor, const DataDescriptorPtr &dummyDescriptor);

    /**
     * @brief Callback for processing incoming data.
     */
    void onDataReceived();

    /**
     * @brief Validates that all required ports are connected.
     */
    void checkPortConnections() const;

    /**
     * @brief Called when an input port is connected.
     *
     * @param inputPort The connected input port.
     */
    void onConnected(const InputPortPtr &inputPort) override;

    /**
     * @brief Called when an input port is disconnected.
     *
     * @param inputPort The disconnected input port.
     */
    void onDisconnected(const InputPortPtr &inputPort) override;

    /**
     * @brief Configures the function block based on input descriptors.
     *
     * @param domainDescriptor The domain descriptor.
     * @param measuringDescriptor The measuring SG descriptor.
     * @param dummyDescriptor The dummy SG descriptor.
     */
    void configure(const DataDescriptorPtr &domainDescriptor,
                   const DataDescriptorPtr &measuringDescriptor,
                   const DataDescriptorPtr &dummyDescriptor);

    /**
     * @brief Initializes the configuration properties.
     */
    void initProperties();

    /**
     * @brief Called when a property changes.
     *
     * @param configure Whether to reconfigure the block.
     */
    void propertyChanged(bool configure);

    /**
     * @brief Reads and applies the current property values.
     */
    void readProperties();
};

END_NAMESPACE_STATIC_FATIGUE_MODULE