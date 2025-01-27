#pragma once
#include <vector>
#include <string>
#include "DirectUI/DirectUI.h"

using namespace std;

struct parameters {
    DirectUI::Element* elem{};
    int x{};
    int y{};
};

extern int shortIndex;
extern vector<parameters> shortpm;
extern vector<wstring> listDirBuffer;

vector<wstring> list_directory();
