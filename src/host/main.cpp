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
    bool autoExitWithGame = false;
    bool mockMode = false;
    std::string dumpDir = "";
    uint32_t maxFrames = 0;
    uint32_t dumpInterval = 10;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--validate-only") {
            validateOnly = true;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--auto-exit-with-game") {
            autoExitWithGame = true;
        } else if (arg == "--mock") {
            mockMode = true;
        } else if (arg == "--dump-dir" && i + 1 < argc) {
            dumpDir = argv[++i];
        } else if (arg == "--max-frames" && i + 1 < argc) {
            maxFrames = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--dump-interval" && i + 1 < argc) {
            dumpInterval = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: riddickvr-host [OPTIONS]\n"
                      << "Options:\n"
                      << "  --validate-only         Verify OpenXR runtime and exit without running VR loop\n"
                      << "  --auto-exit-with-game   Exit automatically when game process disconnects\n"
                      << "  --mock                  Run in headless simulated VR mock mode (no headset needed)\n"
                      << "  --dump-dir <PATH>       Directory to save test frame screenshots (BMP format)\n"
                      << "  --dump-interval <N>     Save screenshot every N frames (default: 10)\n"
                      << "  --max-frames <N>        Exit automatically after N rendered frames\n"
                      << "  --verbose               Enable verbose debug logging\n"
                      << "  --help, -h              Show this help message\n";
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

    if (mockMode) {
        xrManager.SetMockMode(true, dumpDir, maxFrames, dumpInterval);
    }

    if (validateOnly) {
        std::string report;
        bool valid = xrManager.Validate(report);
        std::cout << report << std::endl;
        xrManager.Shutdown();
        ipcHost.Shutdown();
        return valid ? 0 : 1;
    }

    if (!xrManager.Initialize()) {
        std::cerr << "[Host] ERROR: Failed to initialize OpenXR Manager!\n";
        ipcHost.Shutdown();
        return 1;
    }

    // Run VR loop
    xrManager.RunLoop(ipcHost, autoExitWithGame);

    // Cleanup
    xrManager.Shutdown();
    ipcHost.Shutdown();
    std::cout << "[Host] Exited cleanly.\n";
    return 0;
}
