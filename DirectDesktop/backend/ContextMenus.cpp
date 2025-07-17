#include "pch.h"

#include <ShlObj_core.h>
#include "ContextMenus.h"
#include "DirectoryHelper.h"
#include "..\DirectDesktop.h"
#include "..\ui\EditMode.h"

namespace DirectDesktop
{
    std::wstring RemoveQuotes2(const std::wstring& input)
    {
        if (input.size() >= 2 && input.front() == L'\"' && input.back() == L'\"')
        {
            return input.substr(1, input.size() - 2);
        }
        return input;
    }

    void SetView(int iconsz, int shiconsz, int gpiconsz, bool touch)
    {
        if (iconsz == g_iconsz && touch == g_touchmode) return;
        if (isDefaultRes()) SetPos(true);
        g_iconsz = iconsz;
        g_shiconsz = shiconsz;
        g_gpiconsz = gpiconsz;
        bool touchmodeMem = g_touchmode;
        g_touchmode = touch;
        if (!touch) SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize", iconsz, false, nullptr);
        if (touchmodeMem == !touch)
        {
            InitLayout(false, false, false);
            return;
        }
        RearrangeIcons(true, true, false);
    }

    void DesktopRightClick(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Context)
        {
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
                HMENU hsm2 = CreatePopupMenu();
                MENUITEMINFOW mii{};
                mii.cbSize = sizeof(MENUITEMINFOW);
                mii.fMask = MIIM_STATE;
                AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 1001, LoadStrFromRes(4004).c_str());
                AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 1002, LoadStrFromRes(4005).c_str());
                AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 1003, LoadStrFromRes(4006).c_str());
                AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 1004, LoadStrFromRes(4007).c_str());
                AppendMenuW(hsm, MF_STRING | MFT_RADIOCHECK, 1005, LoadStrFromRes(4034).c_str());
                for (int menuitem = 1001; menuitem <= 1005; menuitem++)
                {
                    mii.fState = MFS_UNCHECKED;
                    SetMenuItemInfoW(hsm, menuitem, 0, &mii);
                }
                mii.fState = MFS_CHECKED;
                if (g_touchmode) SetMenuItemInfoW(hsm, 1005, 0, &mii);
                else if (g_iconsz <= 32) SetMenuItemInfoW(hsm, 1004, 0, &mii);
                else if (g_iconsz <= 48) SetMenuItemInfoW(hsm, 1003, 0, &mii);
                else if (g_iconsz <= 96) SetMenuItemInfoW(hsm, 1002, 0, &mii);
                else SetMenuItemInfoW(hsm, 1001, 0, &mii);
                AppendMenuW(hsm, MF_SEPARATOR, 1006, L"_");
                AppendMenuW(hsm, MF_STRING, 1007, LoadStrFromRes(4008).c_str());
                mii.fState = g_hiddenIcons ? MFS_UNCHECKED : MFS_CHECKED;
                SetMenuItemInfoW(hsm, 1007, 0, &mii);
                AppendMenuW(hsm2, MF_STRING, 1008, L"Name");
                AppendMenuW(hsm2, MF_STRING, 1009, L"Date modified");
                AppendMenuW(hsm2, MF_STRING, 1010, L"Type");
                AppendMenuW(hsm2, MF_STRING, 1011, L"Size");
                InsertMenuW(hm, 0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hsm, LoadStrFromRes(4001).c_str());
                //InsertMenuW(hm, 1, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hsm2, L"Sort by");
                InsertMenuW(hm, 1, MF_BYPOSITION | MF_STRING, 2002, LoadStrFromRes(4002).c_str());
                InsertMenuW(hm, 2, MF_BYPOSITION | MF_STRING, 2003, LoadStrFromRes(4003).c_str());
                InsertMenuW(hm, 3, MF_BYPOSITION | MF_SEPARATOR, 2004, L"_");
                pICv1->QueryContextMenu(hm, 4, MIN_SHELL_ID, MAX_SHELL_ID, CMF_EXPLORE);

                int itemCount = GetMenuItemCount(hm);
                for (int i = 0; i < itemCount; i++)
                {
                    MENUITEMINFO mii = { 0 };
                    mii.cbSize = sizeof(MENUITEMINFO);
                    mii.fMask = MIIM_ID;
                    if (GetMenuItemInfoW(hm, i, TRUE, &mii))
                    {
                        if (mii.wID == 2004)
                        {
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
                bool touchmodeMem{};
                switch (menuItemId)
                {
                    case 2002:
                        InitLayout(false, false, true);
                        break;
                    case 2003:
                        ShowSimpleView(false);
                        break;
                    case 1001:
                        SetView(144, 64, 48, false);
                        break;
                    case 1002:
                        SetView(96, 48, 32, false);
                        break;
                    case 1003:
                        SetView(48, 32, 16, false);
                        break;
                    case 1004:
                        SetView(32, 32, 12, false);
                        break;
                    case 1005:
                        SetView(32, 32, 12, true);
                        break;
                    case 1007:
                        for (int items = 0; items < pm.size(); items++)
                        {
                            switch (g_hiddenIcons)
                            {
                                case 0:
                                    pm[items]->SetVisible(false);
                                    break;
                                case 1:
                                    if (pm[items]->GetPage() == g_currentPageID) pm[items]->SetVisible(true);
                                    break;
                            }
                        }
                        g_hiddenIcons = !g_hiddenIcons;
                        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons", g_hiddenIcons, false, nullptr);
                        break;
                    //case 1008:
                    //    break;
                    //case 1009:
                    //    break;
                    //case 1010:
                    //    break;
                    //case 1011:
                    //    break;
                    default:
                        CMINVOKECOMMANDINFO ici;
                        ZeroMemory(&ici, sizeof(ici));
                        ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
                        ici.lpVerb = MAKEINTRESOURCEA(menuItemId - 1);
                        ici.nShow = SW_SHOWNORMAL;
                        pICv1->InvokeCommand(&ici);
                        break;
                }
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"TouchView", g_touchmode, false, nullptr);
            }
            pShellFolder->Release();
        }
    }

    void RightClickCore(LPCWSTR folderPath)
    {
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

    void ItemRightClick(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Context)
        {
            RightClickCore(RemoveQuotes2(((LVItem*)elem)->GetFilename()).c_str());
        }
    }
}
