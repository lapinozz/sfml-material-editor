/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "FormatterSettings.h"

UFormatterSettings::UFormatterSettings()
    : PositioningAlgorithm(EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodTop)
    , HorizontalSpacing(100)
    , VerticalSpacing(80)
    , MaxLayerNodes(5)
    , MaxOrderingIterations(10)
{
}

UFormatterSettings& UFormatterSettings::Get()
{
    static UFormatterSettings settings{};
    return settings;
}
