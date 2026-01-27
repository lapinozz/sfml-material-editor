/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once



class FFormatterStyle
{
public:
    static void Initialize();
    static void Shutdown();
    static std::shared_ptr<class ISlateStyle> Get();
private:
    static std::shared_ptr<class FSlateStyleSet> StyleSet;
};
