#pragma once
#include "framework.h"
#include "DirectUI/DirectUI.h"

using namespace DirectUI;

extern bool showcheckboxes;
extern bool treatdirasgroup;
extern bool isColorized;

extern Element* UIContainer;
extern Edit* chooseColor;

bool GetWindowsBuild(int build);

void ToggleCheckbox(Element* elem, Event* iev);
void ToggleShowHidden(Element* elem, Event* iev);
void ToggleFilenameExts(Element* elem, Event* iev);
void ToggleGroupMode(Element* elem, Event* iev);
void ToggleAccentIcons(Element* elem, Event* iev);
void ApplySelectedColor(Element* elem, Event* iev);