#include <ShlObj_core.h>
#include <ShlGuid.h>
#include "ContextMenus.h"
#include "DirectoryHelper.h"
#include "resource.h"

std::wstring RemoveQuotes2(const std::wstring& input) {
    if (input.size() >= 2 && input.front() == L'\"' && input.back() == L'\"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

int validItems;
void DesktopRightClick(Element* elem, Event* iev) {
    if (iev->uidType == Button::Context) {

        IShellView* pShellView = NULL;
        IShellFolder* pShellFolder = NULL;

        HRESULT hr = SHGetDesktopFolder(&pShellFolder);
        pShellFolder->CreateViewObject(GetShellWindow(), IID_PPV_ARGS(&pShellView));

        LPCONTEXTMENU3 pICv1 = NULL;
        pShellView->GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu3, (LPVOID*)&pICv1);
        if (pICv1)
        {
            HMENU hm = CreatePopupMenu();
            HMENU hsm = CreatePopupMenu();
            MENUITEMINFOW mii{};
            mii.cbSize = sizeof(MENUITEMINFOW);
            mii.fMask = MIIM_STATE;
            AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 1001, LoadStrFromRes(4004).c_str());
            AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 1002, LoadStrFromRes(4005).c_str());
            AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 1003, LoadStrFromRes(4006).c_str());
            AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 1004, LoadStrFromRes(4007).c_str());
            for (int menuitem = 1001; menuitem <= 1004; menuitem++) {
                mii.fState = MFS_UNCHECKED;
                SetMenuItemInfoW(hsm, menuitem, 0, &mii);
            }
            mii.fState = MFS_CHECKED;
            if (globaliconsz <= 32) SetMenuItemInfoW(hsm, 1004, 0, &mii);
            else if (globaliconsz <= 48) SetMenuItemInfoW(hsm, 1003, 0, &mii);
            else if (globaliconsz <= 96) SetMenuItemInfoW(hsm, 1002, 0, &mii);
            else SetMenuItemInfoW(hsm, 1001, 0, &mii);
            AppendMenuW(hsm, MF_SEPARATOR, 1005, L"_");
            AppendMenuW(hsm, MF_STRING, 1006, LoadStrFromRes(4008).c_str());
            mii.fState = hiddenIcons ? MFS_UNCHECKED : MFS_CHECKED;
            SetMenuItemInfoW(hsm, 1006, 0, &mii);
            InsertMenuW(hm, 0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hsm, LoadStrFromRes(4001).c_str());
            InsertMenuW(hm, 1, MF_BYPOSITION | MF_STRING, 2002, LoadStrFromRes(4002).c_str());
            InsertMenuW(hm, 2, MF_BYPOSITION | MF_STRING, 2003, LoadStrFromRes(4003).c_str());
            InsertMenuW(hm, 3, MF_BYPOSITION | MF_SEPARATOR, 2004, L"_");
            pICv1->QueryContextMenu(hm, 4, MIN_SHELL_ID, MAX_SHELL_ID, CMF_EXPLORE);

            int itemCount = GetMenuItemCount(hm);
            for (int i = 0; i < itemCount; i++) {
                MENUITEMINFO mii = { 0 };
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_ID;
                if (GetMenuItemInfoW(hm, i, TRUE, &mii)) {
                    if (mii.wID == 2004) {
                        for (int j = 0; j < 5; j++) RemoveMenu(hm, i, MF_BYPOSITION);
                        break;
                    }
                }
            }

            UINT uFlags = TPM_RIGHTBUTTON;
            if (localeType == 1) uFlags |= TPM_LAYOUTRTL;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
                uFlags |= TPM_RIGHTALIGN;
            else
                uFlags |= TPM_LEFTALIGN;

            // Use TPM_RETURNCMD flag let TrackPopupMenuEx function return the menu item identifier of the user's selection in the return value.
            uFlags |= TPM_RETURNCMD;

            POINT pt;
            GetCursorPos(&pt);
            int menuItemId = TrackPopupMenuEx(hm, uFlags, pt.x, pt.y, wnd->GetHWND(), NULL);
            switch (menuItemId) {
            case 2002:
                InitLayout(false, false);
                break;
            case 2003:
                ShowSimpleView();
                break;
            case 1001:
                globaliconsz = 144;
                globalshiconsz = 48;
                globalgpiconsz = 48;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize", 144, false, nullptr);
                RearrangeIcons(true, true);
                break;
            case 1002:
                globaliconsz = 96;
                globalshiconsz = 48;
                globalgpiconsz = 32;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize", 96, false, nullptr);
                RearrangeIcons(true, true);
                break;
            case 1003:
                globaliconsz = 48;
                globalshiconsz = 32;
                globalgpiconsz = 16;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize", 48, false, nullptr);
                RearrangeIcons(true, true);
                break;
            case 1004:
                globaliconsz = 32;
                globalshiconsz = 32;
                globalgpiconsz = 12;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize", 32, false, nullptr);
                RearrangeIcons(true, true);
                break;
            case 1006:
                for (int items = 0; items < validItems; items++) {
                    switch (hiddenIcons) {
                    case 0:
                        pm[items]->SetVisible(false);
                        break;
                    case 1:
                        if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(true);
                        break;
                    }
                }
                hiddenIcons = !hiddenIcons;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons", hiddenIcons, false, nullptr);
                break;
            default:
                CMINVOKECOMMANDINFO ici;
                ZeroMemory(&ici, sizeof(ici));
                ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
                ici.lpVerb = MAKEINTRESOURCEA(menuItemId - 1);
                ici.nShow = SW_SHOWNORMAL;
                pICv1->InvokeCommand(&ici);
                break;
            }
        }
        pShellFolder->Release();
    }
}

void RightClickCore(LPCWSTR folderPath) {
    LPITEMIDLIST pidl = NULL;
    SHParseDisplayName(folderPath, NULL, &pidl, 0, NULL);

    IShellFolder* ppFolder = NULL;
    LPITEMIDLIST pidlChild = NULL;
    HRESULT hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&ppFolder, (LPCITEMIDLIST*)&pidlChild);

    LPCONTEXTMENU pICv1 = NULL;
    ppFolder->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidlChild, IID_IContextMenu, NULL, (void**)&pICv1);
    if (pICv1)
    {
        HMENU hm = CreatePopupMenu();
        pICv1->QueryContextMenu(hm, 0, MIN_SHELL_ID, MAX_SHELL_ID, CMF_EXPLORE);

        UINT uFlags = TPM_RIGHTBUTTON;
        if (localeType == 1) uFlags |= TPM_LAYOUTRTL;
        if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
            uFlags |= TPM_RIGHTALIGN;
        else
            uFlags |= TPM_LEFTALIGN;

        // Use TPM_RETURNCMD flag let TrackPopupMenuEx function return the menu item identifier of the user's selection in the return value.
        uFlags |= TPM_RETURNCMD;

        POINT pt;
        GetCursorPos(&pt);
        int menuItemId = TrackPopupMenuEx(hm, uFlags, pt.x, pt.y, wnd->GetHWND(), NULL);
        CMINVOKECOMMANDINFO ici;
        ZeroMemory(&ici, sizeof(ici));
        ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
        ici.lpVerb = MAKEINTRESOURCEA(menuItemId - 1);
        ici.nShow = SW_SHOWNORMAL;

        pICv1->InvokeCommand(&ici);
    }
    CoTaskMemFree(pidl);
    ppFolder->Release();
}

void ItemRightClick(Element* elem, Event* iev) {
    if (iev->uidType == Button::Context) {
        for (int items = 0; items < pm.size(); items++) {
            if (pm[items] == elem) {
                RightClickCore(RemoveQuotes2(pm[items]->GetFilename()).c_str());
            }
        }
    }
}

void SubItemRightClick(Element* elem, Event* iev) {
    if (iev->uidType == Button::Context) {
        for (int items = 0; items < subpm.size(); items++) {
            if (subpm[items] == elem) {
                RightClickCore(RemoveQuotes2(subpm[items]->GetFilename()).c_str());
            }
        }
    }
}