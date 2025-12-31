// dllsub.cpp
#include <windows.h>
#include <d3d8.h>

#include <d3d8types.h>
#include <stdio.h>    // FILE, stdout, setvbuf
#include <fcntl.h>    // _O_TEXT
#include <io.h>       // _open_osfhandle
#include <psapi.h>
#include <cstdio>
#include "MinHook.h"

#include <cstdio>

HMODULE g_hModule = nullptr;

class vtMachine;
class CObject;
class vtRenderScene;
class vtRender;
typedef void(__cdecl* SetBBoxRender)(bool param_1);
typedef void (__thiscall* VtDeleteObject)(vtMachine* thisPtr, CObject *param_1);
typedef void(__cdecl* VtRemoveObject)(CObject *param_1);
typedef CObject* (__thiscall* GetObjectById)(vtMachine* thisPtr, int param_1);
typedef int (__thiscall* GetNumObjects)(vtMachine* thisPtr);
typedef void (__thiscall* vtMachine_Dump_t)(vtMachine* thisPtr, FILE* file);

typedef void(__cdecl* VtGraph_SetCullMode)(int param_1);
typedef void(__cdecl* VtGraph_DisableFog)();
typedef void(__cdecl* VtGraph_EnableFog)();
typedef void(__cdecl* VtGraph_TestMode)(int param_1, int param_2);
typedef void(__cdecl* VtGraph_TestCooperativeLevel)();
typedef void(__cdecl* VtGraph_DebugInfo_GetNumDrawPolys)(BOOL param);
typedef void(__cdecl* SetDebugMsgRender)(BOOL param);
typedef void(__cdecl* VtEnableLogs)(bool enable);

void* d3d8table[119];

//void** d3d8_vtable = new void*[119];

typedef HRESULT(STDMETHODCALLTYPE* tEndScene)(IDirect3DDevice8*);
tEndScene oEndScene = nullptr;



struct DX8StateBackup
{
    DWORD vertexShader;
    DWORD zEnable;
    DWORD alphaBlend;
    DWORD srcBlend;
    DWORD destBlend;
    DWORD lighting;
    D3DVIEWPORT8 viewport;
};

void BackupState(IDirect3DDevice8* dev, DX8StateBackup& s)
{
    dev->GetVertexShader(&s.vertexShader);
    dev->GetRenderState(D3DRS_ZENABLE, &s.zEnable);
    dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &s.alphaBlend);
    dev->GetRenderState(D3DRS_SRCBLEND, &s.srcBlend);
    dev->GetRenderState(D3DRS_DESTBLEND, &s.destBlend);
    dev->GetRenderState(D3DRS_LIGHTING, &s.lighting);
    dev->GetViewport(&s.viewport);
}

void RestoreState(IDirect3DDevice8* dev, DX8StateBackup& s)
{
    dev->SetVertexShader(s.vertexShader);
    dev->SetRenderState(D3DRS_ZENABLE, s.zEnable);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, s.alphaBlend);
    dev->SetRenderState(D3DRS_SRCBLEND, s.srcBlend);
    dev->SetRenderState(D3DRS_DESTBLEND, s.destBlend);
    dev->SetRenderState(D3DRS_LIGHTING, s.lighting);
    dev->SetViewport(&s.viewport);
}


struct VERTEX
{
    float x, y, z, rhw;
    DWORD color;
};

#define FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)


void DrawRectDX8_Safe(
    IDirect3DDevice8* dev,
    float x, float y,
    float w, float h,
    DWORD color
)
{
    DX8StateBackup state;
    BackupState(dev, state);

    // Estados para overlay
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    dev->SetTexture(0, nullptr);
    dev->SetVertexShader(FVF);

    VERTEX v[5] =
    {
        { x,     y,     0.f, 1.f, color },
        { x + w, y,     0.f, 1.f, color },
        { x + w, y + h, 0.f, 1.f, color },
        { x,     y + h, 0.f, 1.f, color },
        { x,     y,     0.f, 1.f, color }
    };

    dev->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, v, sizeof(VERTEX));

    RestoreState(dev, state);
}




HRESULT STDMETHODCALLTYPE hkEndScene(IDirect3DDevice8* pDevice)
{

    //hacer lo que sea
    //printf("hooked\n");

    /*
    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    */
    DrawRectDX8_Safe(pDevice, 100, 100, 200, 150, 0xFFFF0000);

    return oEndScene(pDevice);
}



BOOL get_3d_device(void** pointer_table, size_t size)
{
    IDirect3D8* d3d = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d)
        return false;

    //  Crear ventana dummy REAL
    WNDCLASSEXA wc = {
        sizeof(WNDCLASSEXA),
        CS_CLASSDC,
        DefWindowProcA,
        0L, 0L,
        GetModuleHandle(NULL),
        NULL, NULL, NULL, NULL,
        "DX8Dummy",
        NULL
    };

    RegisterClassExA(&wc);

    HWND hWnd = CreateWindowA(
        "DX8Dummy",
        "DX8Dummy",
        WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100,
        NULL, NULL, wc.hInstance, NULL
    );

    if (!hWnd)
        return false;

    // OBTENER formato real del escritorio
    D3DDISPLAYMODE d3ddm;
    if (FAILED(d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm)))
        return false;

    // 3 Inicializar TODOS los campos obligatorios
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    d3dpp.Windowed               = TRUE;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat       = d3ddm.Format;   //  CLAVE
    d3dpp.BackBufferWidth        = 1;
    d3dpp.BackBufferHeight       = 1;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.hDeviceWindow          = hWnd;

    // Crear device HAL + SOFTWARE VP
    IDirect3DDevice8* device = nullptr;

    HRESULT hr = d3d->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,                     //  NUNCA REF
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, //  NUNCA HW
        &d3dpp,
        &device
    );

    if (FAILED(hr))
    {
        DestroyWindow(hWnd);
        d3d->Release();
        return false;
    }

    // 5 Copiar vtable
    memcpy(pointer_table, *(void***)(device), size);

    device->Release();
    d3d->Release();
    DestroyWindow(hWnd);

    return true;
}
DWORD WINAPI EntryThread(LPVOID)
{
    printf("iniciao");
    HMODULE hMod = GetModuleHandleA("vtkernel.dll");
    if (!hMod) {
        return 0;
    }
    printf("despues de hmod\n");
    SetBBoxRender setBBoxRender = (SetBBoxRender)GetProcAddress(
        hMod,
        "?SetBBoxRender@@YAX_N@Z"
    );
    VtDeleteObject vtDeleteObject = (VtDeleteObject)GetProcAddress(
        hMod,
        "?DeleteObject@vtMachine@@QAEXPAVCObject@@@Z"
    );
    VtRemoveObject vtRemoveObject = (VtRemoveObject)GetProcAddress(
        hMod,
        "?vtRemoveObject@@YAXPBVCObject@@@Z"
    );
    GetObjectById getObjectById = (GetObjectById)GetProcAddress(
        hMod,
        "?GetObjectById@vtMachine@@QAEPBVCObject@@H@Z"
    );
    GetNumObjects getNumObjects = (GetNumObjects)GetProcAddress(
        hMod,
        "?GetNumObjects@vtMachine@@QAEHXZ"
    );
    VtGraph_SetCullMode vtGraph_SetCullMode =
        (VtGraph_SetCullMode)GetProcAddress(
            hMod,
            "?vtGraph_SetCullMode@@YAXW4vtGraph_CullMode@@@Z"
        );
    vtMachine_Dump_t vtMachine_Dump =
        (vtMachine_Dump_t)GetProcAddress(
            hMod,
            "?Dump@vtMachine@@QAEXPAU_iobuf@@@Z"
        );
    VtGraph_DisableFog vtGraph_DisableFog = (VtGraph_DisableFog)GetProcAddress(
        hMod,
        "?vtGraph_DisableFog@@YAXXZ"
    );
    VtGraph_EnableFog vtGraph_EnableFog = (VtGraph_EnableFog)GetProcAddress(
        hMod,
        "?vtGraph_EnableFog@@YAXXZ"
    );
    VtGraph_TestCooperativeLevel vtGraph_TestCooperativeLevel = (VtGraph_TestCooperativeLevel)GetProcAddress(
        hMod,
        "?vtGraph_TestCooperativeLevel@@YA_NXZ"
    );
    VtGraph_DebugInfo_GetNumDrawPolys vtGraph_DebugInfo_GetNumDrawPolys = (VtGraph_DebugInfo_GetNumDrawPolys)GetProcAddress(
        hMod,
        "?vtGraph_DebugInfo_GetNumDrawPolys@@YAHXZ"
    );
    SetDebugMsgRender setDebugMsgRender =
        (SetDebugMsgRender)GetProcAddress(
            hMod,
            "?SetDebugMsgRender@@YAX_N@Z"
        );

    VtEnableLogs vtEnableLogs =
        (VtEnableLogs)GetProcAddress(
            hMod,
            "?vtEnableLogs@@YAX_N@Z"
        );
    VtGraph_TestMode vtGraph_TestMode =
        (VtGraph_TestMode)GetProcAddress(
            hMod,
            "?vtGraph_TestMode@@YA_NHH@Z"
        );
    vtRenderScene** ppMainRenderScene =
        (vtRenderScene**)GetProcAddress(hMod, "?mainRenderScene@@3PAVvtRenderScene@@A");
    vtMachine** ppMachine =
    (vtMachine**)GetProcAddress(
        hMod,
        "?machine@@3PAVvtMachine@@A"
    );
        /*if (vtMachine_Dump && ppMachine && *ppMachine)
        {
            printf("dump machine");

            FILE* f = fopen("dump_machine.txt", "wt");
            if (f)
            {
                vtMachine_Dump(*ppMachine, f);
                fclose(f);
            }
        }*/
        printf("setDebugMsgRender()\n");
        vtEnableLogs(TRUE); //habilita el volcado de logs en .txt
        setDebugMsgRender(TRUE);
        vtGraph_DebugInfo_GetNumDrawPolys(TRUE);
        vtGraph_TestCooperativeLevel();
        vtGraph_TestMode(1, 0);
        vtGraph_EnableFog();
        vtGraph_DisableFog();
        vtGraph_SetCullMode(2);
        int numObjects = 0;

        numObjects = getNumObjects(*ppMachine);
        setBBoxRender(TRUE);
        printf("Numero de objetos: %d\n", numObjects);
        for (int i = 2; i < numObjects- 40; i++) {
            printf("objeto nº: %d Borrado\n", i);
            CObject* obj = nullptr;
            obj = getObjectById(*ppMachine, i);
            vtDeleteObject(*ppMachine, obj);
            Sleep(200);
        }
        numObjects = getNumObjects(*ppMachine);
        printf("Numero de objetos-despues: %d\n", numObjects);

    if (!get_3d_device(d3d8table, sizeof(d3d8table)))
        return 0;

    MH_Initialize();

    MH_CreateHook(
        d3d8table[35], // EndScene DX8
        (LPVOID)&hkEndScene,
        reinterpret_cast<void**>(&oEndScene)
    );

    MH_EnableHook(d3d8table[35]);

        printf("iniciao \n");






    printf("FIN");
    return 0;
}

BOOL APIENTRY DllMain( 
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID
)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, EntryThread, nullptr, 0, nullptr);
    }
    else if(ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
    return TRUE;
}


