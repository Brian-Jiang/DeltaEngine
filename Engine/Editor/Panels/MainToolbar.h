#pragma once

#include "EngineIncludes.h"

#include "UIComponents/HorizontalToggleGroup.h"

DELTA_ENGINE_NS_BEGIN

class MainToolbar
{
public:
    // Transform/play controls and viewport dropdowns below the app header.
    void Draw();

private:
    HorizontalToggleGroup m_transformGroup;
    HorizontalToggleGroup m_playGroup;

    int m_transformMode = 1;   // 0=Select 1=Move 2=Rotate 3=Scale
    int m_playState = 0;      // 0=Stopped 1=Playing 2=Paused
    int m_coordSpace = 0;     // 0=World 1=Local
    int m_pivotMode = 0;      // 0=Pivot 1=Center
    int m_projMode = 0;       // 0=Perspective 1=Orthographic
    int m_shadingMode = 0;    // 0=Lit 1=Unlit 2=Wireframe
    float m_snapValue = 0.25f;
    float m_fps = 0.f;
};

DELTA_ENGINE_NS_END
