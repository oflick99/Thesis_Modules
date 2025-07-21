/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* vim: set ts=4 et sw=4 tw=80: */
/*
 * Copyright (C) 2024 HBK – Hottinger Brüel & Kjær
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

#include <gmock/gmock.h>
#include <testutils/testutils.h>

#include <fstream>
#include <filesystem>
#include <cstdlib>

#include <camera_device_module/module_dll.h>
#include <camera_device_module/common.h>

#include <coretypes/common.h>
#include <opendaq/opendaq.h>
#include <opendaq/module_ptr.h>
#include <opendaq/device_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>

namespace lanfactory
{

static daq::ModulePtr CreateModule()
{
    daq::ModulePtr module;
    createModule(&module, daq::NullContext());
    return module;
}

static daq::ModulePtr CreateModule(const daq::ContextPtr& context)
{
    daq::ModulePtr module;
    createModule(&module, context);
    return module;
}

static daq::ContextPtr CreateContext()
{
    const auto logger = daq::Logger();
    auto typeManger = daq::TypeManager();
    auto moduleManager = daq::ModuleManager("[None]");
    return daq::Context(daq::Scheduler(logger), logger, typeManger , moduleManager, nullptr);
}

}