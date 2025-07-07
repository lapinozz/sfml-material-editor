/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once


#include "Framework/Commands/Commands.h"
#include "FormatterStyle.h"

class FFormatterCommands : public TCommands<FFormatterCommands>
{
public:
    FFormatterCommands() :
        TCommands<FFormatterCommands>("GraphFormatterEditor", NSLOCTEXT("Contexts", "GraphFormatterEditor", "Grap Formatter Editor"), NAME_None, FFormatterStyle::Get()->GetStyleSetName())
    {
    }

    std::shared_ptr<FUICommandInfo> FormatGraph;
    std::shared_ptr<FUICommandInfo> StraightenConnections;
    std::shared_ptr<FUICommandInfo> PlaceBlock;
    virtual void RegisterCommands() override;
};
