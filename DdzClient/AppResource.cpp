//
// File: AppResource.cpp
//
//  处理用户头像图标等资源
//
#include "stdafx.h"
#include <olectl.h>
#include "AppResource.h"

// IMAGE LIST
HIMAGELIST  g_himlUser16x16 = NULL;
HIMAGELIST  g_himlUser32x32 = NULL;
HIMAGELIST  g_himlServerTree = NULL;
HIMAGELIST  g_himlTabCtrl = NULL;
HIMAGELIST  g_himlGameInfo = NULL;

HBITMAP     g_hbmpTable = NULL;
HBITMAP     g_hbmpTableStart = NULL;
HICON       g_hiconReady = NULL;

HICON       g_hiconLord = NULL;
HICON       g_hiconBomb = NULL;
HICON       g_hiconDelegated = NULL;
HICON       g_hiconConnLost = NULL;
HICON       g_hiconClock = NULL;

HBITMAP     g_hbmpGameBG = NULL;
HBITMAP     g_hbmpPoker = NULL;
HBITMAP     g_hbmpClockDigital = NULL;

// sound dll
HMODULE     g_hSndModule = NULL;


//
// Load GIF/JPEG/BMP image from file.
// If operation succeeds, valid HBITMAP returned, else NULL.
// It's recommended that caller should use DeleteObject() to release the image
// resources when no long used, although system will free them when process exits.
//
HBITMAP LoadCommonImage(LPCTSTR szFileName)
{
    IStream*    iStream = NULL;
    IPicture*   iPicture = NULL;

    HANDLE      hFile = NULL;
    HGLOBAL     hGlobal = NULL;

    LPVOID      pData = NULL;
    DWORD       nFileSize = 0;
    DWORD       nReadBytes = 0;

    hFile = CreateFile(szFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, _T("Read image file failed."), _T("ERROR"), MB_OK | MB_ICONERROR);
        return NULL;
    }

    nFileSize = GetFileSize(hFile, NULL);

    hGlobal = GlobalAlloc(GMEM_MOVEABLE, nFileSize);
    if (hGlobal == NULL) {
        CloseHandle(hFile);
        return NULL;
    }

    pData = GlobalLock(hGlobal);

    if (ReadFile(hFile, pData, nFileSize, &nReadBytes, NULL) == 0) {
        GlobalFree(hGlobal);
        CloseHandle(hFile);
        return NULL;
    }

    if (CreateStreamOnHGlobal(hGlobal, FALSE, &iStream) != S_OK) {
        GlobalFree(hGlobal);
        CloseHandle(hFile);
        return NULL;
    }

    if (OleLoadPicture(iStream,0,false,IID_IPicture,(void**)&iPicture) != S_OK) {
        iStream->Release();
        GlobalFree(hGlobal);
        CloseHandle(hFile);
        return NULL;
    }

    HBITMAP hBmp = 0;
    iPicture->get_Handle((OLE_HANDLE*)&hBmp);

    HBITMAP hRetBmp = (HBITMAP)CopyImage(hBmp, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);

    GlobalFree(hGlobal);
    CloseHandle(hFile);
    iStream->Release();
    iPicture->Release();

    return hRetBmp;
}

//
// Load GIF/JPEG/BMP image from resource.
// If operation succeeds, valid HBITMAP returned, else NULL.
// It's recommended that caller should use DeleteObject() to release the image
// resources when no long used, although system will free them when process exits.
//
HBITMAP LoadCommonImage(HMODULE hModule, LPCTSTR szResName, LPCTSTR szResType)
{
    IStream*    iStream = NULL;
    IPicture*   iPicture = NULL;

    HRSRC hResInfo = FindResource(hModule, szResName, szResType);
    if (hResInfo == NULL) {
        MessageBox(NULL, _T("Find image from resource failed."), _T("ERROR"), MB_OK | MB_ICONERROR);
        return NULL;
    }

    HGLOBAL hResData = LoadResource(hModule, hResInfo);
    if (hResData == NULL) {
        return NULL;
    }

    DWORD dwSize = SizeofResource(hModule, hResInfo);

    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, dwSize);
    if (hGlobal == NULL) {
        FreeResource(hResData);
        return NULL;
    }

    LPVOID pDest = (LPVOID)GlobalLock(hGlobal);
    LPVOID pSrc = (LPVOID)LockResource(hResData);

    if ((pSrc == NULL) || (pDest == NULL)) {
        GlobalFree(hGlobal);
        FreeResource(hResData);
        return FALSE;
    }

    CopyMemory(pDest, pSrc, dwSize);

    if (CreateStreamOnHGlobal(hGlobal, FALSE, &iStream) != S_OK) {
        GlobalFree(hGlobal);
        FreeResource(hResData);
        return NULL;
    }

    if (OleLoadPicture(iStream,0,false,IID_IPicture,(void**)&iPicture) != S_OK) {
        iStream->Release();
        GlobalFree(hGlobal);
        FreeResource(hResData);
        return NULL;
    }

    HBITMAP hBmp = NULL;
    iPicture->get_Handle((OLE_HANDLE*)&hBmp);

    HBITMAP hRetBmp = (HBITMAP)CopyImage(hBmp, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);

    GlobalFree(hGlobal);
    FreeResource(hResData);
    iStream->Release();
    iPicture->Release();

    return hRetBmp;
}

// 加载程序中使用的IMAGE资源
BOOL LoadAppImageRes(void)
{
    HMODULE     hModule = NULL;
    HICON       hicon = NULL;
    HBITMAP     hbmp  = NULL;

#define STOP_IF_NULL_HANDLE(_handle) if (_handle == NULL) { goto FAILURE; }

    g_himlUser16x16 = ImageList_Create(CX_USER_ICON_SML, CY_USER_ICON_SML,
        ILC_COLOR32 | ILC_MASK, USER_ICON_INIT_NUM, USER_ICON_GROW_NUM);
    STOP_IF_NULL_HANDLE(g_himlUser16x16);

    g_himlUser32x32 = ImageList_Create(CX_USER_ICON_BIG, CY_USER_ICON_BIG,
        ILC_COLOR32 | ILC_MASK, USER_ICON_INIT_NUM, USER_ICON_GROW_NUM);
    STOP_IF_NULL_HANDLE(g_himlUser32x32);

    g_himlServerTree = ImageList_Create(CX_SERVER_TREE_ICON, CY_SERVER_TREE_ICON, 
        ILC_COLOR32, 4, 0);
    STOP_IF_NULL_HANDLE(g_himlServerTree);

    g_himlTabCtrl = ImageList_Create(CX_TAB_CTRL_ICON, CY_TAB_CTRL_ICON,
        ILC_COLOR32, 4, 0);
    STOP_IF_NULL_HANDLE(g_himlTabCtrl);

    g_himlGameInfo = ImageList_Create(CX_GAME_INFO_IMAGE, CY_GAME_INFO_IMAGE,
        ILC_COLOR32 | ILC_MASK, 8, 0);
    STOP_IF_NULL_HANDLE(g_himlGameInfo);

//==============================================================================
    hModule = LoadLibrary(RES_IMAGE_DLL_NAME);
    STOP_IF_NULL_HANDLE(hModule);

    // g_ghimlUser16x16
    for (int id = IDB_USERHEAD_16_FIRST; id <= IDB_USERHEAD_16_LAST; id++) {
        hbmp = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(id), IMAGE_BITMAP,
            0, 0, LR_DEFAULTCOLOR);
        STOP_IF_NULL_HANDLE(hbmp);

        ImageList_AddMasked(g_himlUser16x16, hbmp, DEF_IMG_MASK_CLR);
        DeleteObject(hbmp);
    }

    //
    // 将玩家状态图标存放在16x16头像末尾
    //
    for (int id = IDB_STATE_IDLE; id <= IDB_STATE_LOOKON; id++) {
        hbmp = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(id), IMAGE_BITMAP,
            0, 0, LR_DEFAULTCOLOR);
        STOP_IF_NULL_HANDLE(hbmp);

        ImageList_AddMasked(g_himlUser16x16, hbmp, DEF_IMG_MASK_CLR);
        DeleteObject(hbmp);
    }

    // g_ghimlUser32x32
    for (int id = IDB_USERHEAD_32_FIRST; id <= IDB_USERHEAD_32_LAST; id++) {
        hbmp = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(id), IMAGE_BITMAP,
            0, 0, LR_DEFAULTCOLOR);
        STOP_IF_NULL_HANDLE(hbmp);

        ImageList_AddMasked(g_himlUser32x32, hbmp, DEF_IMG_MASK_CLR);
        DeleteObject(hbmp);
    }

    //
    // g_himlServerTree
    //
    hicon = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_SERVER),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(hicon);
    ImageList_AddIcon(g_himlServerTree, hicon);
    DestroyIcon(hicon);

    hicon = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_ROOM),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(hicon);
    ImageList_AddIcon(g_himlServerTree, hicon);
    DestroyIcon(hicon);

    //
    // g_himlTabCtrl
    //
    hicon = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_HOME),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(hicon);
    ImageList_AddIcon(g_himlTabCtrl, hicon);
    DestroyIcon(hicon);

    hicon = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_ROOM),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(hicon);
    ImageList_AddIcon(g_himlTabCtrl, hicon);
    DestroyIcon(hicon);

    hicon = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_GAME),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(hicon);
    ImageList_AddIcon(g_himlTabCtrl, hicon);
    DestroyIcon(hicon);

    //
    // Game room window
    //
    g_hbmpTable = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(IDB_GAME_TABLE),
        IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hbmpTable);

    g_hbmpTableStart = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(IDB_GAME_TABLE_START),
        IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hbmpTableStart);

    g_hiconReady = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_GAMER_READY),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hiconReady);

    //
    // Game Main Window
    //
    g_hiconLord = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_LORD),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hiconLord);

    g_hiconBomb = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_BOMB),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hiconBomb);

    g_hiconDelegated = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_DELEGATED),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hiconDelegated);

    g_hiconConnLost = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_DISCONNECT),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hiconConnLost);

    g_hiconClock = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_CLOCK),
        IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hiconClock);

    FreeLibrary(hModule);

//==============================================================================
    //
    // 从扑克牌资源DLL中加载扑克相关资源
    //
    hModule = LoadLibrary(RES_POKER_DLL_NAME);
    STOP_IF_NULL_HANDLE(hModule);
    
    //
    // Game background image
    //
    // If the app folder contains a bmp file named GAME_BG_DEF_BMP_FILE, then
    // it's used as game background image. Else, if there is a section in the
    // INI file, which indicates the game background image, it's used. Otherwise,
    // the image from resource DLL will be used.
    //
    g_hbmpGameBG = (HBITMAP)LoadImage(NULL, GAME_BG_DEF_BMP_FILE, IMAGE_BITMAP,
        0, 0, LR_LOADFROMFILE | LR_DEFAULTCOLOR);

    if (g_hbmpGameBG == NULL) {
        TCHAR szBmpFile[32];
        TCHAR szIniFile[MAX_PATH];

        GetModuleFileName(NULL, szIniFile, sizeof(szIniFile) / sizeof(szIniFile[0]));
        PathRemoveFileSpec(szIniFile);
        _tcscat_s(szIniFile, sizeof(szIniFile) / sizeof(szIniFile[0]), _T("\\"));
        _tcscat_s(szIniFile, sizeof(szIniFile) / sizeof(szIniFile[0]), DDZ_CLIENT_INI_FILE);

        GetPrivateProfileString(DDZ_GAME_SEC, GAME_KEY_BG_IMAGE, NULL, szBmpFile,
            sizeof(szBmpFile) / sizeof(szBmpFile[0]), szIniFile);

        if (_tcslen(szBmpFile) > 0) {
            g_hbmpGameBG = (HBITMAP)LoadImage(NULL, szBmpFile, IMAGE_BITMAP,
                0, 0, LR_LOADFROMFILE | LR_DEFAULTCOLOR);
        }
    }

    if (g_hbmpGameBG == NULL) {
        g_hbmpGameBG = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(IDB_BACKGROUND),
            IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        STOP_IF_NULL_HANDLE(g_hbmpGameBG);
    }

    g_hbmpPoker = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(IDB_POKER_CARDS),
        IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hbmpPoker);

    g_hbmpClockDigital = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(IDB_CLOCK_DIGITAL),
        IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    STOP_IF_NULL_HANDLE(g_hbmpClockDigital);

    //
    // Load GAME_STATUS_INFO images.
    // The image id should be assigned as [READY, 0分, 1分, 2分, 3分, PASS]
    //
    for (int i = IDB_INFO_READY; i <= IDB_INFO_PASS; i++) {
        hbmp = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(i), IMAGE_BITMAP,
            0, 0, LR_DEFAULTCOLOR);
        STOP_IF_NULL_HANDLE(hbmp);

        ImageList_AddMasked(g_himlGameInfo, hbmp, GAME_INFO_MASK_CLR);
        DeleteObject(hbmp);
    }

    FreeLibrary(hModule);
    return TRUE;

//==============================================================================
FAILURE:
    if (hModule != NULL) { FreeLibrary(hModule); }
    DestroyAppImageRes();
    return FALSE;
}

// 清理程序使用的IMAGE资源
void DestroyAppImageRes(void)
{
#define DESTROY_IMAGELIST(_imglist) \
    if (_imglist != NULL) { ImageList_Destroy(_imglist); _imglist = NULL; }

    DESTROY_IMAGELIST(g_himlUser16x16);
    DESTROY_IMAGELIST(g_himlUser32x32);
    DESTROY_IMAGELIST(g_himlServerTree);
    DESTROY_IMAGELIST(g_himlTabCtrl);
    DESTROY_IMAGELIST(g_himlGameInfo);

#define DELETE_IMAGEOBJECT(_imgobj) \
    if (_imgobj != NULL) { DeleteObject(_imgobj); _imgobj = NULL; }

    DELETE_IMAGEOBJECT(g_hbmpTable);
    DELETE_IMAGEOBJECT(g_hbmpTableStart);

    DELETE_IMAGEOBJECT(g_hbmpGameBG);
    DELETE_IMAGEOBJECT(g_hbmpPoker);

    DELETE_IMAGEOBJECT(g_hbmpClockDigital);

#define DESTROY_ICON(_hicon) \
    if (_hicon != NULL) { DestroyIcon(_hicon); _hicon = NULL; }

    DESTROY_ICON(g_hiconReady);
    DESTROY_ICON(g_hiconLord);
    DESTROY_ICON(g_hiconBomb);
    DESTROY_ICON(g_hiconDelegated);
    DESTROY_ICON(g_hiconConnLost);
    DESTROY_ICON(g_hiconClock);
}

BOOL LoadAppSoundRes(void)
{
    if (g_hSndModule == NULL) {
        g_hSndModule = LoadLibrary(RES_SOUND_DLL_NAME);
    }

    return (g_hSndModule != NULL);
}

void FreeAppSoundRes(void)
{
    if (g_hSndModule != NULL) {
        FreeLibrary(g_hSndModule);
        g_hSndModule = NULL;
    }
}

void PlayGameSound(SND_TYPE sndType)
{
    if (g_hSndModule == NULL) {
        TRACE(_T("The sound module not loaded.\n"));
        return;
    }

#define PLAY_GAME_SOUND(_resid) \
    PlaySound(MAKEINTRESOURCE(_resid), g_hSndModule, SND_ASYNC | SND_RESOURCE);

#define PLAY_GAME_SOUND_BREAK(_sndtype, _resid) \
    case _sndtype: { PLAY_GAME_SOUND(_resid); break; }

    switch (sndType) {
        PLAY_GAME_SOUND_BREAK(  SND_GAME_OUTPUT_CARD,   IDR_WAV_OUTCARD);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_BOMB,          IDR_WAV_BOMB);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_START,         IDR_WAV_START);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_WIN,           IDR_WAV_WIN);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_RUNAWAY,       IDR_WAV_RUNAWAY);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_TIMEOUT,       IDR_WAV_TIMEOUT);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_SCORE0_M,      IDR_WAV_0_MALE);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_SCORE1_M,      IDR_WAV_1_MALE);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_SCORE2_M,      IDR_WAV_2_MALE);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_SCORE3_M,      IDR_WAV_3_MALE);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_SCORE0_F,      IDR_WAV_0_FEMALE);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_SCORE1_F,      IDR_WAV_1_FEMALE);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_SCORE2_F,      IDR_WAV_2_FEMALE);
        PLAY_GAME_SOUND_BREAK(  SND_GAME_SCORE3_F,      IDR_WAV_3_FEMALE);
    }
}

BOOL LoadAppResource(void)
{
    if (!LoadAppImageRes()) {
        return FALSE;
    }

    if (!LoadAppSoundRes()) {
        DestroyAppImageRes();
        return FALSE;
    }

    return TRUE;
}

void FreeAppResource(void)
{
    DestroyAppImageRes();
    FreeAppSoundRes();
}