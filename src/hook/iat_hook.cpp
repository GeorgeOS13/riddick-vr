#include "iat_hook.h"
#include "logging.h"
#include <string>

namespace RiddickVR {

bool IATHook::Hook(HMODULE hModule, const char* targetDll, const char* targetFunc, void* newFunc, void** origFunc) {
    if (!hModule || !targetDll || !targetFunc || !newFunc) return false;

    auto pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

    auto pNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<BYTE*>(hModule) + pDosHeader->e_lfanew
    );
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) return false;

    auto& importDir = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.VirtualAddress == 0 || importDir.Size == 0) return false;

    auto pImportDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        reinterpret_cast<BYTE*>(hModule) + importDir.VirtualAddress
    );

    for (; pImportDesc->Name != 0; ++pImportDesc) {
        const char* modName = reinterpret_cast<const char*>(
            reinterpret_cast<BYTE*>(hModule) + pImportDesc->Name
        );
        if (_stricmp(modName, targetDll) != 0) continue;

        auto pOriginalThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
            reinterpret_cast<BYTE*>(hModule) + pImportDesc->OriginalFirstThunk
        );
        auto pFirstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
            reinterpret_cast<BYTE*>(hModule) + pImportDesc->FirstThunk
        );

        for (; pOriginalThunk->u1.Function != 0; ++pOriginalThunk, ++pFirstThunk) {
            if (IMAGE_SNAP_BY_ORDINAL(pOriginalThunk->u1.Ordinal)) continue;

            auto pImportByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                reinterpret_cast<BYTE*>(hModule) + pOriginalThunk->u1.AddressOfData
            );

            if (strcmp(reinterpret_cast<const char*>(pImportByName->Name), targetFunc) == 0) {
                DWORD oldProtect = 0;
                if (!VirtualProtect(&pFirstThunk->u1.Function, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    VR_LOG_ERROR("VirtualProtect failed when hooking IAT %s!%s", targetDll, targetFunc);
                    return false;
                }

                if (origFunc && !*origFunc) {
                    *origFunc = reinterpret_cast<void*>(pFirstThunk->u1.Function);
                }
                pFirstThunk->u1.Function = reinterpret_cast<uintptr_t>(newFunc);

                VirtualProtect(&pFirstThunk->u1.Function, sizeof(void*), oldProtect, &oldProtect);
                VR_LOG_INFO("Successfully IAT-hooked %s!%s in module %p", targetDll, targetFunc, hModule);
                return true;
            }
        }
    }

    return false;
}

} // namespace RiddickVR
