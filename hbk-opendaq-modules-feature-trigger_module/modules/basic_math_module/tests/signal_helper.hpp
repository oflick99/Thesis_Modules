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

#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>

using namespace daq;

template <typename T>
using vecvec = std::vector<std::vector<T> >;

template <typename T>
class SignalHelper
{
public:
    SignalHelper( const ContextPtr& context,
                  const DataRulePtr& rule,
                  const SampleType& sampleType,
                  const vecvec<T>& mockPackets,
                  const vecvec<Int>& mockDomainPackets = {})
                  
    {
        m_context = context;
        m_rule = rule;
        m_sampleType = sampleType;
        m_mockPackets = mockPackets;
        m_mockDomainPackets = mockDomainPackets;
        createDomainSignal();
        createDomainPackets();
        createSignal();
    }

    void sendPackets()
    {
        // For each packet
        for (size_t i = 0; i < m_mockPackets.size(); i++)
        {
            sendPacket(i);
        }
    }

    void sendPacket(const size_t index)
    {
        if (index >= m_mockPackets.size())
            return;
        
        // Create data packet
        auto dataPacket = DataPacketWithDomain(m_domainPackets[index], m_signalDescriptor, m_mockPackets[index].size());
        auto packetData = static_cast<T*>(dataPacket.getRawData());
        for (size_t i = 0; i < m_mockPackets[index].size(); i++)
            *packetData++ = static_cast<T>(m_mockPackets[index][i]);

        // Send packet
        m_domainSignal.sendPacket(m_domainPackets[index]);
        m_signal.sendPacket(dataPacket);
        
    }

    SignalConfigPtr getSignal()
    {
        return m_signal;
    }

    SignalConfigPtr getDomainSignal()
    {
        return m_domainSignal;
    }


private:

    void createDomainSignal()
    {
        // Create domain signal with descriptor
        auto domainSignalDescriptorBuilder = DataDescriptorBuilder();
        domainSignalDescriptorBuilder.setUnit(Unit("s", -1, "seconds", "time"));
        domainSignalDescriptorBuilder.setSampleType(SampleType::Int64);
        domainSignalDescriptorBuilder.setRule(m_rule);
        domainSignalDescriptorBuilder.setOrigin("1970");
        domainSignalDescriptorBuilder.setTickResolution(Ratio(1, 1000));
        m_domainSignalDescriptor = domainSignalDescriptorBuilder.build();
        m_domainSignal = SignalWithDescriptor(m_context, m_domainSignalDescriptor, nullptr, "domain_signal");
    }

    void createDomainPackets()
    {
        // Create domain packets
        if (m_rule.getType() == DataRuleType::Explicit)
        {
            for (size_t i = 0; i < m_mockPackets.size(); i++)
            {
                // Explicit creation of one domain packet
                auto domainPacket = DataPacket(m_domainSignalDescriptor, m_mockPackets[i].size());
                auto domainPacketData = static_cast<Int*>(domainPacket.getRawData());
                for (size_t ii = 0; ii < m_mockDomainPackets[i].size(); ii++)
                    *domainPacketData++ = static_cast<Int>(m_mockDomainPackets[i][ii]);
                m_domainPackets.push_back(domainPacket);
            }
        }
        else
        {
            Int delta = m_rule.getParameters().get("delta");
            for (size_t i = 0; i < m_mockPackets.size(); i++)
            {
                // Linear creation of one domain packet
                auto offset = 0;
                for (size_t ii = 0; ii < i; ii++)
                {
                    offset += m_mockPackets[ii].size() * delta;
                }

                auto domainPacket = DataPacket(m_domainSignalDescriptor, m_mockPackets[i].size(), offset);
                m_domainPackets.push_back(domainPacket);
            }
        }
    }

    void createSignal()
    {
        // Create signal with descriptor
        auto signalDescriptorBuilder = DataDescriptorBuilder();
        signalDescriptorBuilder.setSampleType(m_sampleType);
        signalDescriptorBuilder.setValueRange(Range(0, 300));
        signalDescriptorBuilder.setRule(ExplicitDataRule());
        m_signalDescriptor = signalDescriptorBuilder.build();
        m_signal = SignalWithDescriptor(m_context, m_signalDescriptor, nullptr, "signal");

        // Set domain signal of signal
        m_signal.setDomainSignal(m_domainSignal);
    }

    DataRulePtr m_rule;
    SampleType m_sampleType;
    vecvec<T> m_mockPackets;
    vecvec<Int> m_mockDomainPackets;
    ContextPtr m_context;

    DataDescriptorPtr m_domainSignalDescriptor;
    SignalConfigPtr m_domainSignal;
    std::vector<DataPacketPtr> m_domainPackets;
    DataDescriptorPtr m_signalDescriptor;
    SignalConfigPtr m_signal;
};

template <typename T>
void checkReceivedPacketEqualExpected(const daq::DataPacketPtr& receivedPacket, std::vector<T> expectedValuePacket)
{
    // Check packet content
    auto data = static_cast<T*>(receivedPacket.getData());
    const size_t sampleCount = receivedPacket.getSampleCount();

    ASSERT_EQ(sampleCount, expectedValuePacket.size());

    for (size_t i = 0; i < sampleCount; ++i)
    {
        ASSERT_FLOAT_EQ(data[i], expectedValuePacket[i]);
    }
    
}

void checkReceivedTriggerPacketEqualExpected(const daq::DataPacketPtr& receivedPacket, uint8_t active, int64_t domainCount)
{
    // Check packet content
    auto data = static_cast<uint8_t*>(receivedPacket.getData());
    const size_t sampleCount = receivedPacket.getSampleCount();

    auto domainData = static_cast<Int*>(receivedPacket.getDomainPacket().getData());
    const size_t domainSampleCount = receivedPacket.getDomainPacket().getSampleCount();

    // Assert that packet has one sample
    ASSERT_EQ(sampleCount, 1);
    // Assert that domain packet has one sample
    ASSERT_EQ(domainSampleCount, 1);

    // Assert that first sample equals expected value
    ASSERT_EQ(data[0], active);
    ASSERT_EQ(domainData[0], domainCount);

}