#include "includes.h"
#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include "include/xorstr/xorstr.hpp"
#include "cfx/resource.h"
#include "cfx/resource_manager.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#include "TextEditor.h"

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

std::vector<std::string> m_savedScripts;

// Declare showMenu before WndProc so it can be used
// Declare showMenu before WndProc so it can be used
static bool showMenu = false;

// Execution state handling
static std::atomic<bool> g_executionPending{false};
static std::string g_scriptBuffer;
static int g_targetResourceIndex = -1;

void InitImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (showMenu && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

static SHORT lastInsertState = 0;
bool init = false;
static float menuAlpha = 0.0f;

void SetupCustomImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsLight();

    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.ScrollbarSize = 14;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowRounding = 16.0f;
    style.FrameRounding = 12.0f;
    style.GrabRounding = 10.0f;
    style.ScrollbarRounding = 12.0f;
    style.TabRounding = 12.0f;

    ImVec4* colors = style.Colors;
    // Modern mavi-mor ve açık tonlar
    colors[ImGuiCol_Text] = ImVec4(0.13f, 0.14f, 0.23f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.93f, 0.95f, 0.99f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.95f, 0.97f, 1.00f, 0.95f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.85f, 0.88f, 0.98f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.45f, 0.48f, 0.80f, 0.40f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.85f, 0.88f, 0.98f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.73f, 0.76f, 0.95f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.60f, 0.62f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.60f, 0.62f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.93f, 0.95f, 0.99f, 0.75f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.85f, 0.88f, 0.98f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.93f, 0.95f, 0.99f, 0.70f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.60f, 0.62f, 0.90f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.38f, 0.70f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.62f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.60f, 0.62f, 0.90f, 0.90f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.38f, 0.70f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.60f, 0.62f, 0.90f, 0.85f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.38f, 0.70f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.60f, 0.62f, 0.90f, 0.70f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.38f, 0.70f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.60f, 0.62f, 0.90f, 0.85f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.38f, 0.70f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.85f, 0.88f, 0.98f, 0.90f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.60f, 0.62f, 0.90f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.93f, 0.95f, 0.99f, 0.70f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.85f, 0.88f, 0.98f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.60f, 0.62f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.60f, 0.62f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.45f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.60f, 0.62f, 0.90f, 0.40f);
}


TextEditor luaEditor;
bool editorInitialized = false;

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!init)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
        {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
            InitImGui();
            SetupCustomImGuiStyle();
            init = true;
        }
        else
            return oPresent(pSwapChain, SyncInterval, Flags);
    }

    SHORT currentInsertState = GetAsyncKeyState(VK_INSERT);
    if ((currentInsertState & 0x8000) && !(lastInsertState & 0x8000))
    {
        showMenu = !showMenu;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Only draw cursor when menu is actually visible
    io.MouseDrawCursor = showMenu && (menuAlpha > 0.0f);
    lastInsertState = currentInsertState;

    if (showMenu)
    {
        menuAlpha = (menuAlpha + 0.05f > 1.0f) ? 1.0f : menuAlpha + 0.05f;
    }
    else
    {
        menuAlpha = (menuAlpha - 0.05f < 0.0f) ? 0.0f : menuAlpha - 0.05f;
    }

    if (menuAlpha > 0.0f)
    {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
        ImGui::Begin(xorstr_("DX11 ImGui Lua Console (Educational Build)"), nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

        if (ImGui::BeginTabBar(xorstr_("WindowTabBar")))
        {
            if (ImGui::BeginTabItem(xorstr_("Executor")))
            {
                auto resources = fx::ResourceManager::getCurrentIl()->getAllResources();
                static int selectedResourceIndex = 0;
                static std::string scriptInput = xorstr_("print('Hello from the educational build')");

                // Index bounds check - resource list may have changed
                if (resources.empty())
                    selectedResourceIndex = 0;
                else if (selectedResourceIndex >= (int)resources.size())
                    selectedResourceIndex = (int)resources.size() - 1;

                if (!editorInitialized)
                {
                    TextEditor::Palette customPalette = TextEditor::GetLightPalette();
                    // Modern mavi ve mor tonları
                    customPalette[(int)TextEditor::PaletteIndex::Default] = 0xFF22223B;
                    customPalette[(int)TextEditor::PaletteIndex::Background] = 0xFFEDF2FB;
                    customPalette[(int)TextEditor::PaletteIndex::Cursor] = 0xFF9F86C0;
                    customPalette[(int)TextEditor::PaletteIndex::Selection] = 0xFF4A4E69;
                    customPalette[(int)TextEditor::PaletteIndex::Keyword] = 0xFF5F6CAF;
                    customPalette[(int)TextEditor::PaletteIndex::Number] = 0xFF9F86C0;
                    customPalette[(int)TextEditor::PaletteIndex::String] = 0xFF4A4E69;
                    customPalette[(int)TextEditor::PaletteIndex::CharLiteral] = 0xFF9A8C98;
                    customPalette[(int)TextEditor::PaletteIndex::Identifier] = 0xFF22223B;
                    customPalette[(int)TextEditor::PaletteIndex::Comment] = 0xFFB8C1EC;
                    customPalette[(int)TextEditor::PaletteIndex::MultiLineComment] = 0xFFB8C1EC;
                    customPalette[(int)TextEditor::PaletteIndex::Preprocessor] = 0xFF5F6CAF;
                    customPalette[(int)TextEditor::PaletteIndex::ErrorMarker] = 0xFFD7263D;
                    customPalette[(int)TextEditor::PaletteIndex::Breakpoint] = 0xFFD7263D;
                    customPalette[(int)TextEditor::PaletteIndex::LineNumber] = 0xFF9A8C98;
                    customPalette[(int)TextEditor::PaletteIndex::CurrentLineFill] = 0x224A4E69;
                    customPalette[(int)TextEditor::PaletteIndex::CurrentLineFillInactive] = 0x11000000;
                    luaEditor.SetPalette(customPalette);
                }

                luaEditor.Render(xorstr_("LuaEditor"), ImVec2(-1, 325));

                if (ImGui::Button(xorstr_("Execute")))
                {
                    if (!resources.empty() && selectedResourceIndex >= 0 && selectedResourceIndex < (int)resources.size())
                    {
                        fx::fwRefContainer<fx::Resource> r = resources[selectedResourceIndex];
                        if (r.GetRef() != nullptr)
                        {
                            // Only queue if no execution is already pending
                            if (!g_executionPending.load())
                            {
                                g_scriptBuffer = luaEditor.GetText() + ";";
                                g_targetResourceIndex = selectedResourceIndex;
                                g_executionPending.store(true);
                            }
                        }
                    }
                    else
                    {
                        MessageBoxA(nullptr, xorstr_("Error finding resource"), xorstr_("Error"), 0);
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button(xorstr_("Clear")))
                {
                    luaEditor.SetText(xorstr_(""));
                }

                ImGui::SameLine();

                std::string curNameStr = xorstr_("Select a resource to execute");
                if (!resources.empty() && selectedResourceIndex >= 0 && selectedResourceIndex < (int)resources.size())
                {
                    auto& res = resources[selectedResourceIndex];
                    if (res.GetRef() != nullptr)
                        curNameStr = std::string(res->GetName());
                }

                if (ImGui::BeginCombo(xorstr_("##resourceCombo"), curNameStr.c_str()))
                {
                    for (int i = 0; i < (int)resources.size(); i++)
                    {
                        auto& res = resources[i];
                        if (res.GetRef() == nullptr) continue;
                        std::string nameStr = std::string(res->GetName());
                        bool isSelected = (selectedResourceIndex == i);
                        if (ImGui::Selectable(nameStr.c_str(), isSelected))
                        {
                            selectedResourceIndex = i;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(xorstr_("Resources")))
            {
                auto resources = fx::ResourceManager::getCurrentIl()->getAllResources();
                int Count = (int)resources.size();
                static int CurrentId = -1;

                if (CurrentId >= Count)
                    CurrentId = -1;

                static std::vector<bool> started_flags;
                if ((int)started_flags.size() != Count)
                {
                    started_flags.assign(Count, true);
                }

                ImGui::BeginGroup();

                static ImGuiTextFilter filter;
                filter.Draw(xorstr_("##Filter"), ImGui::GetContentRegionAvailWidth());

                if (ImGui::BeginChild(xorstr_("##ResourceList"), ImVec2{ 0, -ImGui::GetFrameHeightWithSpacing() }, true))
                {
                    for (int i = 0; i < Count; i++)
                    {
                        std::string name = std::string(resources[i]->GetName());
                        bool selected = (CurrentId == i);

                        if (filter.PassFilter(name.c_str()))
                        {
                            std::string displayName = name + (started_flags[i] ? xorstr_(" - Started") : xorstr_(" - Stopped"));
                            if (ImGui::Selectable(displayName.c_str(), selected))
                            {
                                CurrentId = i;
                            }
                        }
                        else if (selected)
                        {
                            CurrentId = -1;
                        }
                    }
                    ImGui::EndChild();
                }

                if (CurrentId != -1)
                {
                    if (ImGui::Button(xorstr_("Start")))
                    {
                        resources[CurrentId]->get_impl()->Start();
                        started_flags[CurrentId] = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(xorstr_("Stop")))
                    {
                        resources[CurrentId]->get_impl()->Stop();
                        started_flags[CurrentId] = false;
                    }
                }
                else
                {
                    // Use PushStyleVar to make buttons look disabled
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    ImGui::Button(xorstr_("Start"));
                    ImGui::SameLine();
                    ImGui::Button(xorstr_("Stop"));
                    ImGui::PopStyleVar();
                }

                ImGui::EndGroup();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID)
{
    if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
    {
        // Hook into the Game Thread via ResourceManager::OnTick
        // This ensures all script and network operations happen on the correct thread
        auto manager = fx::ResourceManager::getCurrent();
        if (manager)
        {
            manager->OnTick.Connect([]()
            {
                bool expected = true;
                if (g_executionPending.compare_exchange_strong(expected, false))
                {
                    if (g_targetResourceIndex >= 0)
                    {
                        try
                        {
                            auto resources = fx::ResourceManager::getCurrentIl()->getAllResources();
                            if (g_targetResourceIndex < (int)resources.size())
                            {
                                fx::fwRefContainer<fx::Resource> r = resources[g_targetResourceIndex];
                                if (r.GetRef() != nullptr)
                                {
                                    std::string scriptToExecute = g_scriptBuffer;
                                    
                                    size_t handle = r->Runtime.Connect(
                                        [scriptToExecute, callCount = std::make_shared<int>(0)](std::vector<char>* info)
                                        {
                                            if (*callCount == 4)
                                            {
                                                info->insert(info->begin(), scriptToExecute.begin(), scriptToExecute.end());
                                            }
                                            (*callCount)++;
                                        }
                                    );
                                    
                                    // Stop/Start on Game Thread is safe for network operations
                                    r->get_impl()->Stop();
                                    r->get_impl()->Start();
                                    r->Runtime.Disconnect(handle);
                                }
                            }
                        }
                        catch (...) {
                            // Silently handle errors
                        }
                    }
                }
            });
        }

        kiero::bind(8, (void**)&oPresent, hkPresent);
        Beep(1000, 200);
    }
    else
    {
        MessageBoxA(nullptr, xorstr_("Error inicializando hook"), xorstr_("K9"), MB_OK | MB_ICONERROR);
    }

    return TRUE;
}

static std::thread g_executionWorker;

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        kiero::shutdown();
        break;
    }
    return TRUE;
}