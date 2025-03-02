#include <ShlObj_core.h>
#include <ShlGuid.h>
#include "ContextMenus.h"
#include "DirectoryHelper.h"

int validItems;
void DesktopRightClick(Element* elem, Event* iev) {
    if (iev->uidType == Button::Context) {

        IShellView* pShellView = NULL;
        IContextMenu3* pContextMenu = NULL;
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
            AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 101, L"Large icons");
            AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 102, L"Medium icons");
            AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 103, L"Small icons");
            for (int menuitem = 101; menuitem <= 103; menuitem++) {
                mii.fState = MFS_UNCHECKED;
                SetMenuItemInfoW(hsm, menuitem, 0, &mii);
            }
            mii.fState = MFS_CHECKED;
            if (globaliconsz <= 32) SetMenuItemInfoW(hsm, 103, 0, &mii);
            else if (globaliconsz <= 48) SetMenuItemInfoW(hsm, 102, 0, &mii);
            else SetMenuItemInfoW(hsm, 101, 0, &mii);
            AppendMenuW(hsm, MF_SEPARATOR, 104, L"_");
            AppendMenuW(hsm, MF_STRING, 105, L"Show desktop icons");
            mii.fState = hiddenIcons ? MFS_UNCHECKED : MFS_CHECKED;
            SetMenuItemInfoW(hsm, 105, 0, &mii);
            AppendMenuW(hm, MF_STRING | MF_POPUP, (UINT_PTR)hsm, L"View");
            pICv1->QueryContextMenu(hm, 1, MIN_SHELL_ID, MAX_SHELL_ID, CMF_EXPLORE);
            RemoveMenu(hm, 1, MF_BYPOSITION);
            RemoveMenu(hm, 1, MF_BYPOSITION);
            RemoveMenu(hm, 1, MF_BYPOSITION);
            RemoveMenu(hm, 1, MF_BYPOSITION);
            InsertMenuW(hm, 1, MF_BYPOSITION | MF_STRING, 3, L"Open Edit Mode");
            InsertMenuW(hm, 1, MF_BYPOSITION | MF_STRING, 2, L"Refresh");

            UINT uFlags = TPM_RIGHTBUTTON;
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
            case 2:
                InitLayout();
                break;
            case 3:
                ShowSimpleView();
                break;
            case 101:
                globaliconsz = 96;
                globalshiconsz = 48;
                globalgpiconsz = 32;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize", globaliconsz);
                RearrangeIcons(true, true);
                break;
            case 102:
                globaliconsz = 48;
                globalshiconsz = 32;
                globalgpiconsz = 16;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize", globaliconsz);
                RearrangeIcons(true, true);
                break;
            case 103:
                globaliconsz = 32;
                globalshiconsz = 32;
                globalgpiconsz = 12;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize", globaliconsz);
                RearrangeIcons(true, true);
                break;
            case 105:
                for (int items = 0; items < validItems; items++) {
                    switch (hiddenIcons) {
                    case 0:
                        pm[items].elem->SetVisible(false);
                        break;
                    case 1:
                        if (pm[items].page == currentPageID) pm[items].elem->SetVisible(true);
                        break;
                    }
                }
                hiddenIcons = !hiddenIcons;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons", hiddenIcons);
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

void ItemRightClick(Element* elem, Event* iev) {
    if (iev->uidType == Button::Context)
    {
        for (int items = 0; items < pm.size(); items++) {
            if (pm[items].elem == elem) {
                LPCWSTR folderPath = pm[items].filename.c_str();
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
        }
    }
}