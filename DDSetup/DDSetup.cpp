#include "framework.h"
#include "resource.h"
#include "SplashScreen.h"
#include <shlobj.h>
#include <strsafe.h>
#include <wrl.h>

extern "C"
{
#include "7z\7z.h"
#include "7z\7zAlloc.h"
#include "7z\7zCrc.h"
}

UINT g_dpi;

void InitialUpdateScale()
{
    HDC screen = GetDC(nullptr);
    g_dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(nullptr, screen);
}

int GetCurrentScaleInterval()
{
    if (g_dpi >= 384) return 6;
    if (g_dpi >= 288) return 5;
    if (g_dpi >= 240) return 4;
    if (g_dpi >= 192) return 3;
    if (g_dpi >= 144) return 2;
    if (g_dpi >= 120) return 1;
    return 0;
}

enum ExtractResult
{
    ER_OK = 0,
    ER_UNKNOWN = -1,
    ER_NORESOURCE = -2,
    ER_INVALID = -3,
    ER_UNSUPPORTED = -4,
    ER_OUTOFMEMORY = -5,
    ER_INVALIDPATH = -6,
    ER_CREATEDIRFAILED = -7,
    ER_CREATEFILEFAILED = -8,
    ER_WRITEFAILED = -9,
    ER_READFAILED = -10,
};

struct ResourceStream
{
    ISeekInStream stream{};
    const Byte* data = nullptr;
    ULONGLONG size = 0;
    ULONGLONG position = 0;
};

SRes ResourceStream_Read(ISeekInStreamPtr p, void* buf, size_t* size)
{
    ResourceStream* self = (ResourceStream*)p;
    if (!size)
        return SZ_ERROR_PARAM;
    ULONGLONG requested = *size;
    if (self->position >= self->size)
    {
        *size = 0;
        return SZ_OK;
    }
    ULONGLONG available = self->size - self->position;
    if (requested > available)
        requested = available;
    memcpy(buf, self->data + self->position, requested);
    self->position += requested;
    *size = requested;
    return SZ_OK;
}


SRes ResourceStream_Seek(ISeekInStreamPtr p, Int64* pos, ESzSeek origin)
{
    ResourceStream* self = (ResourceStream*)p;
    if (!pos)
        return SZ_ERROR_PARAM;
    LONGLONG base = 0;
    switch (origin)
    {
    case SZ_SEEK_SET:
        base = 0;
        break;
    case SZ_SEEK_CUR:
        base = static_cast<LONGLONG>(self->position);
        break;
    case SZ_SEEK_END:
        base = static_cast<LONGLONG>(self->size);
        break;
    default:
        return SZ_ERROR_PARAM;
    }
    const LONGLONG newPosition = base + *pos;
    if (newPosition < 0 || static_cast<ULONGLONG>(newPosition) > self->size)
        return SZ_ERROR_PARAM;
    self->position = static_cast<size_t>(newPosition);
    *pos = newPosition;
    return SZ_OK;
}


void ResourceStream_Init(ResourceStream& stream, const void* data, size_t size)
{
    memset(&stream, 0, sizeof(stream));
    stream.data = static_cast<const Byte*>(data);
    stream.size = size;
    stream.stream.Read = ResourceStream_Read;
    stream.stream.Seek = ResourceStream_Seek;
}

bool LoadResourceData(HINSTANCE hInstance, int iResourceId, void** ppvData, DWORD* dwSize)
{
    HRSRC resource = FindResourceW(hInstance, MAKEINTRESOURCEW(iResourceId), RT_RCDATA);
    if (!resource) return false;
    const DWORD resourceSize = SizeofResource(hInstance, resource);
    if (resourceSize == 0) return false;
    HGLOBAL loaded = LoadResource(hInstance, resource);
    if (!loaded) return false;
    void* resourceData = LockResource(loaded);
    if (!resourceData) return false;
    *ppvData = resourceData;
    *dwSize = resourceSize;
    return true;
}

HRESULT GetArchiveFilename(const CSzArEx& db, UInt32 index, LPWSTR pszName, UINT cch)
{
    ULONGLONG count = SzArEx_GetFileNameUtf16(&db, index, nullptr);
    if (count == 0)
        return E_FAIL;
    if (count > cch)
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    SzArEx_GetFileNameUtf16(&db, index, pszName);
    return S_OK;
}

bool IsSafeArchivePath(LPWSTR pszPath)
{
    const ULONGLONG strLength = wcslen(pszPath);
    if (strLength == 0) return false;
    for (int i = 0; i < strLength; i++)
    {
        if (pszPath[i] == L'/')
            pszPath[i] = L'\\';
    }
    if (pszPath[0] == L'\\') return false;
    if (strLength >= 2 && pszPath[1] == L':')
        return false;

    ULONGLONG start = 0;
    while (start < strLength)
    {
        ULONGLONG idxSlash = start;
        while (pszPath[idxSlash] != '\\' && idxSlash < strLength)
            idxSlash++;
        if (idxSlash >= strLength)
            break;
        if (pszPath[idxSlash + 1] == L'.' && pszPath[idxSlash + 2] == L'.')
            return false;
        start = idxSlash + 1;
    }
    return true;
}

ExtractResult ConvertSdkError(SRes result)
{
    switch (result)
    {
    case SZ_OK:
        return ER_OK;
    case SZ_ERROR_MEM:
        return ER_OUTOFMEMORY;
    case SZ_ERROR_UNSUPPORTED:
        return ER_UNSUPPORTED;
    case SZ_ERROR_INPUT_EOF:
    case SZ_ERROR_READ:
        return ER_READFAILED;
    case SZ_ERROR_ARCHIVE:
    case SZ_ERROR_NO_ARCHIVE:
        return ER_INVALID;
    default:
        return ER_UNKNOWN;
    }
}

bool EnsureDirectoryExists(LPCWSTR pszPath)
{
    if (!pszPath || wcslen(pszPath) == 0)
        return false;
    DWORD attrs = GetFileAttributesW(pszPath);
    if (attrs != INVALID_FILE_ATTRIBUTES)
        return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    DWORD dwSeparator = wcslen(pszPath);
    while (dwSeparator > 0)
    {
        dwSeparator--;
        if (pszPath[dwSeparator] == L'\\' || pszPath[dwSeparator] == L'/')
            break;
    }
    if (dwSeparator == 0)
    {
        return CreateDirectoryW(pszPath, nullptr) ||
            GetLastError() == ERROR_ALREADY_EXISTS;
    }
    if (dwSeparator == 2 && pszPath[1] == L':')
        return true;

    WCHAR pszParent[260];
    StringCchCopyW(pszParent, dwSeparator + 1, pszPath);

    if (!EnsureDirectoryExists(pszParent))
        return false;
    if (CreateDirectoryW(pszPath, nullptr))
        return true;
    if (GetLastError() != ERROR_ALREADY_EXISTS)
        return false;
    attrs = GetFileAttributesW(pszPath);
    return attrs != INVALID_FILE_ATTRIBUTES &&
        (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

ExtractResult ExtractEmbedded7z(HINSTANCE hInstance, int iResourceId, LPCWSTR pszDest)
{
    void* archiveData = nullptr;
    DWORD archiveSize = 0;
    if (!LoadResourceData(hInstance, iResourceId, &archiveData, &archiveSize))
        return ER_NORESOURCE;
    ResourceStream input;
    ResourceStream_Init(input, archiveData, static_cast<size_t>(archiveSize));
    CLookToRead2 lr2{};
    Byte buf[16384];
    LookToRead2_CreateVTable(&lr2, 0);
    lr2.buf = buf;
    lr2.bufSize = 16384;
    lr2.realStream = &input.stream;
    LookToRead2_INIT(&lr2);

    ISzAlloc allocMain =
    {
        SzAlloc,
        SzFree
    };
    ISzAlloc allocTemp =
    {
        SzAllocTemp,
        SzFreeTemp
    };

    CrcGenerateTable();

    CSzArEx db;
    SzArEx_Init(&db);
    SRes result = SzArEx_Open(&db, &lr2.vt, &allocMain, &allocTemp);
    if (result != SZ_OK)
    {
        const ExtractResult error = ConvertSdkError(result);
        SzArEx_Free(&db, &allocMain);
        return error;
    }
    if (!EnsureDirectoryExists(pszDest))
    {
        SzArEx_Free(&db, &allocMain);
        return ER_CREATEDIRFAILED;
    }

    UInt32 blockIndex = static_cast<UInt32>(-1);
    Byte* outBuffer = nullptr;
    size_t outBufferSize = 0;
    ExtractResult finalResult = ExtractResult::ER_OK;
    for (UInt32 index = 0; index < db.NumFiles; ++index)
    {
        const bool isDirectory = SzArEx_IsDir(&db, index) != 0;
        WCHAR pszArchiveName[260];
        GetArchiveFilename(db, index, pszArchiveName, 260);
        if (!IsSafeArchivePath(pszArchiveName))
        {
            finalResult = ER_INVALIDPATH;
            break;
        }

        for (int i = 0; i < wcslen(pszArchiveName); i++)
        {
            if (pszArchiveName[i] == L'/')
                pszArchiveName[i] = L'\\';
        }

        WCHAR pszArchive[260];
        StringCchPrintfW(pszArchive, 260, L"%s\\%s", pszDest, pszArchiveName);
        if (isDirectory)
        {
            if (!EnsureDirectoryExists(pszArchive))
                finalResult = ER_CREATEDIRFAILED;
            continue;
        }
        DWORD dwSeparator = wcslen(pszArchive);
        while (dwSeparator > 0)
        {
            dwSeparator--;
            if (pszArchive[dwSeparator] == L'\\' || pszArchive[dwSeparator] == L'/')
                break;
        }
        WCHAR pszParent[260];
        StringCchCopyW(pszParent, dwSeparator + 1, pszArchive);
        if (!EnsureDirectoryExists(pszParent))
        {
            finalResult = ER_CREATEDIRFAILED;
            break;
        }
        ULONGLONG offset = 0;
        ULONGLONG processed = 0;
        result = SzArEx_Extract(&db, &lr2.vt, index, &blockIndex, &outBuffer, &outBufferSize, &offset, &processed, &allocMain, &allocTemp);
        if (result != SZ_OK)
        {
            finalResult = ConvertSdkError(result);
            break;
        }

        HANDLE hFile = CreateFileW(pszArchive, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            finalResult = ER_CREATEFILEFAILED;
            break;
        }

        const Byte* source = outBuffer + offset;
        bool writeSucceeded = true;
        size_t remaining = processed;
        while (remaining != 0)
        {
            const DWORD chunk = static_cast<DWORD>(remaining > 0xFFFFFFFF ? 0xFFFFFFFF : remaining);
            DWORD written = 0;
            if (!WriteFile(hFile, source, chunk, &written, nullptr))
            {
                writeSucceeded = false;
                break;
            }
            if (written == 0)
            {
                writeSucceeded = false;
                break;
            }
            source += written;
            remaining -= written;
        }
        CloseHandle(hFile);
        if (!writeSucceeded)
        {
            finalResult = ER_WRITEFAILED;
            break;
        }
    }

    if (outBuffer)
        allocMain.Free(&allocMain, outBuffer);
    
    SzArEx_Free(&db, &allocMain);
    return finalResult;
}

HRESULT PerformDeleteOp(LPCWSTR destDir)
{
    if (!destDir || wcslen(destDir) == 0) return E_INVALIDARG;

    Microsoft::WRL::ComPtr<IShellItem> psiFolder;
    HRESULT hr = SHCreateItemFromParsingName(destDir, nullptr, IID_PPV_ARGS(&psiFolder));
    if (SUCCEEDED(hr))
    {
        Microsoft::WRL::ComPtr<IFileOperation> pfo;
        hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfo));
        if (SUCCEEDED(hr))
        {
            pfo->SetOperationFlags(0x0614);
                hr = pfo->DeleteItem(psiFolder.Get(), nullptr);
            pfo->PerformOperations();
        }
    }
    return hr;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_WAIT);
    wc.lpszClassName = L"DD_SetupSplash";
    RegisterClass(&wc);
    HWND hWndOwner = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW, wc.lpszClassName, NULL, WS_POPUP | WS_VISIBLE, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    InitialUpdateScale();
    HBITMAP hbmSplash{};
    LoadPNGAsBitmap(hInstance, hbmSplash, 301 + GetCurrentScaleInterval());
    SetSplashImage(hWndOwner, hbmSplash);
    Sleep(250);
    WCHAR pszTemp[260];
    GetTempPathW(260, pszTemp);
    WCHAR pszDirInTemp[260];
    StringCchPrintfW(pszDirInTemp, 260, L"%s%s", pszTemp, L"DirectDesktop");
    ExtractResult t = ExtractEmbedded7z(hInstance, IDR_PAYLOAD, pszDirInTemp);
    WCHAR pszApp[260];
    StringCchPrintfW(pszApp, 260, L"%s\\%s", pszDirInTemp, L"InstallDD.exe");
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessW(nullptr, pszApp, nullptr, nullptr, FALSE, NULL, nullptr, pszDirInTemp, &si, &pi))
    {
        WCHAR pszErr[260];
        StringCchPrintfW(pszErr, 260, L"0x%X", GetLastError());
        MessageBoxW(NULL, L"Cannot find executable.", pszErr, MB_ICONERROR);
        return 1;
    }
    DeleteObject(hbmSplash);
    SetWindowPos(hWndOwner, NULL, 0, 0, 0, 0, NULL);
    WaitForSingleObject(pi.hProcess, INFINITE);
    PerformDeleteOp(pszDirInTemp);
    return 0;
}