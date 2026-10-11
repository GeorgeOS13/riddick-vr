$allExports = Get-Content "C:\Users\GigelCastel\riddick-vr\all_exports.txt"

$overridden = @(
    "wglCreateContext",
    "wglDeleteContext",
    "wglMakeCurrent",
    "wglSwapBuffers",
    "wglGetProcAddress",
    "glViewport",
    "glScissor"
)

# 1. Generate opengl32.def
$defContent = "LIBRARY opengl32.dll`r`nEXPORTS`r`n"
foreach ($name in $allExports) {
    $defContent += "    $name`r`n"
}
$defPath = "C:\Users\GigelCastel\riddick-vr\src\hook\opengl32.def"
[System.IO.File]::WriteAllText($defPath, $defContent)
Write-Host "Generated opengl32.def with $($allExports.Count) exports."

# 2. Generate opengl_forwarders.c
$cHeader = @"
typedef void* HMODULE;
typedef void* FARPROC;
__declspec(dllimport) FARPROC __stdcall GetProcAddress(HMODULE hModule, const char* lpProcName);
#define NULL ((void*)0)

#define DEFINE_FORWARDER(func) \
    static void* pReal_##func = NULL; \
    __declspec(naked) void __stdcall func(void) { \
        __asm { jmp dword ptr [pReal_##func] } \
    }

#define LOAD_FORWARDER(func) \
    pReal_##func = (void*)GetProcAddress(hRealDll, #func)
"@

$defines = @()
$loads = @()

foreach ($name in $allExports) {
    if ($overridden -notcontains $name) {
        $defines += "DEFINE_FORWARDER($name)"
        $loads += "    LOAD_FORWARDER($name);"
    }
}

$cBody = ($defines -join "`r`n") + "`r`n`r`nvoid InitializeForwarders(HMODULE hRealDll) {`r`n    if (!hRealDll) return;`r`n`r`n" + ($loads -join "`r`n") + "`r`n}`r`n"

$cContent = $cHeader + "`r`n" + $cBody
$cPath = "C:\Users\GigelCastel\riddick-vr\src\hook\opengl_forwarders.c"
[System.IO.File]::WriteAllText($cPath, $cContent)
Write-Host "Generated opengl_forwarders.c with $($defines.Count) forwarders."
