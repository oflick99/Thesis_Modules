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

 #include <coretypes/common.h>
 #include <iostream>
 #include <string>
 
 #define BEGIN_NAMESPACE_STATIC_FATIGUE_MODULE BEGIN_NAMESPACE_OPENDAQ_MODULE(static_fatigue_module)
 
 static const std::string STATIC_FATIGUE_MODULE_NAME = "StaticFatigueModule";
 static const std::string SG_SUBTRACTION_FB_NAME = "SgSubtractionFb";

 #define END_NAMESPACE_STATIC_FATIGUE_MODULE END_NAMESPACE_OPENDAQ_MODULE