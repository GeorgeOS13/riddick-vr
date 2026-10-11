#include "openxr_manager.h"
#include "ipc_bridge.h"
#include <iostream>
#include <windows.h>

static RiddickVR::OpenXRManager* g_pManager = nullptr;

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        std::cout << "\n[Host] Termination signal received. Cleaning up..." << std::endl;
        if (g_pManager) {
            g_pManager->RequestExit();
        }
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char* argv[]) {
    bool validateOnly = false;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--validate-only") {
            validateOnly = true;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: riddickvr-host [OPTIONS]\n"
                      << "Options:\n"
                      << "  --validate-only    Verify OpenXR runtime and exit without running VR loop\n"
                      << "  --verbose          Enable verbose debug logging\n"
                      << "  --help, -h         Show this help message\n";
            return 0;
        }
    }

    std::cout << "========================================\n"
              << "   Riddick VR - x64 OpenXR Companion    \n"
              << "========================================\n";

    if (validateOnly) {
        std::cout << "[Host] Running in validation-only mode (--validate-only)...\n";
    }

    SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

    // Initialize IPC Bridge Host
    RiddickVR::IpcBridgeHost ipcHost;
    if (!ipcHost.Initialize()) {
        std::cerr << "[Host] ERROR: Failed to create IPC shared memory mapping!\n";
        return 1;
    }
    std::cout << "[Host] Shared memory IPC bridge initialized successfully.\n";

    // Initialize OpenXR Manager
    RiddickVR::OpenXRManager xrManager;
    g_pManager = &xrManager;

    if (!xrManager.Initialize()) {
        std::cerr << "[Host] ERROR: Failed to initialize OpenXR Manager!\n";
        ipcHost.Shutdown();
        return 1;
    }

    if (validateOnly) {
        std::cout << "[Host] OpenXR runtime validation SUCCESSFUL. Ready for VR.\n";
        xrManager.Shutdown();
        ipcHost.Shutdown();
        return 0;
    }

    // Run VR loop
    xrManager.RunLoop(ipcHost);

    // Cleanup
    xrManager.Shutdown();
    ipcHost.Shutdown();
    std::cout << "[Host] Exited cleanly.\n";
    return 0;
}
