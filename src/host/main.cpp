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
    std::cout << "========================================" << std::endl;
    std::cout << "  Riddick VR - OpenXR Companion Host   " << std::endl;
    std::cout << "========================================" << std::endl;

    SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

    // Initialize IPC Bridge Host
    RiddickVR::IpcBridgeHost ipcHost;
    if (!ipcHost.Initialize()) {
        std::cerr << "[Host] ERROR: Failed to create IPC shared memory mapping!" << std::endl;
        return 1;
    }
    std::cout << "[Host] Shared memory IPC bridge initialized successfully." << std::endl;

    // Initialize OpenXR Manager
    RiddickVR::OpenXRManager xrManager;
    g_pManager = &xrManager;

    if (!xrManager.Initialize()) {
        std::cerr << "[Host] ERROR: Failed to initialize OpenXR Manager!" << std::endl;
        ipcHost.Shutdown();
        return 1;
    }

    // Run VR loop
    xrManager.RunLoop(ipcHost);

    // Cleanup
    xrManager.Shutdown();
    ipcHost.Shutdown();
    std::cout << "[Host] Exited cleanly." << std::endl;
    return 0;
}
