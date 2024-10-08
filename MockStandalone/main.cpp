// an example imgui standalone app for testing the addon

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <format>
#include <filesystem>

#include <d3d11.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain *g_pSwapChain = nullptr;
static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);

void CleanupDeviceD3D();

void CreateRenderTarget();

void CleanupRenderTarget();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static std::string get_dir() {
    wchar_t path[FILENAME_MAX] = {0};
    GetModuleFileNameW(nullptr, path, FILENAME_MAX);

    auto path_str = std::format("{}", std::filesystem::path(path).parent_path().string());

    return path_str;
}

typedef void (*ADDON_LOAD)(void *ctxt, void *, void *, void *, const char *);

typedef void (*ADDON_RENDER)();

void LoadTextureFromResource(
    const int resId,
    const HMODULE hInstance,
    void *&texture,
    int &out_width,
    int &out_height
) {
    HRSRC resource = FindResource(hInstance, MAKEINTRESOURCE(resId), "PNG");
    const HGLOBAL loadedResource = LoadResource(hInstance, resource);
    const void *pData = LockResource(loadedResource);
    const DWORD size = SizeofResource(hInstance, resource);

    int width, height, channels;
    unsigned char *imageData = stbi_load_from_memory(
        static_cast<const stbi_uc *>(pData),
        static_cast<int>(size),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    if (imageData == nullptr) {
        return;
    }

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = imageData;
    initData.SysMemPitch = width * 4;

    ID3D11Texture2D *pTexture = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&textureDesc, &initData, &pTexture);
    if (FAILED(hr)) {
        stbi_image_free(imageData);
        return;
    }

    ID3D11ShaderResourceView *pSRV = nullptr;
    hr = g_pd3dDevice->CreateShaderResourceView(pTexture, nullptr, &pSRV);
    if (FAILED(hr)) {
        pTexture->Release();
        stbi_image_free(imageData);
        return;
    }

    stbi_image_free(imageData);
    pTexture->Release();

    texture = pSRV;
    out_width = width;
    out_height = height;
}

int WinMain(
    HINSTANCE /*hInstance*/,
    HINSTANCE /*hPrevInstance*/,
    LPSTR /*lpCmdLine*/,
    int /*nShowCmd*/
) {
    const HMODULE searchDll = LoadLibrary("Finder.dll");

    if (!searchDll) {
        std::cerr << "failed to load the search dll." << std::endl;
        return -1;
    }

    const auto AddonLoadStandalone = reinterpret_cast<ADDON_LOAD>(GetProcAddress(searchDll, "AddonLoadStandalone"));
    const auto AddonRenderStandalone = reinterpret_cast<ADDON_RENDER>(
        GetProcAddress(searchDll, "AddonRenderStandalone"));

    if (!AddonLoadStandalone || !AddonRenderStandalone) {
        std::cerr << "failed to load addon functions." << std::endl;
        FreeLibrary(searchDll);
        return -1;
    }

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    const WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        L"Mock Standalone",
        nullptr
    };
    RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName,
                                L"Mock Standalone app for Search addon",
                                WS_OVERLAPPEDWINDOW,
                                100,
                                100,
                                1280,
                                800,
                                nullptr,
                                nullptr,
                                wc.hInstance,
                                nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    const auto ctxt = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // state
    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    auto dir = get_dir();

    AddonLoadStandalone(ctxt, ImGui::MemAlloc, ImGui::MemFree, LoadTextureFromResource, dir.c_str());

    if (std::filesystem::exists(std::format("{0}\\test_font.ttf", dir))) {
        io.Fonts->AddFontFromFileTTF(std::format("{0}\\test_font.ttf", dir).c_str(), 17.0f);
    }

    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        if (!io.Fonts->Fonts.empty()) {
            ImFont *font = io.Fonts->Fonts[0];
            ImGui::PushFont(font);
        }
        AddonRenderStandalone();

        ImGui::PopFont();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        const HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    FreeLibrary(searchDll);

    return 0;
}


// Helper functions

bool CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    constexpr D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,};
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr,
                                                D3D_DRIVER_TYPE_HARDWARE,
                                                nullptr,
                                                createDeviceFlags,
                                                featureLevelArray,
                                                2,
                                                D3D11_SDK_VERSION,
                                                &sd,
                                                &g_pSwapChain,
                                                &g_pd3dDevice,
                                                &featureLevel,
                                                &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr,
                                            D3D_DRIVER_TYPE_WARP,
                                            nullptr,
                                            createDeviceFlags,
                                            featureLevelArray,
                                            2,
                                            D3D11_SDK_VERSION,
                                            &sd,
                                            &g_pSwapChain,
                                            &g_pd3dDevice,
                                            &featureLevel,
                                            &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

void CreateRenderTarget() {
    ID3D11Texture2D *pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED)
                return 0;
            g_ResizeWidth = (UINT) LOWORD(lParam); // Queue resize
            g_ResizeHeight = (UINT) HIWORD(lParam);
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
