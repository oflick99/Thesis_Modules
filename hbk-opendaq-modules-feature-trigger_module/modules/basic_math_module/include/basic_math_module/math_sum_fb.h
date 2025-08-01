/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* vim: set ts=4 et sw=4 tw=80: */
/*
 * Copyright (C) 2020 HBK – Hottinger Brüel & Kjær
 * Skodsborgvej 307
 * DK-2850 Nærum
 * Denmark
 * http://www.hbkworld.com
 * All rights reserved
 */

#pragma once
#include "basic_math_module/common.h"
#include <opendaq/function_block_impl.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/multi_reader_ptr.h>
#include <opendaq/signal_config_ptr.h>

BEGIN_NAMESPACE_BASIC_MATH_MODULE

// Identifier string for the Math Sum function block
static const char BASIC_MATH_MODULE_MATH_SUM_STR[] = "RefFBModuleMathSum";

/**
 * @brief Function block for summing multiple input signals.
 *
 * This block sums all input signals. The number of inputs is configurable
 * via the InputCount property.
 */
class MathSumFbImpl final : public FunctionBlock
{
public:
    /**
     * @brief Constructs the Math Sum function block.
     */
    explicit MathSumFbImpl(const ContextPtr &ctx, const ComponentPtr &parent, const StringPtr &localId, const PropertyObjectPtr &config);
    ~MathSumFbImpl() override = default;

    /**
     * @brief Creates the function block type descriptor.
     */
    static FunctionBlockTypePtr CreateType();

    /**
     * @brief Checks if a data descriptor is not null.
     */
    static bool descriptorNotNull(const DataDescriptorPtr &descriptor);

    /**
     * @brief Extracts value and domain descriptors from an event packet.
     */
    static void getDataDescriptors(const EventPacketPtr &eventPacket, DataDescriptorPtr &valueDesc, DataDescriptorPtr &domainDesc);

    /**
     * @brief Extracts the value descriptor from an event packet.
     */
    static bool getDataDescriptor(const EventPacketPtr &eventPacket, DataDescriptorPtr &valueDesc);

    /**
     * @brief Extracts the domain descriptor from an event packet.
     */
    static bool getDomainDescriptor(const EventPacketPtr &eventPacket, DataDescriptorPtr &domainDesc);

private:
    // Input ports
    std::vector<InputPortPtr> inputPorts;
    size_t inputCount;

    // Data descriptors
    std::vector<DataDescriptorPtr> inputDescriptors;
    DataDescriptorPtr domainDescriptor;
    DataDescriptorPtr sumDataDescriptor;
    DataDescriptorPtr sumDomainDataDescriptor;

    // Output signals
    SignalConfigPtr sumSignal;
    SignalConfigPtr sumDomainSignal;

    // Configuration properties
    Float sumHighValue;
    Float sumLowValue;
    Bool useCustomOutputRange;
    std::chrono::milliseconds tickOffsetToleranceUs;

    // Reader for synchronized input data
    MultiReaderPtr reader;

    /**
     * @brief Initializes the input ports based on InputCount.
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
     */
    RangePtr getValueRange() const;

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
     */
    void onConnected(const InputPortPtr &inputPort) override;

    /**
     * @brief Called when an input port is disconnected.
     */
    void onDisconnected(const InputPortPtr &inputPort) override;

    /**
     * @brief Configures the function block based on input descriptors.
     */
    void configure(const DataDescriptorPtr &domainDescriptor, const std::vector<DataDescriptorPtr> &inputDescriptors);

    /**
     * @brief Initializes the configuration properties.
     */
    void initProperties();

    /**
     * @brief Called when a property changes.
     */
    void propertyChanged(bool configure);

    /**
     * @brief Reads and applies the current property values.
     */
    void readProperties();

    /**
     * @brief Updates input ports when InputCount changes.
     */
    void updateInputPorts();
};

END_NAMESPACE_BASIC_MATH_MODULE