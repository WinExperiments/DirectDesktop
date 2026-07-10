#include "pch.h"

#include <ShlObj_core.h>
#include "ContextMenus.h"
#include "DirectoryHelper.h"
#include "RenameCore.h"
#include "..\DirectDesktop.h"
#include "..\ui\EditMode.h"
#include "..\ui\Subview.h"

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
        if (g_canRefreshMain)
        {
            bool touchmodeMem = g_touchmode;
            if (isDefaultRes()) SetPos(true);
            g_iconsz = iconsz;
            g_shiconsz = shiconsz;
            g_gpiconsz = gpiconsz;
            g_touchmode = touch;
            SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"TouchView", touch, false, nullptr);
            if (!touch) SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize", iconsz, false, nullptr);
            if (touchmodeMem == !touch)
            {
                if (touch) UIContainer->AddFlags(LVCF_TOUCH);
                else UIContainer->RemoveFlags(LVCF_TOUCH);
                // DO NOT REMOVE THIS TIMER OTHERWISE CRASHING HAPPENS MORE OFTEN
                g_canRefreshMain = false;
                SetTimer(wnd->GetHWND(), 16, 200, nullptr);
                SetTimer(wnd->GetHWND(), 13, 400, nullptr);
            }
            else
            {
                RearrangeIcons(true, true, false);
                g_canRefreshMain = false;
                SetTimer(wnd->GetHWND(), 13, 400, nullptr);
            }
        }
    }

    void DesktopRightClickCore(Element* peTarget, LPCSTR cmdID)
    {
        POINT pt;
        GetCursorPos(&pt);
        if (!peTarget)
            pt.x = 0, pt.y = 0;
        DDMenu* ddm = new DDMenu();
        DDMenu* ddsm = new DDMenu();
        //DDMenu* ddsm2 = new DDMenu();
        IShellView* pShellView = nullptr;
        IShellFolder* pShellFolder = nullptr;

        HRESULT hr = SHGetDesktopFolder(&pShellFolder);
        if (SUCCEEDED(hr))
        {
            hr = pShellFolder->CreateViewObject(GetShellWindow(), IID_PPV_ARGS(&pShellView));
            if (SUCCEEDED(hr))
            {
                hr = ddm->InitializeDesktopEntries(pShellFolder, pShellView);
                if (SUCCEEDED(hr))
                {
                    ddm->CreatePopupMenu(cmdID);
                    ddsm->CreatePopupMenu(cmdID);
                    g_menu = true;
                    //ddsm2->CreatePopupMenu(false);
                    MENUITEMINFOW mii{};
                    mii.cbSize = sizeof(MENUITEMINFOW);
                    mii.fMask = MIIM_STATE;
                    WCHAR pszMenu[64];
                    for (int i = 1001; i <= 1004; i++)
                    {
                        LoadStrFromRes(pszMenu, 64, i + 3003);
                        ddsm->AppendMenuW(MF_STRING | MFT_RADIOCHECK, i, pszMenu);
                    }
                    LoadStrFromRes(pszMenu, 64, 4034);
                    ddsm->AppendMenuW(MF_STRING | MFT_RADIOCHECK, 1005, pszMenu);
                    for (int menuitem = 1001; menuitem <= 1005; menuitem++)
                    {
                        mii.fState = MFS_UNCHECKED;
                        ddsm->SetMenuItemInfoW(menuitem, 0, &mii);
                    }
                    mii.fState = MFS_CHECKED;
                    if (g_touchmode) ddsm->SetMenuItemInfoW(1005, 0, &mii);
                    else if (g_iconsz <= 32) ddsm->SetMenuItemInfoW(1004, 0, &mii);
                    else if (g_iconsz <= 48) ddsm->SetMenuItemInfoW(1003, 0, &mii);
                    else if (g_iconsz <= 96) ddsm->SetMenuItemInfoW(1002, 0, &mii);
                    else ddsm->SetMenuItemInfoW(1001, 0, &mii);
                    ddsm->AppendMenuW(MF_SEPARATOR, 1006, L"_");
                    LoadStrFromRes(pszMenu, 64, 4008);
                    ddsm->AppendMenuW(MF_STRING, 1007, pszMenu);
                    mii.fState = g_hiddenIcons ? MFS_UNCHECKED : MFS_CHECKED;
                    ddsm->SetMenuItemInfoW(1007, 0, &mii);
                    //ddsm2->AppendMenuW(MF_STRING, 1008, L"Name");
                    //ddsm2->AppendMenuW(MF_STRING, 1009, L"Date modified");
                    //ddsm2->AppendMenuW(MF_STRING, 1010, L"Type");
                    //ddsm2->AppendMenuW(MF_STRING, 1011, L"Size");
                    LoadStrFromRes(pszMenu, 64, 4001);
                    ddm->InsertMenuW(0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)ddsm, pszMenu);
                    //ddm->InsertMenuW(1, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)ddsm2, L"Sort by");
                    LoadStrFromRes(pszMenu, 64, 4002);
                    ddm->InsertMenuW(1, MF_BYPOSITION | MF_STRING, 2002, pszMenu);
                    LoadStrFromRes(pszMenu, 64, 4003);
                    ddm->InsertMenuW(2, MF_BYPOSITION | MF_STRING, 2003, pszMenu);
                    ddm->InsertMenuW(3, MF_BYPOSITION | MF_SEPARATOR, 2004, L"_");
                    UINT uQuery = CMF_EXPLORE;
                    if (GetKeyState(VK_SHIFT) < 0) uQuery |= CMF_EXTENDEDVERBS;
                    if (cmdID) uQuery = CMF_DEFAULTONLY;
                    ddm->QueryContextMenu(4, MIN_SHELL_ID, MAX_SHELL_ID, uQuery);

                    int itemCount = ddm->GetItemCount();
                    for (int i = 0; i < itemCount; i++)
                    {
                        MENUITEMINFOW mii{};
                        mii.cbSize = sizeof(MENUITEMINFOW);
                        mii.fMask = MIIM_ID;
                        if (ddm->GetMenuItemInfoW(i, TRUE, &mii))
                        {
                            if (mii.wID == 2004)
                            {
                                for (int j = 0; j < 5; j++) ddm->RemoveMenu(i + 1, MF_BYPOSITION);
                                break;
                            }
                        }
                    }
                    ddm->ForceEnablePaste();

                    UINT uFlags = TPM_RIGHTBUTTON | TPM_RETURNCMD | DDM_ANIMATESUBMENUS;
                    if (g_pctx->localeType == 1) uFlags |= TPM_LAYOUTRTL;

                    int menuItemId = -1;
                    if (!cmdID)
                        menuItemId = ddm->TrackPopupMenuEx(uFlags, pt.x, pt.y, wnd->GetHWND(), nullptr);

                    g_menu = false;
                    bool touchmodeMem{};
                    RECT dimensions;
                    GetClientRect(wnd->GetHWND(), &dimensions);
                    switch (menuItemId)
                    {
                    case 2002:
                        if (g_canRefreshMain)
                        {
                            SetTimer(wnd->GetHWND(), 2, 200, nullptr);
                            SetTimer(wnd->GetHWND(), 13, 400, nullptr);
                        }
                        break;
                    case 2003:
                        ShowSimpleView(true, 0x0);
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
                            if (pm[items]->GetPage() == g_currentPageID)
                            {
                                float delay = (pm[items]->GetY() + pm[items]->GetHeight() / 2) / static_cast<float>(dimensions.bottom * 9);
                                float startXPos = ((dimensions.right / 2.0f) - (pm[items]->GetX() + (pm[items]->GetWidth() / 2))) * 0.2f;
                                float startYPos = ((dimensions.bottom / 2.0f) - (pm[items]->GetY() + (pm[items]->GetHeight() / 2))) * 0.2f;
                                GTRANS_DESC transReset[3];
                                switch (g_hiddenIcons)
                                {
                                case 0:
                                    TriggerTranslate(pm[items], transReset, 0, delay, delay + 0.22f, 1.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX() + startXPos, pm[items]->GetY() + startYPos, false, false, false);
                                    TriggerFade(pm[items], transReset, 1, delay + 0.11f, delay + 0.22f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                                    TriggerScaleOut_Ref((Element**)&pm[items], transReset, 2, delay, delay + 0.22f, 1.0f, 0.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, true, false);
                                    break;
                                case 1:
                                    delay *= 2;
                                    TriggerTranslate(pm[items], transReset, 0, delay, delay + 0.44f, 0.1f, 0.9f, 0.2f, 1.0f, pm[items]->GetX() + startXPos, pm[items]->GetY() + startYPos, pm[items]->GetX(), pm[items]->GetY(), true, false, false);
                                    TriggerFade(pm[items], transReset, 1, delay, delay + 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                                    TriggerScaleIn(pm[items], transReset, 2, delay, delay + 0.44f, 0.1f, 0.9f, 0.2f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                                    break;
                                }
                                TransitionStoryboardInfo tsbInfo = {};
                                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transReset), transReset, nullptr, &tsbInfo);
                                DUI_SetGadgetZOrder(pm[items], -1);
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
                    case 0:
                        break;
                    default:
                        CMINVOKECOMMANDINFOEX ici;
                        ZeroMemory(&ici, sizeof(ici));
                        ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
                        ici.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
                        if (GetKeyState(VK_CONTROL) < 0)
                            ici.fMask |= CMIC_MASK_CONTROL_DOWN;
                        if (GetKeyState(VK_SHIFT) < 0)
                            ici.fMask |= CMIC_MASK_SHIFT_DOWN;
                        ici.hwnd = GetShellWindow();
                        ici.lpVerb = MAKEINTRESOURCEA(menuItemId - MIN_SHELL_ID);
                        ici.lpVerbW = MAKEINTRESOURCEW(menuItemId - MIN_SHELL_ID);
                        ici.nShow = SW_SHOWNORMAL;
                        ici.ptInvoke = pt;
                        CHAR path[MAX_PATH];
                        WCHAR pathW[MAX_PATH];
                        SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path);
                        SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, pathW);
                        ici.lpDirectory = path;
                        ici.lpDirectoryW = pathW;
                        CHAR commandW[MAX_PATH];
                        if (cmdID)
                        {
                            int wideSize = MultiByteToWideChar(CP_ACP, 0, cmdID, -1, nullptr, 0);
                            if (wideSize >= 0)
                            {
                                WCHAR* cmdTemp = new WCHAR[wideSize];
                                MultiByteToWideChar(CP_ACP, 0, cmdID, -1, cmdTemp, wideSize);
                                CopyMemory(commandW, cmdTemp, MAX_PATH);
                                delete[] cmdTemp;
                            }
                            ici.lpVerb = cmdID;
                        }
                        else
                        {
                            ici.lpVerb = MAKEINTRESOURCEA(menuItemId - MIN_SHELL_ID);
                            ici.lpVerbW = MAKEINTRESOURCEW(menuItemId - MIN_SHELL_ID);
                            ddm->GetCommandString(menuItemId - MIN_SHELL_ID, GCS_VERBW, nullptr, commandW, MAX_PATH);
                        }
                        if (wcscmp((LPWSTR)commandW, L"paste") == 0)
                        {
                            vector<LVItem**> dummy;
                            RightClickCore(dummy, "paste", true);
                            break;
                        }
                        if (wcscmp((LPWSTR)commandW, L"undo") == 0 || wcscmp((LPWSTR)commandW, L"redo") == 0)
                        {
                            UINT id = (wcscmp((LPWSTR)commandW, L"undo") == 0) ? 28699 : 28704;
                            SendMessageW(g_hSHELLDLL_DefView, WM_COMMAND, id, NULL);
                            break;
                        }
                        if (wcscmp((LPWSTR)commandW, L"NewFolder") == 0 || ((LPWSTR)commandW)[0] == L'.')
                            g_newfolder = true;
                        hr = ddm->InvokeCommand((CMINVOKECOMMANDINFO*)&ici);
                        break;
                    }
                }
            }
            pShellView->Release();
            pShellFolder->Release();
        }
        ddm->DestroyPopupMenu();
    }

    void DesktopRightClick(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::RightClick && elem->GetMouseFocused())
            DesktopRightClickCore(iev->peTarget, nullptr);
    }

    void RightClickCore(std::vector<LVItem**> vItems, LPCSTR cmdID, bool fMouse)
    {
        POINT pt;
        if (fMouse)
            GetCursorPos(&pt);
        else
        {
            RECT rc;
            GetGadgetRect((*vItems[0])->GetDisplayNode(), &rc, 0x8);
            pt.x = (rc.left + rc.right) / 2;
            pt.y = (rc.top + rc.bottom) / 2;
        }

        const UINT cidl = vItems.size();
        vector<LPITEMIDLIST> rgpidl;
        for (int i = 0; i < cidl; i++)
        {
            LPITEMIDLIST pidl = nullptr;
            if (SUCCEEDED(SHParseDisplayName((LPWSTR)RemoveQuotes((*vItems[i])->GetFilename()).c_str(), nullptr, &pidl, 0, nullptr)))
                rgpidl.push_back(pidl);
        }

        IShellFolder* ppFolder = nullptr;
        HRESULT hr = SHGetDesktopFolder(&ppFolder);

        DDMenu* ddm = new DDMenu();
        hr = ddm->InitializeItemEntries(vItems, ppFolder, (LPCITEMIDLIST*)rgpidl.data(), cidl);
        if (SUCCEEDED(hr))
        {
            ddm->CreatePopupMenu(cmdID);
            g_menu = true;
            if (!cmdID && g_touchmode && cidl == 1)
            {
                DDMenu* ddsm = new DDMenu();
                ddsm->CreatePopupMenu(cmdID);
                MENUITEMINFOW mii{};
                mii.cbSize = sizeof(MENUITEMINFOW);
                mii.fMask = MIIM_STATE;
                WCHAR pszMenu[64];
                for (int i = 1001; i <= 1003; i++)
                {
                    LoadStrFromRes(pszMenu, 64, i + 3088);
                    ddsm->AppendMenuW(MF_STRING | MFT_RADIOCHECK, i, pszMenu);
                }
                for (int menuitem = 1001; menuitem <= 1005; menuitem++)
                {
                    mii.fState = MFS_UNCHECKED;
                    ddsm->SetMenuItemInfoW(menuitem, 0, &mii);
                }
                mii.fState = MFS_CHECKED;
                if ((*vItems[0])->GetTileSize() == LVITS_ICONONLY) ddsm->SetMenuItemInfoW(1001, 0, &mii);
                else if ((*vItems[0])->GetTileSize() == LVITS_NONE) ddsm->SetMenuItemInfoW(1002, 0, &mii);
                else ddsm->SetMenuItemInfoW(1003, 0, &mii);
                LoadStrFromRes(pszMenu, 64, 4088);
                ddm->InsertMenuW(0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)ddsm, pszMenu);
                ddm->InsertMenuW(1, MF_BYPOSITION | MF_SEPARATOR, 2002, L"_");
                if (!isDefaultRes()) ddm->EnableMenuItem(0, MF_BYPOSITION | MF_DISABLED);
            }
            UINT uQuery = CMF_EXPLORE;
            if (cidl < 2) uQuery |= CMF_CANRENAME;
            if (GetKeyState(VK_SHIFT) < 0) uQuery |= CMF_EXTENDEDVERBS;
            if (cmdID) uQuery = CMF_DEFAULTONLY;
            ddm->QueryContextMenu(2, MIN_SHELL_ID, MAX_SHELL_ID, uQuery);

            UINT uFlags = TPM_RIGHTBUTTON | TPM_RETURNCMD | DDM_ANIMATESUBMENUS;
            if (g_pctx->localeType == 1) uFlags |= TPM_LAYOUTRTL;

            int menuItemId = -1;
            if (!cmdID)
                menuItemId = ddm->TrackPopupMenuEx(uFlags, pt.x, pt.y, wnd->GetHWND(), nullptr);

            g_menu = false;
            CSafeElementPtr<RichText> textElem;
            LVItemTileSize lvits;
            if (cmdID) lvits = LVITS_NONE;
            else lvits = (*vItems[0])->GetTileSize();
            int tilepadding = DESKPADDING_TOUCH * g_pctx->flScaleFactor;
            switch (menuItemId)
            {
            case 1001:
                if (g_pctx->localeType == 1)
                {
                    if (lvits == LVITS_NONE)
                    {
                        (*vItems[0])->SetMemXPos((*vItems[0])->GetMemXPos() + g_touchSizeX / 2 + tilepadding / 2);
                        (*vItems[0])->SetX((*vItems[0])->GetMemXPos());
                    }
                    if (lvits == LVITS_DETAILED)
                    {
                        (*vItems[0])->SetMemXPos((*vItems[0])->GetMemXPos() + g_touchSizeX * 1.5f + tilepadding * 1.5f);
                        (*vItems[0])->SetX((*vItems[0])->GetMemXPos());
                    }
                }
                (*vItems[0])->SetTileSize(LVITS_ICONONLY);
                (*vItems[0])->SetTouchGrid(new LVItemTouchGrid);
                RearrangeIcons(true, false, true);
                if (isDefaultRes())
                {
                    textElem.Assign((RichText*)regElem(L"textElem", (*vItems[0])));
                    textElem->SetVisible(false);
                }
                break;
            case 1002:
                if (g_pctx->localeType == 1)
                {
                    if (lvits == LVITS_ICONONLY)
                    {
                        (*vItems[0])->SetMemXPos((*vItems[0])->GetMemXPos() - g_touchSizeX / 2 - tilepadding / 2);
                        (*vItems[0])->SetX((*vItems[0])->GetMemXPos());
                    }
                    if (lvits == LVITS_DETAILED)
                    {
                        (*vItems[0])->SetMemXPos((*vItems[0])->GetMemXPos() + g_touchSizeX + tilepadding);
                        (*vItems[0])->SetX((*vItems[0])->GetMemXPos());
                    }
                }
                (*vItems[0])->SetTouchGrid(nullptr);
                (*vItems[0])->SetTileSize(LVITS_NONE);
                (*vItems[0])->SetSmallPos(1);
                RearrangeIcons(true, false, true);
                if (isDefaultRes())
                {
                    textElem.Assign((RichText*)regElem(L"textElem", (*vItems[0])));
                    textElem->SetVisible(true);
                }
                break;
            case 1003:
                if (g_pctx->localeType == 1)
                {
                    if (lvits == LVITS_ICONONLY)
                    {
                        (*vItems[0])->SetMemXPos((*vItems[0])->GetMemXPos() - g_touchSizeX * 1.5f - tilepadding * 1.5f);
                        (*vItems[0])->SetX((*vItems[0])->GetMemXPos());
                    }
                    if (lvits == LVITS_NONE)
                    {
                        (*vItems[0])->SetMemXPos((*vItems[0])->GetMemXPos() - g_touchSizeX - tilepadding);
                        (*vItems[0])->SetX((*vItems[0])->GetMemXPos());
                    }
                }
                (*vItems[0])->SetTouchGrid(nullptr);
                (*vItems[0])->SetTileSize(LVITS_DETAILED);
                (*vItems[0])->SetSmallPos(1);
                RearrangeIcons(true, false, true);
                if (isDefaultRes())
                {
                    textElem.Assign((RichText*)regElem(L"textElem", (*vItems[0])));
                    textElem->SetVisible(true);
                }
                break;
            case 0:
                break;
            default:
                CMINVOKECOMMANDINFOEX ici;
                ZeroMemory(&ici, sizeof(ici));
                ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
                ici.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
                if (GetKeyState(VK_CONTROL) < 0)
                    ici.fMask |= CMIC_MASK_CONTROL_DOWN;
                if (GetKeyState(VK_SHIFT) < 0)
                    ici.fMask |= CMIC_MASK_SHIFT_DOWN;
                ici.hwnd = GetShellWindow();
                ici.nShow = SW_SHOWNORMAL;
                ici.ptInvoke = pt;
                CHAR path[MAX_PATH];
                WCHAR pathW[MAX_PATH];
                SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path);
                SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, pathW);
                ici.lpDirectory = path;
                ici.lpDirectoryW = pathW;
                CHAR command[MAX_PATH];
                if (cmdID)
                {
                    strcpy_s(command, cmdID);
                    ici.lpVerb = cmdID;
                }
                else
                {
                    ici.lpVerb = MAKEINTRESOURCEA(menuItemId - MIN_SHELL_ID);
                    ici.lpVerbW = MAKEINTRESOURCEW(menuItemId - MIN_SHELL_ID);
                    ddm->GetCommandString(menuItemId - MIN_SHELL_ID, GCS_VERBA, nullptr, command, MAX_PATH);
                }
                if (strcmp(command, "open") == 0 && cidl == 1 && g_treatdirasgroup && (*vItems[0])->GetFlags() & LVIF_GROUP)
                {
                    if ((*vItems[0])->GetGroupSize() == LVIGS_NORMAL)
                    {
                        ShowDirAsGroup(vItems[0]);
                        break;
                    }
                }
                if (strcmp(command, "cut") == 0)
                {
                    GTRANS_DESC* transDesc = new GTRANS_DESC[cidl];
                    TransitionStoryboardInfo tsbInfo = {};
                    for (int i = 0; i < cidl; i++)
                        TriggerFade((*vItems[i]), transDesc, i, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.6f, false, false, true);
                    ScheduleGadgetTransitions_DWMCheck(0, cidl, transDesc, nullptr, &tsbInfo);
                    for (int i = 0; i < cidl; i++)
                        DUI_SetGadgetZOrder((*vItems[i]), -1);
                    delete[] transDesc;
                }
                if (strcmp(command, "rename") == 0)
                {
                    ShowRename((*vItems[0]));
                    break;
                }
                hr = ddm->InvokeCommand((CMINVOKECOMMANDINFO*)&ici);
                break;
            }
        }
        else if (hr == E_NOTIMPL)
        {
            ddm->CreatePopupMenu(false);
            ddm->InsertMenuW(0, MF_BYPOSITION | MF_STRING, 1, L"Not Implemented");
            UINT uFlags = TPM_RIGHTBUTTON;
            if (g_pctx->localeType == 1) uFlags |= TPM_LAYOUTRTL;
            WCHAR glyph[3];
            LoadStrFromRes(glyph, 3, 138);
            ddm->SetMenuItemGlyph(0, TRUE, glyph);
            ddm->TrackPopupMenuEx(uFlags, pt.x, pt.y, wnd->GetHWND(), nullptr);
        }
        ddm->DestroyPopupMenu();
        ppFolder->Release();
    }

    // 0.5.8: may need revision in the future
    void ItemRightClick(Element* elem, Event* iev)
    {
        if (iev->uidType == LVItem::RightClick)
        {
            bool checkselections = true;
            selectedLVItems.clear();
            if (!elem->GetSelected() || (elem->GetParent()->GetParent()->GetClassInfoW() != LVGrid::GetClassInfoPtr()))
            {
                bool main = false;
                checkselections = false;
                for (int items = 0; items < pm.size(); items++)
                {
                    pm[items]->SetSelected(false);
                    if (pm[items] == elem)
                    {
                        main = true;
                        pm[items]->SetSelected(true);
                        selectedLVItems.push_back(&pm[items]);
                    }
                }
                if (!main) selectedLVItems.push_back((LVItem**)&elem);
            }
            if (!g_issubviewopen && checkselections) // 0.5.6.4: temporary hack until selected lvitems is extended to subview
            {
                for (int items = 0; items < pm.size(); items++)
                {
                    if (pm[items] != elem && pm[items]->GetSelected()) selectedLVItems.push_back(&pm[items]);
                    else if (pm[items] == elem) selectedLVItems.insert(selectedLVItems.begin(), &pm[items]);
                }
            }
            if (elem->GetMouseFocused() && selectedLVItems.size() > 0)
                RightClickCore(selectedLVItems, nullptr, true);
        }
    }
}
