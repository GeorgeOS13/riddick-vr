#pragma once

#include <windows.h>

namespace RiddickVR {

class IATHook {
public:
    static bool Hook(HMODULE hModule, const char* targetDll, const char* targetFunc, void* newFunc, void** origFunc);
};

} // namespace RiddickVR
