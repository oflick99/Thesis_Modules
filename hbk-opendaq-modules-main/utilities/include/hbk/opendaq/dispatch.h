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

 #include <opendaq/sample_type.h>
 
 #define SAMPLE_TYPE_DISPATCH(ST, Func, ...)                   \
    switch (ST)                                                \
    {                                                          \
        case daq::SampleType::Int8:                            \
            Func<daq::SampleType::Int8>(__VA_ARGS__);          \
            break;                                             \
        case daq::SampleType::Int16:                           \
            Func<daq::SampleType::Int16>(__VA_ARGS__);         \
            break;                                             \
        case daq::SampleType::Int32:                           \
            Func<daq::SampleType::Int32>(__VA_ARGS__);         \
            break;                                             \
        case daq::SampleType::Int64:                           \
            Func<daq::SampleType::Int64>(__VA_ARGS__);         \
            break;                                             \
        case daq::SampleType::UInt8:                           \
            Func<daq::SampleType::UInt8>(__VA_ARGS__);         \
            break;                                             \
        case daq::SampleType::UInt16:                          \
            Func<daq::SampleType::UInt16>(__VA_ARGS__);        \
            break;                                             \
        case daq::SampleType::UInt32:                          \
            Func<daq::SampleType::UInt32>(__VA_ARGS__);        \
            break;                                             \
        case daq::SampleType::UInt64:                          \
            Func<daq::SampleType::UInt64>(__VA_ARGS__);        \
            break;                                             \
        case daq::SampleType::Float32:                         \
            Func<daq::SampleType::Float32>(__VA_ARGS__);       \
            break;                                             \
        case daq::SampleType::Float64:                         \
            Func<daq::SampleType::Float64>(__VA_ARGS__);       \
            break;                                             \
        default:                                               \
            assert(false);                                     \
    }
 