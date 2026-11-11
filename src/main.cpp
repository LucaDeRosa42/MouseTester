// Mouse Tester - Copyright (C) 2026 lcrd42
// SPDX-License-Identifier: GPL-3.0-or-later
// Win32 window + DirectX 11 + Dear ImGui
#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>

#include <intrin.h>

#include <memory>

#include "analyzer.hpp"
#include "i18n.hpp"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "raw_input.hpp"
#include "resource.h"
#include "ui.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

ID3D11Device*           g_device = nullptr;
ID3D11DeviceContext*    g_context = nullptr;
IDXGISwapChain*         g_swapChain = nullptr;
ID3D11RenderTargetView* g_rtv = nullptr;
bool g_occluded = false;
UINT g_resizeW = 0, g_resizeH = 0;

void CreateRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv);
    backBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
}

bool CreateDeviceD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    // "Blt" model: measured, the "flip" model gave no benefit and with variable refresh rate
    // displays (windowed G-Sync) it dropped the screen to ~65 Hz while the mouse was idle.
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL got;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
                                               D3D11_SDK_VERSION, &sd, &g_swapChain, &g_device, &got, &g_context);
    if (hr == DXGI_ERROR_UNSUPPORTED) // no compatible GPU: software rendering
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2,
                                           D3D11_SDK_VERSION, &sd, &g_swapChain, &g_device, &got, &g_context);
    if (FAILED(hr)) return false;
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_swapChain) { g_swapChain->Release(); g_swapChain = nullptr; }
    if (g_context)   { g_context->Release();   g_context = nullptr; }
    if (g_device)    { g_device->Release();    g_device = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_resizeW = LOWORD(lParam);
        g_resizeH = HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0; // no ALT menu
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool FileExists(const char* path) { return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES; }

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    ImGui_ImplWin32_EnableDpiAwareness();
    const float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc{sizeof(wc)};
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // Large icon (Alt+Tab, taskbar) and small icon (title bar)
    wc.hIcon = static_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
                                             GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
    wc.hIconSm = static_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
                                               GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    wc.lpszClassName = L"MouseTesterWindow";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Mouse Tester", WS_OVERLAPPEDWINDOW, 100, 100,
                              int(1320 * scale), int(860 * scale), nullptr, nullptr, hInstance, nullptr);

    // Dark title bar (Windows 10 20H1+ / 11)
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        MessageBoxW(nullptr, L"Could not initialize DirectX 11.", L"Mouse Tester", MB_ICONERROR);
        return 1;
    }

    // Start capturing
    RawInputCapture capture;
    if (!capture.start()) {
        MessageBoxW(nullptr, L"Could not register mouse Raw Input.", L"Mouse Tester", MB_ICONERROR);
        return 1;
    }
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    auto analyzer = std::make_unique<Analyzer>(freq.QuadPart);

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    UiState ui;
    ui.scale = scale;
    LoadLanguage();

    // Only the characters actually used by the translations are put in the font atlas,
    // so Cyrillic and Chinese cost a few hundred glyphs instead of tens of thousands.
    static ImVector<ImWchar> glyphRanges; // must outlive the atlas build
    {
        ImFontGlyphRangesBuilder builder;
        builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
        builder.AddText("µ°·→–—“”„«»");
        AddTranslationGlyphs(builder);
        builder.BuildRanges(&glyphRanges);
    }
    const char* fontPath = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char* fontBoldPath = "C:\\Windows\\Fonts\\seguisb.ttf";
    const char* fontCjkPath = "C:\\Windows\\Fonts\\msyh.ttc"; // Microsoft YaHei, for Chinese
    if (FileExists(fontPath)) {
        io.Fonts->AddFontFromFileTTF(fontPath, 17.0f * scale, nullptr, glyphRanges.Data);
        if (FileExists(fontCjkPath)) {
            // Merged into the main font: Segoe UI keeps Latin and Cyrillic, YaHei fills in the rest.
            ImFontConfig cjk;
            cjk.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(fontCjkPath, 17.0f * scale, &cjk, glyphRanges.Data);
        }
        ui.fontBig = io.Fonts->AddFontFromFileTTF(FileExists(fontBoldPath) ? fontBoldPath : fontPath, 40.0f * scale);
    } else {
        ImFontConfig cfg;
        cfg.SizePixels = 13.0f * scale;
        io.Fonts->AddFontDefault(&cfg);
        cfg.SizePixels = 30.0f * scale;
        ui.fontBig = io.Fonts->AddFontDefault(&cfg);
    }
    ApplyTheme(scale);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);

    LARGE_INTEGER last;
    QueryPerformanceCounter(&last);
    bool done = false;

    // Capture thread CPU: thread cycles are counted in TSC ticks, so the TSC frequency is
    // measured against QPC over the whole run (more precise the longer the app runs).
    const uint64_t tscStart = __rdtsc();
    const int64_t qpcStart = last.QuadPart;
    int64_t cpuQpc = last.QuadPart;
    uint64_t cpuCycles = capture.cycles(), cpuHandled = capture.handled();
    while (!done) {
        // Window messages with a time budget: with an 8000 Hz mouse Windows keeps generating
        // new WM_MOUSEMOVE while we process them and the loop might never end (measured:
        // up to 475 ms per frame). Leftovers are handled next frame (ImGui only needs the last position).
        LARGE_INTEGER pumpStart, pumpNow;
        QueryPerformanceCounter(&pumpStart);
        const int64_t pumpBudget = freq.QuadPart / 500; // 2 ms
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) done = true;
            QueryPerformanceCounter(&pumpNow);
            if (pumpNow.QuadPart - pumpStart.QuadPart > pumpBudget) break;
        }
        if (done) break;

        // Drain the capture queue even when the window is hidden, so it never fills up
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        MouseEvent ev;
        while (capture.queue().pop(ev)) analyzer->process(ev);
        analyzer->refresh(now.QuadPart);
        const double dt = double(now.QuadPart - last.QuadPart) / double(freq.QuadPart);
        last = now;
        ui.frameMs.push(float(dt * 1000.0));

        if (now.QuadPart - cpuQpc > freq.QuadPart / 2) { // every 0.5 s
            const double tscHz = double(__rdtsc() - tscStart) * double(freq.QuadPart) / double(now.QuadPart - qpcStart);
            const double seconds = double(now.QuadPart - cpuQpc) / double(freq.QuadPart);
            const uint64_t cycles = capture.cycles(), handled = capture.handled();
            const double busy = double(cycles - cpuCycles) / tscHz; // seconds of CPU
            ui.captureCpuPct = 100.0 * busy / seconds;
            ui.captureUsPerPacket = handled > cpuHandled ? busy * 1e6 / double(handled - cpuHandled) : -1.0;
            cpuQpc = now.QuadPart; cpuCycles = cycles; cpuHandled = handled;
        }

        if (g_occluded && g_swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        g_occluded = false;

        if (g_resizeW != 0 && g_resizeH != 0) {
            CleanupRenderTarget();
            g_swapChain->ResizeBuffers(0, g_resizeW, g_resizeH, DXGI_FORMAT_UNKNOWN, 0);
            g_resizeW = g_resizeH = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        DrawUi(*analyzer, ui, dt, capture.dropped());
        ImGui::Render();

        const float clear[4] = {0.065f, 0.070f, 0.080f, 1.0f};
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        const HRESULT hr = g_swapChain->Present(1, 0); // vsync
        g_occluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    capture.stop();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
