#include "pch.h"

#include "ShutdownDialog.h"

#include "..\backend\DirectoryHelper.h"
#include "..\coreui\BitmapHelper.h"
#include "..\DirectDesktop.h"
#include <wrl.h>

#include "..\coreui\StyleModifier.h"

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    NativeHWNDHost* shutdownwnd;
    DUIXmlParser* parserShutdown;
    HWNDElement* parentShutdown;
    Element* pShutdown;
    WNDPROC WndProcShutdown;
    DDScalableButton *SwitchUser, *SignOut, *SleepButton, *Hibernate, *Shutdown, *Restart, *StatusCancel;
    Element* StatusBarResid;
    Element* StatusText;
    Element* AdvancedOptions;
    DDScalableButton *RestartWinRE, *RestartBIOS;
    DDScalableTouchEdit* delayseconds;

    HANDLE ActionThread, TimerThread;
    int savedremaining; // Display remaining time immediately when the dialog is invoked
    wstring reasonStr = LoadStrFromRes(8261, L"user32.dll");

    struct DialogValues
    {
        int buttonID{};
        int delay{};
    };

    bool IsServer()
    {
        wchar_t* productType = nullptr;
        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions", L"ProductType", &productType);
        return (wcscmp(productType, L"ServerNT") == 0);
        free(productType);
    }

    void ShowNotification(wstring title, wstring content)
    {
        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.uFlags = NIF_INFO;
        wcscpy_s(nid.szInfo, content.c_str());
        wcscpy_s(nid.szInfoTitle, title.c_str());
        nid.dwInfoFlags = NIIF_INFO;
        Shell_NotifyIconW(NIM_ADD, &nid);
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }

    wstring GetNotificationString(int id, int delay)
    {
        wstring action;
        switch (id)
        {
            case 1:
                action = LoadStrFromRes(3052, L"ShutdownUX.dll");
                break;
            case 2:
                action = LoadStrFromRes(3034, L"ShutdownUX.dll");
                break;
            case 3:
                action = LoadStrFromRes(3019, L"ShutdownUX.dll");
                break;
            case 4:
                action = LoadStrFromRes(3022, L"ShutdownUX.dll");
                break;
            case 5:
                action = LoadStrFromRes(3013, L"ShutdownUX.dll");
                break;
            case 6:
                action = LoadStrFromRes(3016, L"ShutdownUX.dll");
                break;
            default:
                action = L"0";
                break;
        }
        transform(action.begin(), action.end(), action.begin(), ::tolower);
        WCHAR* cStatus = new WCHAR[128];
        StringCchPrintfW(cStatus, 128, LoadStrFromRes(4037).c_str(), action.c_str(), delay);
        wstring result = cStatus;
        if (cStatus) delete[] cStatus;
        return result;
    }

    LRESULT CALLBACK ShutdownWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_CLOSE:
                DestroyShutdownDialog();
                return 0;
            case WM_DESTROY:
                return 0;
            case WM_ACTIVATE:
                WCHAR className[64];
                GetClassNameW((HWND)lParam, className, 64);
                if (LOWORD(wParam) == WA_INACTIVE && wcscmp(className, L"DDCMBMenuWindow") != 0) DestroyShutdownDialog();
                break;
            case WM_USER + 1:
            {
                if (StatusText) StatusText->SetContentString(GetNotificationString(wParam, lParam).c_str());
                break;
            }
        }
        return CallWindowProc(WndProcShutdown, hWnd, uMsg, wParam, lParam);
    }

    DWORD WINAPI ShowTimerStatus(LPVOID lpParam)
    {
        DialogValues* dv = (DialogValues*)lpParam;
        int id = dv->buttonID;
        int remaining = dv->delay;
        while (remaining >= 0)
        {
            if (IsWindowVisible(shutdownwnd->GetHWND())) SendMessageW(shutdownwnd->GetHWND(), WM_USER + 1, id, remaining);
            Sleep(1000);
            remaining--;
            savedremaining = remaining;
        }
        return 0;
    }

    DWORD WINAPI DelayedAction(LPVOID lpParam)
    {
        DialogValues* dv = (DialogValues*)lpParam;
        delayedshutdownstatuses[dv->buttonID - 1] = true;
        int seconds = dv->delay;
        int id = dv->buttonID;
        if (seconds > 0) TimerThread = CreateThread(nullptr, 0, ShowTimerStatus, dv, NULL, nullptr);
        Sleep(seconds * 1000);
        SendMessageW(wnd->GetHWND(), WM_USER + 19, NULL, id);
        return 0;
    }

    void PerformOperation(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            static bool validation{};
            validation = !validation;
            if (validation)
            {
                CValuePtr v;
                int pressedID{};
                if (delayseconds->GetContentString(&v) == nullptr) delayseconds->SetContentString(L"0");
                StatusText = nullptr;

                // TODO: Find a better way to stop older threads or make the bool array not global
                if (ActionThread) TerminateThread(ActionThread, 1);
                if (TimerThread) TerminateThread(TimerThread, 1);

                for (int i = 0; i < 6; i++)
                {
                    delayedshutdownstatuses[i] = false;
                }
                if (elem == StatusCancel)
                {
                    DestroyShutdownDialog();
                    ShowNotification(LoadStrFromRes(4024), LoadStrFromRes(4039));
                    return;
                }
                if (elem == SwitchUser) pressedID = 1;
                if (elem == SignOut) pressedID = 2;
                if (elem == SleepButton) pressedID = 3;
                if (elem == Hibernate) pressedID = 4;
                if (elem == Shutdown) pressedID = 5;
                if (elem == Restart) pressedID = 6;
                int delay = (AdvancedOptions->GetLayoutPos() == -3) ? 0 : _wtoi(delayseconds->GetContentString(&v));
                DialogValues* dv = new DialogValues{ pressedID, delay };
                DWORD dwAction{};
                ActionThread = CreateThread(nullptr, 0, DelayedAction, dv, NULL, &dwAction);
                DestroyShutdownDialog();
                if (delay > 0) ShowNotification(LoadStrFromRes(4024), GetNotificationString(pressedID, delay));
            }
        }
    }

    void ToggleAdvancedOptions(Element* elem, Event* iev)
    {
        static int expanded{};
        if (iev->uidType == Button::Click)
        {
            expanded++;
            short pos = expanded & 2 ? 3 : -3;
            bool arrowselection = expanded & 2 ? true : false;
            AdvancedOptions->SetLayoutPos(pos);
            CSafeElementPtr<RichText> AdvancedOptionsArrow;
            AdvancedOptionsArrow.Assign(regElem<RichText*>(L"AdvancedOptionsArrow", elem));
            AdvancedOptionsArrow->SetSelected(arrowselection);
            int ShutdownReasonUI = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\Reliability", L"ShutdownReasonUI");
            int sizeX = 500 * g_flScaleFactor;
            int sizeY = 400 * g_flScaleFactor;
            if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2) sizeY += 92 * g_flScaleFactor;
            for (int i = 0; i < 6; i++)
            {
                if (delayedshutdownstatuses[i] == true)
                {
                    sizeY += StatusBarResid->GetHeight();
                    break;
                }
            }
            if (expanded & 2) sizeY += AdvancedOptions->GetHeight();
            SetWindowPos(shutdownwnd->GetHWND(), nullptr, 0, 0, sizeX, sizeY, SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    void ToggleDelayOption(Element* elem, Event* iev)
    {
        if (iev->uidType == DDCheckBox::Click)
        {
            bool newChecked = (((DDCheckBox*)elem)->GetCheckedState() == false) ? true : false;
            ((DDCheckBox*)elem)->SetCheckedState(newChecked);
            delayseconds->SetEnabled((newChecked == true) ? true : false);
            if (newChecked == false)
            {
                delayseconds->SetContentString(L"0");
            }
        }
    }


    void AdvancedShutdown(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            if (elem == RestartWinRE)
            {
                WinExec("reagentc /boottore", SW_HIDE);
                WinExec("shutdown /r /f /t 0", SW_HIDE);
            }
            if (elem == RestartBIOS)
            {
                WinExec("shutdown /r /fw", SW_HIDE);
            }
        }
    }

    void UpdateShutdownReasonCode(Element* elem, Event* iev)
    {
        if (iev->uidType == DDCombobox::SelectionChange())
        {
            switch (((DDCombobox*)elem)->GetSelection())
            {
                case 0:
                    shutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER;
                    reasonStr = LoadStrFromRes(8261, L"user32.dll");
                    break;
                case 1:
                    shutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED;
                    reasonStr = LoadStrFromRes(8262, L"user32.dll");
                    break;
                case 2:
                    shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE;
                    reasonStr = LoadStrFromRes(8250, L"user32.dll");
                    break;
                case 3:
                    shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED;
                    reasonStr = LoadStrFromRes(8251, L"user32.dll");
                    break;
                case 4:
                    shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION;
                    reasonStr = LoadStrFromRes(8252, L"user32.dll");
                    break;
                case 5:
                    shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED;
                    reasonStr = LoadStrFromRes(8253, L"user32.dll");
                    break;
                case 6:
                    shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SYSTEMRESTORE;
                    reasonStr = LoadStrFromRes(8272, L"user32.dll");
                    break;
                case 7:
                    shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SYSTEMRESTORE | SHTDN_REASON_FLAG_PLANNED;
                    reasonStr = LoadStrFromRes(8271, L"user32.dll");
                    break;
                case 8:
                    shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG;
                    reasonStr = LoadStrFromRes(8256, L"user32.dll");
                    break;
                case 9:
                    shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG | SHTDN_REASON_FLAG_PLANNED;
                    reasonStr = LoadStrFromRes(8257, L"user32.dll");
                    break;
                case 10:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE;
                    reasonStr = LoadStrFromRes(8260, L"user32.dll");
                    break;
                case 11:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED;
                    reasonStr = LoadStrFromRes(8268, L"user32.dll");
                    break;
                case 12:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED;
                    reasonStr = LoadStrFromRes(8293, L"user32.dll");
                    break;
                case 13:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_HUNG;
                    reasonStr = LoadStrFromRes(8258, L"user32.dll");
                    break;
                case 14:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_UNSTABLE;
                    reasonStr = LoadStrFromRes(8259, L"user32.dll");
                    break;
                case 15:
                    shutdownReason = SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY;
                    reasonStr = LoadStrFromRes(8299, L"user32.dll");
                    break;
                case 16:
                    shutdownReason = SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY | SHTDN_REASON_FLAG_PLANNED;
                    reasonStr = LoadStrFromRes(8300, L"user32.dll");
                    break;
                case 17:
                    shutdownReason = SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_NETWORK_CONNECTIVITY;
                    reasonStr = LoadStrFromRes(8301, L"user32.dll");
                    break;
            }
        }
    }

    void DisplayShutdownDialog()
    {
        wstring caption = GetDialogString(2000, L"shutdownux.dll", NULL, NULL);
        HWND hWndShutdown = FindWindowW(L"DD_ShutdownHost", caption.c_str());
        if (hWndShutdown) return;
        unsigned long key3 = 0;
        int ShutdownReasonUI = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\Reliability", L"ShutdownReasonUI");
        int sizeX = 500 * g_flScaleFactor;
        int sizeY = 400 * g_flScaleFactor;
        if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2) sizeY += 92 * g_flScaleFactor;
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        NativeHWNDHost::Create(L"DD_ShutdownHost", caption.c_str(), nullptr, nullptr, (dimensions.left + dimensions.right - sizeX) / 2, (dimensions.bottom - sizeY) / 3 + dimensions.top / 1.33, sizeX, sizeY, NULL, WS_POPUP | WS_BORDER, nullptr, 0x43, &shutdownwnd);
        DUIXmlParser::Create(&parserShutdown, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
        parserShutdown->SetXMLFromResource(IDR_UIFILE4, HINST_THISCOMPONENT, HINST_THISCOMPONENT);
        HWNDElement::Create(shutdownwnd->GetHWND(), true, NULL, nullptr, &key3, (Element**)&parentShutdown);
        Microsoft::WRL::ComPtr<ITaskbarList> pTaskbarList = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
            IID_ITaskbarList, (void**)&pTaskbarList)))
        {
            if (SUCCEEDED(pTaskbarList->HrInit()))
            {
                pTaskbarList->DeleteTab(shutdownwnd->GetHWND());
            }
        }
        parserShutdown->CreateElement(L"ShutDownWindows", parentShutdown, nullptr, nullptr, &pShutdown);
        WndProcShutdown = (WNDPROC)SetWindowLongPtrW(shutdownwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)ShutdownWindowProc);
        pShutdown->SetVisible(true);
        pShutdown->EndDefer(key3);
        shutdownwnd->Host(pShutdown);
        g_dialogopen = true;
        CSafeElementPtr<DDScalableElement> FakeTitlebar;
        FakeTitlebar.Assign(regElem<DDScalableElement*>(L"FakeTitlebar", pShutdown));
        CSafeElementPtr<DDScalableElement> TitlebarText;
        TitlebarText.Assign(regElem<DDScalableElement*>(L"TitlebarText", pShutdown));
        CSafeElementPtr<DDScalableElement> Logo;
        Logo.Assign(regElem<DDScalableElement*>(L"Logo", pShutdown));
        CSafeElementPtr<Element> StatusBar;
        StatusBar.Assign(regElem<Element*>(L"StatusBar", pShutdown));
        CSafeElementPtr<Element> SeparatorLine;
        SeparatorLine.Assign(regElem<Element*>(L"SeparatorLine", pShutdown));
        CSafeElementPtr<Element> SeparatorLine2;
        SeparatorLine2.Assign(regElem<Element*>(L"SeparatorLine2", pShutdown));
        CSafeElementPtr<Element> ShutdownActions;
        ShutdownActions.Assign(regElem<Element*>(L"ShutdownActions", pShutdown));
        CSafeElementPtr<Element> ShutdownEventTracker;
        ShutdownEventTracker.Assign(regElem<Element*>(L"ShutdownEventTracker", pShutdown));
        CSafeElementPtr<Element> AdvancedOptionsBar;
        AdvancedOptionsBar.Assign(regElem<Element*>(L"AdvancedOptionsBar", pShutdown));
        RestartWinRE = regElem<DDScalableButton*>(L"RestartWinRE", pShutdown);
        RestartBIOS = regElem<DDScalableButton*>(L"RestartBIOS", pShutdown);
        FIRMWARE_TYPE firmwareType{};
        GetFirmwareType(&firmwareType);
        if (firmwareType == FirmwareTypeUefi) RestartBIOS->SetLayoutPos(-1);
        AdvancedOptions = regElem<Element*>(L"AdvancedOptions", pShutdown);
        CSafeElementPtr<RichText> moon;
        moon.Assign(regElem<RichText*>(L"moon", pShutdown));
        CSafeElementPtr<RichText> stars;
        stars.Assign(regElem<RichText*>(L"stars", pShutdown));
        for (int i = 0; i < 6; i++)
        {
            if (delayedshutdownstatuses[i] == true)
            {
                parserShutdown->CreateElement(L"StatusBar", nullptr, nullptr, nullptr, (Element**)&StatusBarResid);
                StatusBar->Add((Element**)&StatusBarResid, 1);
                StatusText = regElem<DDScalableElement*>(L"StatusText", StatusBarResid);
                StatusCancel = regElem<DDScalableButton*>(L"StatusCancel", StatusBarResid);
                assignFn(StatusCancel, PerformOperation);
                sizeY += StatusBarResid->GetHeight();
                SetWindowPos(shutdownwnd->GetHWND(), nullptr, 0, 0, sizeX, sizeY, SWP_NOMOVE | SWP_NOZORDER);
                StatusText->SetContentString(GetNotificationString(i + 1, savedremaining).c_str());
                if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2)
                {
                    CSafeElementPtr<DDScalableElement> ReasonText;
                    ReasonText.Assign(regElem<DDScalableElement*>(L"ReasonText", StatusBarResid));
                    ReasonText->SetLayoutPos(-1);
                    WCHAR* cReason = new WCHAR[128];
                    StringCchPrintfW(cReason, 128, LoadStrFromRes(4038).c_str(), reasonStr.c_str());
                    ReasonText->SetContentString(cReason);
                    if (cReason) delete[] cReason;
                }
                break;
            }
        }
        if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2)
        {
            shutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER;
            Element* ShutdownEventTrackerResid{};
            parserShutdown->CreateElement(L"ShutdownEventTracker", nullptr, nullptr, nullptr, (Element**)&ShutdownEventTrackerResid);
            ShutdownEventTracker->Add((Element**)&ShutdownEventTrackerResid, 1);
            CSafeElementPtr<DDScalableElement> SETText;
            SETText.Assign(regElem<DDScalableElement*>(L"SETText", ShutdownEventTrackerResid));
            SETText->SetContentString(GetDialogString(2210, L"shutdownext.dll", NULL, NULL).c_str());
            DDCombobox* SETReason = regElem<DDCombobox*>(L"SETReason", ShutdownEventTracker);
            for (short s = 8261; s <= 8262; s++) SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, LoadStrFromRes(s, L"user32.dll").c_str());
            for (short s = 8250; s <= 8253; s++) SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, LoadStrFromRes(s, L"user32.dll").c_str());
            for (short s = 8272; s >= 8271; s--) SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, LoadStrFromRes(s, L"user32.dll").c_str());
            for (short s = 8256; s <= 8257; s++) SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, LoadStrFromRes(s, L"user32.dll").c_str());
            SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, LoadStrFromRes(8260, L"user32.dll").c_str());
            SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, LoadStrFromRes(8268, L"user32.dll").c_str());
            SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, LoadStrFromRes(8293, L"user32.dll").c_str());
            for (short s = 8258; s <= 8259; s++) SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, LoadStrFromRes(s, L"user32.dll").c_str());
            for (short s = 8299; s <= 8301; s++) SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, LoadStrFromRes(s, L"user32.dll").c_str());
            SETReason->SetSelection(0);
            assignFn(SETReason, UpdateShutdownReasonCode);
        }
        WCHAR* cBuffer = new WCHAR[64];
        int dpiAdjusted = (g_dpi * 96.0) / g_dpiLaunch;
        StringCchPrintfW(cBuffer, 64, LoadStrFromRes(208).c_str(), static_cast<int>((min(GetSystemMetricsForDpi(SM_CXSMICON, dpiAdjusted), GetSystemMetricsForDpi(SM_CYSMICON, dpiAdjusted))) * 0.75));
        moon->SetFont(cBuffer);
        StringCchPrintfW(cBuffer, 64, LoadStrFromRes(210).c_str(), static_cast<int>((min(GetSystemMetricsForDpi(SM_CXSMICON, dpiAdjusted), GetSystemMetricsForDpi(SM_CYSMICON, dpiAdjusted))) * 0.375));
        stars->SetFont(cBuffer);
        delete[] cBuffer;
        TitlebarText->SetContentString(caption.c_str());
        HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(2), IMAGE_ICON, 16, 16, LR_SHARED);
        HBITMAP colorBMP{}, colorBMP2{};
        IconToBitmap(dummyi, colorBMP, 16, 16);
        IconToBitmap(dummyi, colorBMP2, 16, 16);
        COLORREF separator = g_theme ? RGB(0, 0, 0) : RGB(255, 255, 255);
        float bodyalpha = g_theme ? 0.4 : 0.05;
        IterateBitmap(colorBMP, SimpleBitmapPixelHandler, 3, 0, 0.125, separator);
        CValuePtr spvColorBMP = DirectUI::Value::CreateGraphic(colorBMP, 7, 0xffffffff, false, false, false);
        SeparatorLine->SetValue(Element::BackgroundProp, 1, spvColorBMP);
        SeparatorLine2->SetValue(Element::BackgroundProp, 1, spvColorBMP);
        DeleteObject(colorBMP);
        COLORREF white = RGB(255, 255, 255);
        IterateBitmap(colorBMP2, SimpleBitmapPixelHandler, 3, 0, bodyalpha, white);
        CValuePtr spvColorBMP2 = DirectUI::Value::CreateGraphic(colorBMP2, 7, 0xffffffff, false, false, false);
        ShutdownActions->SetValue(Element::BackgroundProp, 1, spvColorBMP2);
        AdvancedOptions->SetValue(Element::BackgroundProp, 1, spvColorBMP2);
        StatusBar->SetValue(Element::BackgroundProp, 1, spvColorBMP2);
        ShutdownEventTracker->SetValue(Element::BackgroundProp, 1, spvColorBMP2);
        DeleteObject(dummyi);
        DeleteObject(colorBMP2);
        WCHAR* WindowsBuildStr;
        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
        int WindowsBuild = _wtoi(WindowsBuildStr);
        free(WindowsBuildStr);
        int WindowsRev = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\BuildLayers\\ShellCommon", L"BuildQfe");
        BOOL value = TRUE;
        LPWSTR sheetName = (LPWSTR)L"shutdownstyle";
        if (!g_theme)
        {
            sheetName = (LPWSTR)L"shutdownstyledark";
            DwmSetWindowAttribute(shutdownwnd->GetHWND(), DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
        }
        StyleSheet* sheet = pShutdown->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parserShutdown->GetSheet(sheetName, &sheetStorage);
        pShutdown->SetValue(Element::SheetProp, 1, sheetStorage);
        //AnimateWindow(shutdownwnd->GetHWND(), 180, AW_BLEND);
        shutdownwnd->ShowWindow(SW_SHOW);
        //SetFocus(shutdownwnd->GetHWND());
        if (WindowsBuild > 22000 || WindowsBuild == 22000 && WindowsRev >= 51)
        {
            MARGINS margins = { -1, -1, -1, -1 };
            DwmExtendFrameIntoClientArea(shutdownwnd->GetHWND(), &margins);
            DwmSetWindowAttribute(shutdownwnd->GetHWND(), DWMWA_USE_HOSTBACKDROPBRUSH, &value, sizeof(value));
            DWM_SYSTEMBACKDROP_TYPE backdrop_type = DWMSBT_MAINWINDOW;
            DwmSetWindowAttribute(shutdownwnd->GetHWND(), DWMWA_SYSTEMBACKDROP_TYPE, &backdrop_type, sizeof(backdrop_type));
            DWORD cornerPreference = DWMWCP_ROUND;
            DwmSetWindowAttribute(shutdownwnd->GetHWND(), DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
            pShutdown->SetBackgroundStdColor(7);
        }
        if (WindowsBuild < 21996)
        {
            if (IsServer())
                Logo->SetAccDesc(L"Windows Server 2022");
            else
                Logo->SetAccDesc(L"Windows 10");
        }
        else
        {
            if (IsServer())
                Logo->SetAccDesc(L"Windows Server 2025");
            else
                Logo->SetAccDesc(L"Windows 11");
        }
        SwitchUser = regElem<DDScalableButton*>(L"SwitchUser", pShutdown), SignOut = regElem<DDScalableButton*>(L"SignOut", pShutdown), SleepButton = regElem<DDScalableButton*>(L"SleepButton", pShutdown),
            Hibernate = regElem<DDScalableButton*>(L"Hibernate", pShutdown), Shutdown = regElem<DDScalableButton*>(L"Shutdown", pShutdown), Restart = regElem<DDScalableButton*>(L"Restart", pShutdown);
        Button* buttons[6] = { SwitchUser, SignOut, SleepButton, Hibernate, Shutdown, Restart };
        CSafeElementPtr<DDCheckBox> delaytoggle;
        delaytoggle.Assign(regElem<DDCheckBox*>(L"delaytoggle", pShutdown));
        delayseconds = regElem<DDScalableTouchEdit*>(L"delayseconds", pShutdown);
        delayseconds->SetContentString(L"0");
        for (auto btn : buttons)
        {
            assignFn(btn, PerformOperation);
        }
        assignFn(AdvancedOptionsBar, ToggleAdvancedOptions);
        assignFn(delaytoggle, ToggleDelayOption);
        assignFn(RestartWinRE, AdvancedShutdown);
        assignFn(RestartBIOS, AdvancedShutdown);
    }

    void DestroyShutdownDialog()
    {
        CSafeElementPtr<DDScalableElement> delaysecondsbackground;
        delaysecondsbackground.Assign(regElem<DDScalableElement*>(L"delaysecondsbackground", pShutdown));
        if (delaysecondsbackground) delaysecondsbackground->Destroy(true);
        AnimateWindow(shutdownwnd->GetHWND(), 120, AW_BLEND | AW_HIDE);
        pShutdown->DestroyAll(true);
        shutdownwnd->DestroyWindow();
        g_dialogopen = false;
    }
}
