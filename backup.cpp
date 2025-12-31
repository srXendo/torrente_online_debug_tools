// dllmain.cpp : Define el punto de entrada de la aplicación DLL.


#include "pch.h"
#include <fstream>
#include <commctrl.h>
#include <vector>
#include <string>
#include <windows.h>
HMODULE g_hModule = nullptr;
#pragma comment(lib, "comctl32.lib")

#define IDC_LISTVIEW 1001
#define IDC_BTN_CONNECT 1002
#define IDC_BTN_CANCEL 1003


struct Partida {
    std::string name_party;
    std::string map_party;
    std::string ip;
    std::string port;
};

std::vector<Partida> partidas = {
    {"partida 1", "dm_plaza", " gunner.es", "8889"},
    {"partida 2", "dm_plaza", "1.1.1.2", "8080"},
    {"partida pepe", "dm_plaza", "1.1.3.1", "8080"}
};



void InitConsole()
{
    AllocConsole(); // crea una nueva consola
    //freopen("CONOUT$", "w", stdout);  // redirige std::cout
    //freopen("CONOUT$", "w", stderr);
    //freopen("CONIN$", "r", stdin);
    std::cout.clear(); // limpia cualquier flag de error
    std::cout << "[+] Consola creada con éxito\n";
}
void PatchMemory(BYTE* dst, BYTE* src, size_t size) {
    DWORD oldProtect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(dst, src, size);
    VirtualProtect(dst, size, oldProtect, &oldProtect);
}
void* GetFunctionOffset(const char* moduleName, const char* functionName, size_t offset) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) return nullptr;

    void* funcAddr = (void*)GetProcAddress(hModule, functionName);
    if (!funcAddr) return nullptr;

    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(funcAddr) + offset);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hListView, hBtnConnect, hBtnCancel;

    switch (msg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
        InitCommonControlsEx(&icex);

        hListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
            10, 10, 500, 200,
            hwnd, (HMENU)IDC_LISTVIEW, GetModuleHandle(NULL), NULL);

        ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);

        LVCOLUMN col;
        col.mask = LVCF_TEXT | LVCF_WIDTH;

        col.pszText = (LPSTR)"Nombre de Partida";
        col.cx = 160;
        ListView_InsertColumn(hListView, 0, &col);

        col.pszText = (LPSTR)"Mapa";
        col.cx = 120;
        ListView_InsertColumn(hListView, 1, &col);

        col.pszText = (LPSTR)"IP";
        col.cx = 160;
        ListView_InsertColumn(hListView, 2, &col);
        
        col.pszText = (LPSTR)"PUERTO";
        col.cx = 160;
        ListView_InsertColumn(hListView, 3, &col);

        for (int i = 0; i < partidas.size(); ++i) {
            LVITEM item = { 0 };
            item.mask = LVIF_TEXT;
            item.iItem = i;

            item.pszText = (LPSTR)partidas[i].name_party.c_str();
            ListView_InsertItem(hListView, &item);

            ListView_SetItemText(hListView, i, 1, (LPSTR)partidas[i].map_party.c_str());
            ListView_SetItemText(hListView, i, 2, (LPSTR)partidas[i].ip.c_str());
            ListView_SetItemText(hListView, i, 3, (LPSTR)partidas[i].port.c_str());
        }

        hBtnConnect = CreateWindow("BUTTON", "Conectar",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            320, 220, 90, 30, hwnd, (HMENU)IDC_BTN_CONNECT, GetModuleHandle(NULL), NULL);

        hBtnCancel = CreateWindow("BUTTON", "Cancelar",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            420, 220, 90, 30, hwnd, (HMENU)IDC_BTN_CANCEL, GetModuleHandle(NULL), NULL);

        EnableWindow(hBtnConnect, FALSE);  // Deshabilitar hasta que se seleccione algo
        break;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_BTN_CONNECT) {
            int selected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (selected >= 0 && selected < partidas.size()) {
                std::string mensaje = "Conectando a la IP: " + partidas[selected].ip + ":"+ partidas[selected].port;
                MessageBoxA(hwnd, mensaje.c_str(), "Conexión", MB_OK | MB_ICONINFORMATION);

                SetForegroundWindow(hwnd);
                SetActiveWindow(hwnd);
                ShowWindow(hwnd, SW_SHOW);
                SetFocus(hwnd); // o el campo que estés usando
                //start set ip manual
                uintptr_t base = (uintptr_t)GetModuleHandleA("vtKernel.dll");
                uintptr_t ptrBase = *(uintptr_t*)(base + 0xC00C4);
                uintptr_t ptr = *(uintptr_t*)(ptrBase + 0x14);
                ptr = *(uintptr_t*)(ptr + 0xC);
                ptr = *(uintptr_t*)(ptr + 0x10);
                ptr = *(uintptr_t*)(ptr + 0xC);
                ptr = *(uintptr_t*)(ptr + 0xC);
                ptr = *(uintptr_t*)(ptr + 0xC);
                char* destino = (char*)(ptr + 0xA4);

                char texto[32];
                strcpy_s(texto, sizeof(texto), partidas[selected].ip.data());
                strcpy_s(destino, sizeof(texto)+1, texto); // +1 para el null terminator
                //end set ip manual

                //start set port manual
                uintptr_t ptrBase2 = *(uintptr_t*)(base + 0xC00C4);
                uintptr_t ptr2 = *(uintptr_t*)(ptrBase2 + 0x14);
                ptr2 = *(uintptr_t*)(ptr2 + 0xC);
                ptr2 = *(uintptr_t*)(ptr2 + 0xC);
                ptr2 = *(uintptr_t*)(ptr2 + 0x10);
                ptr2 = *(uintptr_t*)(ptr2 + 0xC);
                ptr2 = *(uintptr_t*)(ptr2 + 0x10);
                char* destino2 = (char*)(ptr2 + 0xA4);

                char texto2[5];
                strcpy_s(texto2, sizeof(texto2), partidas[selected].port.data());
                strcpy_s(destino2, sizeof(texto2), texto2); // +1 para el null terminator
                //end set port manual


                // Supongamos que queremos escribir NOPs en "main.MainMenu::Run+0x20E"
                const char* moduleName = "Main.dll";  // O DLL si aplica
                const char* functionName = "MainMenu::Run";  // Nombre sin namespace si compilas sin decorador
                size_t offset = 0x2E2;
                InitConsole(); // crea consola
       
                //PatchAndRestore();

                std::cout << "Inyección exitosa!\n";

                    MessageBoxA(hwnd, "parche aplicado correctamente", "Test", MB_OK);
                    
                    //antes de forzar el click
                     
                    //despues de hacer o forzar el click 
                    //BYTE original[6] = { 0x89, 0x8E, 0x58, 0x11, 0x00, 0x00 };  // 6 NOPs
                    //PatchMemory(target, original, sizeof(original));


            }
        }
        if (LOWORD(wParam) == IDC_BTN_CANCEL) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        break;
    }

    case WM_NOTIFY: {
        if (((LPNMHDR)lParam)->idFrom == IDC_LISTVIEW &&
            ((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
            NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
            int selected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            EnableWindow(hBtnConnect, selected >= 0);
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI MostrarVentanaTabla(LPVOID) {
    Sleep(100);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hModule;
    wc.lpszClassName = "TablaPartidas";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "TablaPartidas", "Xendo tol MatchMaker",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 320,
        NULL, NULL, wc.hInstance, NULL);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
extern "C" __declspec(dllexport) void main_window() {
  
    std::wofstream file(L"test.txt");
    if (file.is_open()) {
        file << L"hola mundo";
        file.close();
    }
    
    wchar_t dir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, dir);
    MessageBoxW(NULL, dir, L"Directorio de trabajo", MB_OK);
    MessageBox(NULL, "texto de prueba", "XendoMatchMaker", MB_OK);
}
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, MostrarVentanaTabla, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

