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
#include <vector>

HMODULE g_hModule = nullptr;

template<typename T>
struct FakeCArray
{
    T*   pData;      // offset 0x00
    int  nSize;      // offset 0x04
    int  nMaxSize;   // offset 0x08
};
class VtEngine;
class vtMachine;
class CObject;
class vtRenderScene;
class vtRender;
class VtWorld;
class MainScene;
class MainMenu;


typedef void* (__stdcall* GetNewObject)(void);
typedef char* (__thiscall* GetTexturePath)(VtEngine* thisPtr);
typedef void (__thiscall* SetBoneRenderState)(VtEngine* thisPtr,bool param_1);
typedef void (__thiscall* AdvancedMenu_t)(void* thisptr);
typedef FakeCArray<int> (__thiscall* GetObjectList)(MainScene* thisPtr);
typedef CObject* (__thiscall* GetObjectByName)(vtMachine* thisPtr,char *param_1);
typedef void(__cdecl* LogRenderScene)(int param_1);
typedef std::vector<int> ActorList;
typedef ActorList (__thiscall* GetActorList)(int param1);
typedef void (__thiscall* SetMeshRenderState)(VtEngine* thisPtr,bool param_1);
typedef void (__thiscall* SetCBoxRenderState)(VtEngine* thisPtr,bool param_1);
typedef void (__thiscall* SetBBoxRenderState)(VtEngine* thisPtr,bool param_1);
typedef void (__thiscall* SetPause)(VtEngine* thisPtr,bool param_1);

typedef VtWorld* (__cdecl* GetWorld)();
typedef void (__thiscall* Reload)(VtEngine* thisPtr);
typedef void (__thiscall* RemoveAllLights)(VtWorld* thisPtr);
typedef void (__thiscall* ClearObjectList)(vtMachine* thisPtr);
typedef void(__cdecl* EnableDither)(bool param_1);
typedef void(__cdecl* RemoveSceneObjects)(unsigned int param_1);
typedef unsigned int (__cdecl* vtFile_Length_t)(unsigned int fd);

typedef unsigned int (__cdecl* vtFile_Read_t)(
    unsigned int fd,
    void* buffer,
    unsigned int size
);
typedef unsigned int (__cdecl* vtFile_Open_t)(
    const char* path,
    unsigned char flags
);
typedef VtEngine* (__cdecl* GetEngine)();

typedef void(__cdecl* SetCBoxRender)(bool param_1);
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
void DumpMemory(void* ptr, size_t size)
{
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < size; i += 16)
    {
        printf("%08p  ", p + i);
        for (size_t j = 0; j < 16 && i + j < size; j++)
            printf("%02X ", p[i + j]);
        printf("\n");
    }

}
void ScanForMethods(void* obj, size_t size)
{
    BYTE* p = (BYTE*)obj;

    for (size_t off = 0; off < size; off += 4)
    {
        DWORD val = *(DWORD*)(p + off);

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery((void*)val, &mbi, sizeof(mbi)))
        {
            if ((mbi.State == MEM_COMMIT) &&
                (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
            {
                printf("[+] Method candidate @ offset 0x%02X -> %08X\n", off, val);
            }
        }
    }
}

DWORD WINAPI EntryThread(LPVOID)
{
    printf("iniciao");
    HMODULE hMod = GetModuleHandleA("vtkernel.dll");
    HMODULE hModb = GetModuleHandleA("Main.dll");
    HMODULE hModC = GetModuleHandleA("mp_Cliente.dll");
    if (!hMod) {
        return 0;
    }
    if (!hModb) {
        return 0;
    }
    printf("despues de hmod\n");
    GetNewObject getNewObject = (GetNewObject)GetProcAddress(hModC, "GetNewObject");
    GetTexturePath getTexturePath = (GetTexturePath)GetProcAddress(hMod, "?SetBoneRenderState@vtEngine@@QAEX_N@Z");
    SetBoneRenderState setBoneRenderState = (SetBoneRenderState)GetProcAddress(hMod, "?SetBoneRenderState@vtEngine@@QAEX_N@Z");
    AdvancedMenu_t advancedMenu = (AdvancedMenu_t)GetProcAddress(hModb, "?AdvancedMenu@MainMenu@@AAEXXZ");
    GetObjectList getObjectList = (GetObjectList)GetProcAddress(hMod, "?GetObjectList@CScene@@QAEAAV?$CArray@H@@XZ");
    GetObjectByName getObjectByName = (GetObjectByName)GetProcAddress(hMod, "?GetObjectByName@vtMachine@@QAEPBVCObject@@PBD@Z");
    LogRenderScene logRenderScene = (LogRenderScene)GetProcAddress(hMod, "?LogRenderScene@@YAXH@Z");
    GetActorList getActorList = (GetActorList)GetProcAddress(hMod, "?GetActorList@CScene@@QAEAAV?$CArray@H@@XZ");
    SetMeshRenderState setMeshRenderState = (SetMeshRenderState)GetProcAddress(hMod, "?SetMeshRenderState@vtEngine@@QAEX_N@Z");
    SetCBoxRenderState setCBoxRenderState = (SetCBoxRenderState)GetProcAddress(hMod, "?SetCBoxRenderState@vtEngine@@QAEX_N@Z");
    SetBBoxRenderState setBBoxRenderState = (SetBBoxRenderState)GetProcAddress(hMod, "?SetBBoxRenderState@vtEngine@@QAEX_N@Z");
    SetPause setPause = (SetPause)GetProcAddress(hMod, "?SetPause@vtEngine@@QAEX_N@Z");
    Reload reload = (Reload)GetProcAddress(hMod, "?Reload@vtEngine@@QAEXXZ");
    GetWorld getWorld =
        (GetWorld)GetProcAddress(hMod, "?GetWorld@@YAPAVvtWorld@@XZ");
    RemoveAllLights removeAllLights =
        (RemoveAllLights)GetProcAddress(hMod, "?RemoveAllLights@vtWorld@@QAEXXZ");
    ClearObjectList clearObjectList =
        (ClearObjectList)GetProcAddress(hMod, "?ClearObjectList@vtMachine@@QAEXXZ");
    EnableDither enableDither =
        (EnableDither)GetProcAddress(hMod, "?EnableDither@@YAX_N@Z");
    RemoveSceneObjects removeSceneObjects =
        (RemoveSceneObjects)GetProcAddress(hMod, "?RemoveSceneObjects@@YAXI@Z");

    vtFile_Length_t vtFile_Length =
    (vtFile_Length_t)GetProcAddress(hMod, "?vtFile_Length@@YAII@Z");

    vtFile_Open_t vtFile_Open = (vtFile_Open_t)GetProcAddress(
        hMod,
        "?vtFile_Open@@YAIPBDE@Z"
    );

    vtFile_Read_t vtFile_Read = (vtFile_Read_t)GetProcAddress(
        hMod,
        "?vtFile_Read@@YAIIPAXI@Z"
    );
    GetEngine getEngine = (GetEngine)GetProcAddress(
        hMod,
        "?GetEngine@@YAPAVvtEngine@@XZ"
    );
    SetCBoxRender setCBoxRender = (SetCBoxRender)GetProcAddress(
        hMod,
        "?SetCBoxRender@@YAX_N@Z"
    );
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
    VtEngine** ppEngine = (VtEngine**)GetProcAddress(
        hMod,
        "?renderEngine@@3PAVvtEngine@@A"
    );
    MainScene** ppMainScene = (MainScene**)GetProcAddress(hMod, "?mainScene@@3PAVvtWorld@@A");
    vtRenderScene** ppMainRenderScene = (vtRenderScene**)GetProcAddress(hMod, "?mainRenderScene@@3PAVvtRenderScene@@A");
    vtMachine** ppMachine = (vtMachine**)GetProcAddress(
        hMod,
        "?machine@@3PAVvtMachine@@A"
    );

        printf("setDebugMsgRender()\n");
        vtEnableLogs(TRUE); //habilita el volcado de logs en .txt

        vtGraph_DebugInfo_GetNumDrawPolys(TRUE);
        //vtGraph_TestCooperativeLevel();
        //logRenderScene((int)*ppMainRenderScene);
        vtGraph_TestMode(1, 0);
        //vtGraph_EnableFog();

        //vtGraph_SetCullMode(0);
        int numObjects = 0;

        numObjects = getNumObjects(*ppMachine);
        setBBoxRender(TRUE);
        setCBoxRender(TRUE);
        //enableDither(TRUE);
        setBBoxRenderState(*ppEngine, TRUE);
        setMeshRenderState(*ppEngine, TRUE);
        setCBoxRenderState(*ppEngine, TRUE);
        setBoneRenderState(*ppEngine, TRUE);
        char* texture_path = getTexturePath(*ppEngine);
        void* newobj = getNewObject();
        DumpMemory(newobj, 0x100);
        DumpMemory((void*)0x0D8E1348, 0x80);
        ScanForMethods(newobj, 0x400);

        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery((void*)0x0D8E1348, &mbi, sizeof(mbi));

        printf("Base=%p Size=%lx Protect=%lx State=%lx\n",
            mbi.BaseAddress,
            mbi.RegionSize,
            mbi.Protect,
            mbi.State);

        printf("texture path: %s\n", texture_path);
        //removeAllLights(getWorld());
        
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
        char* text_obj = (char*)"main"; 
        CObject* startpoint = getObjectByName(*ppMachine, text_obj);
        /*unsigned int id = *(int*)((char*)startpoint + 0x10);
        for (int off = 0x00; off <= 0x30; off += 4)
        {
            unsigned int v = *(unsigned int*)((char*)startpoint + off);
            printf("offset 0x%02X = 0x%08X (%u)\n", off, v, v);
        }
        const char* name = *(const char**)((char*)startpoint + 0x18);
        printf("name ptr: %p\n", (void*)name);
        if (name) {
            printf("name: %s\n", name);
        }
        for (int i = 0; i < 0x20; i += 4)
        {
            printf("0x%02X = 0x%08X\n", i,
                *(unsigned int*)((char*)texture_path + i));
        }*/
        /*
        CObject* startpoint_id = getObjectById(*ppMachine, id);
        unsigned int id_get_by_id = *(int*)((char*)startpoint_id + 0x10);
        const char* name_id = *(const char**)((char*)startpoint_id + 0x18);
       //int id_get_by_id =  *(int*)((char*)obj + 0x10);
        printf("id del objeto  %d\n" ,id);
        printf("id del objeto pero por id: %d\n", id_get_by_id);
        printf("name_id ptr: %p\n", (void*)name_id);
        printf("name_id: %s\n", name);

        MainScene* scene = *ppMainScene;
        char* base = (char*)scene;
        void* objectList = base + 0x54;
        char* arr = (char*)objectList;
        int raw0 = *(int*)(arr + 0x00);
        int raw4 = *(int*)(arr + 0x04);
        int raw8 = *(int*)(arr + 0x08);
        int rawC = *(int*)(arr + 0x0C);

        printf("raw +0x00: 0x%08X\n", raw0);
        printf("raw +0x04: 0x%08X\n", raw4);
        printf("raw +0x08: 0x%08X\n", raw8);
        printf("raw +0x0C: 0x%08X\n", rawC);
        int* pData = *(int**)arr;
        int nSize  = *(int*)(arr + 0x04);
        int nMax   = *(int*)(arr + 0x08);

        printf("pData=%p size=%d max=%d\n", pData, nSize, nMax);*/

        //MainMenu** ppMainMenu = (MainMenu**)GetProcAddress(hMod, "?g_MainMenu@@3PAVMainMenu@@A");
        //void* mainMenu = *ppMainMenu; // puntero real al objeto MainMenu



        // Llamada a AdvancedMenu

        //advancedMenu(mainMenu);
        /*unsigned char flags = 0x01 | 0x80; // read + binary
        unsigned int fd = 0;
        fd = vtFile_Open("./nfos/Bala.NFO", flags);
        unsigned int file_length =  vtFile_Length(fd);
        printf(" _filelength vt_file: %d\n", file_length);
        char* buffer = (char*)malloc(file_length+1);
        unsigned int bytes = 0;
        bytes = _read(fd, buffer, file_length);
        _close(fd);
        buffer[bytes] = '\0';
        FILE* f = fopen("./dump_mplayer.txt", "wt");
        fwrite(buffer, 1, bytes, f);
        fclose(f);*/

        //setPause(*ppEngine, TRUE);
        //reload(*ppEngine);
        //getActorList((int)startpoint);
        //printf("Pointer first actor: \n");
        
        printf("Numero de objetos: %d\n", numObjects);
        //removeSceneObjects(numObjects);
        for (int i = 2; i < numObjects- 40; i++) {
            printf("objeto nº: %d Borrado\n", i);
            //setDebugMsgRender(TRUE);
            /*CObject* obj = nullptr;
            obj = getObjectById(*ppMachine, i);
            vtDeleteObject(*ppMachine, obj);*/
            
            Sleep(200);
        }
        
        numObjects = getNumObjects(*ppMachine);
        printf("Numero de objetos-despues: %d\n", numObjects);
        //setPause(*ppEngine, FALSE);
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


