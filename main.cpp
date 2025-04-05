// If you want to create only the .txt

// #include "journal/UsnJournal.hh"

// int main() {
//    USNReader::ReadUSNJournal(L"C:");
//    return 0;
// }

#include <d3d9.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"

#include "UsnJournal.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LPDIRECT3D9 d3d;
LPDIRECT3DDEVICE9 d3d_device;
HWND hwnd;
bool isRunning = true;
bool usnDone = false;
std::vector<std::vector<std::string>> usnEntries;
std::mutex resultMutex;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_CLOSE:
        isRunning = false;
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void CreateDevice(HWND hWnd) {
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3d_device);
}

void CleanupDevice() {
    if (d3d_device) d3d_device->Release();
    if (d3d) d3d->Release();
}

void Render() {
    d3d_device->Clear(0, nullptr, D3DCLEAR_TARGET,
        D3DCOLOR_XRGB(20, 20, 20), 1.0f, 0);
    d3d_device->BeginScene();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    d3d_device->EndScene();
    d3d_device->Present(nullptr, nullptr, nullptr, nullptr);
}

void LoadUSNResults() {
    USNReader::ReadUSNJournal(L"C:");
    std::ifstream in("USN_Journal_Log.txt");
    std::string line;

    std::lock_guard<std::mutex> lock(resultMutex);
    usnEntries.clear();
    while (std::getline(in, line)) {
        if (!line.empty()) {
            std::vector<std::string> entry;
            std::istringstream ss(line);
            std::string token;
            while (std::getline(ss, token, '*')) {
                size_t pos = token.find(": ");
                if (pos != std::string::npos) {
                    entry.push_back(token.substr(pos + 2));
                }
            }
            if (entry.size() == 5) {
                usnEntries.push_back(entry);
            }
        }
    }
    usnDone = true;
}

int CompareSystemTime(const SYSTEMTIME* st1, const SYSTEMTIME* st2) {
    FILETIME ft1, ft2;
    SystemTimeToFileTime(st1, &ft1);
    SystemTimeToFileTime(st2, &ft2);

    return CompareFileTime(&ft1, &ft2);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      L"ImGuiUSNWindow", NULL };
    RegisterClassEx(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    hwnd = CreateWindowExW(0, L"ImGuiUSNWindow", L"USN Journal",
        WS_OVERLAPPEDWINDOW, 0, 0, screenWidth, screenHeight,
        NULL, NULL, wc.hInstance, NULL);
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(hwnd);

    CreateDevice(hwnd);
    ImGui::CreateContext();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.80f, 0.85f, 0.92f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.75f, 0.85f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.65f, 0.78f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.72f, 0.78f, 0.88f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.65f, 0.70f, 0.82f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.60f, 0.75f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.90f, 0.92f, 0.96f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.82f, 0.86f, 0.92f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.75f, 0.80f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.70f, 0.75f, 0.85f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.60f, 0.65f, 0.78f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.80f, 0.85f, 0.92f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.82f, 0.87f, 0.95f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.75f, 0.80f, 0.90f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.85f, 0.88f, 0.95f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.15f, 1.00f);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 17.0f);
    io.Fonts->Build();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(d3d_device);

    std::thread usnThread(LoadUSNResults);
    usnThread.detach();

    static char searchBuffer[256] = "";
    static std::vector<int> filteredIndices;
    static std::string previousSearchBuffer = "";

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT && isRunning) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        if (previousSearchBuffer != searchBuffer) {
            previousSearchBuffer = searchBuffer;

            filteredIndices.clear();
            std::string query(searchBuffer);
            std::transform(query.begin(), query.end(), query.begin(), ::tolower);

            std::vector<int> fullIndices;
            const std::vector<int>* displayIndices = nullptr;

            if (query.empty()) {
                fullIndices.reserve(usnEntries.size());
                for (int i = 0; i < usnEntries.size(); ++i)
                    fullIndices.push_back(i);
                displayIndices = &fullIndices;
            }
            else {
                std::vector<std::string> conditions;
                size_t startPos = 0;
                while (true) {
                    size_t semicolonPos = query.find(';', startPos);
                    std::string condition = query.substr(startPos, (semicolonPos == std::string::npos) ? std::string::npos : semicolonPos - startPos);
                    conditions.push_back(condition);

                    if (semicolonPos == std::string::npos) break;
                    startPos = semicolonPos + 1;
                }

                for (size_t i = 0; i < usnEntries.size(); ++i) {
                    bool matchesAllConditions = true;

                    for (const auto& condition : conditions) {
                        size_t colonPos = condition.find(':');
                        if (colonPos == std::string::npos) continue;

                        std::string column = condition.substr(0, colonPos);
                        std::string value = condition.substr(colonPos + 1);
                        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

                        int columnIndex = -1;
                        if (column == "usn") columnIndex = 0;
                        else if (column == "name") columnIndex = 1;
                        else if (column == "date") columnIndex = 2;
                        else if (column == "reason") columnIndex = 3;
                        else if (column == "directory") columnIndex = 4;

                        if (columnIndex != -1) {
                            std::string field = usnEntries[i][columnIndex];
                            std::transform(field.begin(), field.end(), field.begin(), ::tolower);
                            if (field.rfind(value, 0) != 0) {
                                matchesAllConditions = false;
                                break;
                            }
                        }
                    }

                    if (matchesAllConditions) {
                        filteredIndices.push_back(static_cast<int>(i));
                    }
                }

                displayIndices = &filteredIndices;
            }

            filteredIndices = *displayIndices;
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("USN Journal Analysis", nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
        ImGui::SetWindowPos(ImVec2(0, 0));
        ImGui::SetWindowSize(io.DisplaySize);

        if (!usnDone) {
            ImVec2 textSize = ImGui::CalcTextSize("Analyzing USN Journal...");
            ImVec2 center = ImVec2((io.DisplaySize.x - textSize.x) * 0.5f, (io.DisplaySize.y - textSize.y) * 0.5f);
            ImGui::SetCursorPos(center);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Analyzing USN Journal...");
        }
        else {
            if (filteredIndices.empty()) {
                filteredIndices.reserve(usnEntries.size());
                for (int i = 0; i < usnEntries.size(); ++i)
                    filteredIndices.push_back(i);
                previousSearchBuffer = "";
            }

            ImGui::InputTextWithHint("##Search", "Search... (e.g: Name:console;Reason:file delete)", searchBuffer, IM_ARRAYSIZE(searchBuffer));

            if (ImGui::BeginTable("USNTable", 5,
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable |
                ImGuiTableFlags_Hideable |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                ImGuiTableFlags_BordersV |
                ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_SizingStretchSame |
                ImGuiTableFlags_Sortable))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("USN", ImGuiTableColumnFlags_DefaultSort);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
                ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_None);
                ImGui::TableSetupColumn("Reason", ImGuiTableColumnFlags_None);
                ImGui::TableSetupColumn("Directory", ImGuiTableColumnFlags_None);
                ImGui::TableHeadersRow();

                ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
                if (sortSpecs && sortSpecs->SpecsCount > 0 && sortSpecs->SpecsDirty) {
                    const ImGuiTableColumnSortSpecs& sort = sortSpecs->Specs[0];
                    int columnIndex = sort.ColumnIndex;
                    bool ascending = (sort.SortDirection == ImGuiSortDirection_Ascending);

                    std::sort(filteredIndices.begin(), filteredIndices.end(),
                        [&](int lhs, int rhs) {
                            const std::string& left = usnEntries[lhs][columnIndex];
                            const std::string& right = usnEntries[rhs][columnIndex];

                            if (columnIndex == 2) {
                                std::tm tmL = {}, tmR = {};
                                std::istringstream ssL(left), ssR(right);

                                ssL >> std::get_time(&tmL, "%Y-%m-%d %H:%M:%S");
                                ssR >> std::get_time(&tmR, "%Y-%m-%d %H:%M:%S");

                                SYSTEMTIME stL, stR;
                                stL.wYear = tmL.tm_year + 1900;
                                stL.wMonth = tmL.tm_mon + 1;
                                stL.wDay = tmL.tm_mday;
                                stL.wHour = tmL.tm_hour;
                                stL.wMinute = tmL.tm_min;
                                stL.wSecond = tmL.tm_sec;
                                stL.wMilliseconds = 0;

                                stR.wYear = tmR.tm_year + 1900;
                                stR.wMonth = tmR.tm_mon + 1;
                                stR.wDay = tmR.tm_mday;
                                stR.wHour = tmR.tm_hour;
                                stR.wMinute = tmR.tm_min;
                                stR.wSecond = tmR.tm_sec;
                                stR.wMilliseconds = 0;

                                int result = CompareSystemTime(&stL, &stR);
                                return ascending ? (result < 0) : (result > 0);
                            }
                            else {
                                return ascending ? (left < right) : (left > right);
                            }
                        });

                    sortSpecs->SpecsDirty = false;
                }

                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(filteredIndices.size()));
                while (clipper.Step()) {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                        int row = filteredIndices[i];
                        ImGui::TableNextRow();
                        for (int col = 0; col < 5; ++col) {
                            ImGui::TableSetColumnIndex(col);
                            ImGui::TextUnformatted(usnEntries[row][col].c_str());
                        }
                    }
                }
                ImGui::EndTable();
            }
        }

        ImGui::End();
        ImGui::EndFrame();
        Render();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
