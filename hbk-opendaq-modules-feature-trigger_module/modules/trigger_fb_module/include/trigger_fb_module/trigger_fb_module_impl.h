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
 #include <opendaq/module_impl.h>

 #include "trigger_fb_module/common.h"
 
 BEGIN_NAMESPACE_TRIGGER_FB_MODULE
 
 class TriggerFbModule final : public Module
 {
 public:
     explicit TriggerFbModule(ContextPtr ctx);
 
     DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
     FunctionBlockPtr onCreateFunctionBlock(const StringPtr& id, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config) override;
 private:
 };
 
 END_NAMESPACE_TRIGGER_FB_MODULE