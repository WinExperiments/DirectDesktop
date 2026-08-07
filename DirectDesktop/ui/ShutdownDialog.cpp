#include "pch.h"

#include "ShutdownDialog.h"

#include "..\backend\DirectoryHelper.h"
#include "..\DirectDesktop.h"
#include <powrprof.h>
#include <wrl.h>
#include <wtsapi32.h>


using namespace std;
using namespace DirectUI;
using namespace DDUI;

namespace DirectDesktop
{
    NativeHWNDHost* shutdownwnd;
    DUIXmlParser* parserShutdown;
    HWNDElement* parentShutdown;
    Element* pShutdown;
    WNDPROC WndProcShutdown;
    DDIconButton* SwitchUser, *SignOut, *SleepButton, *Hibernate, *Shutdown, *Restart;
    DDScalableTouchButton* StatusCancel;
    Element* StatusBarResid;
    Element* StatusText;
    Element* AdvancedOptions;
    DDScalableButton *RestartWinRE, *RestartBIOS;
    DDScalableTouchEdit* delayseconds;

    HANDLE ActionThread, TimerThread;
    HWND hShutdownTimer;
    int savedremaining; // Display remaining time immediately when the dialog is invoked
    WCHAR reasonStr[128];
    bool g_dialogAnimation = false;
    static SimpleCubicBezierInterpolator* g_scbi = new SimpleCubicBezierInterpolator(0.8, 0.0, 0.0, 1.0);

    typedef HWND(WINAPI* pfnSHCreateWorkerWindowW)(WNDPROC, HWND, DWORD, DWORD, LPVOID);

    struct DialogValues
    {
        int buttonID{};
        int delay{};
    };

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
        WCHAR action[48] = L"0";
        wstring actionStr;
        switch (id)
        {
            case 1:
                LoadStrFromRes(action, 48, 3052, L"ShutdownUX.dll");
                break;
            case 2:
                LoadStrFromRes(action, 48, 3034, L"ShutdownUX.dll");
                break;
            case 3:
                LoadStrFromRes(action, 48, 3019, L"ShutdownUX.dll");
                break;
            case 4:
                LoadStrFromRes(action, 48, 3022, L"ShutdownUX.dll");
                break;
            case 5:
                LoadStrFromRes(action, 48, 3013, L"ShutdownUX.dll");
                break;
            case 6:
                LoadStrFromRes(action, 48, 3016, L"ShutdownUX.dll");
                break;
        }
        actionStr = action;
        transform(actionStr.begin(), actionStr.end(), actionStr.begin(), ::tolower);
        WCHAR cStatus[128], cStatusBuf[96];
        LoadStrFromRes(cStatusBuf, 96, 4037);
        StringCchPrintfW(cStatus, 128, cStatusBuf, actionStr.c_str(), delay);
        wstring result = cStatus;
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
            case WM_USER + 2:
            {
                if (delayedshutdownstatuses[lParam - 1] == false) break;
                switch (lParam)
                {
                case 1:
                {
                    WTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, FALSE);
                    break;
                }
                case 2:
                {
                    ExitWindowsEx(EWX_LOGOFF, 0);
                    break;
                }
                case 3:
                {
                    SetSuspendState(FALSE, FALSE, FALSE);
                    break;
                }
                case 4:
                {
                    SetSuspendState(TRUE, FALSE, FALSE);
                    break;
                }
                case 5:
                {
                    ExitWindowsEx(EWX_SHUTDOWN | EWX_POWEROFF, shutdownReason);
                    break;
                }
                case 6:
                {
                    ExitWindowsEx(EWX_REBOOT, shutdownReason);
                    break;
                }
                }
                delayedshutdownstatuses[lParam - 1] = false;
                break;
            }
        }
        return CallWindowProc(WndProcShutdown, hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK ShutdownTimerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        static RECT rcWindow{}, rcGadget{};
        static LONGLONG s_tick{};
        static short windowDirection;
        CSafeElementPtr<Element> ShutdownActions;
        switch (uMsg)
        {
        case WM_TIMER:
            static DWORD animCoef;
            switch (wParam)
            {
            case 1:
            case 3:
                KillTimer(hWnd, wParam);
                KillTimer(hWnd, wParam + 1);
                windowDirection = (wParam == 1) ? -1 : 1;
                s_tick = GetTickCount64();
                animCoef = g_pctx->animCoef;
                if (g_pctx->AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                GetWindowRect(shutdownwnd->GetHWND(), &rcWindow);
                ShutdownActions.Assign(regElem(L"ShutdownActions", pShutdown));
                ShutdownActions->SetLayoutPos(-3);
                GetGadgetRect(AdvancedOptions->GetDisplayNode(), &rcGadget, 0xC);
                ShutdownActions->SetLayoutPos(1);
                SetTimer(hWnd, wParam + 1, 10, nullptr);
                break;
            case 2:
            case 4:
                LONGLONG dwTickDiff = GetTickCount64() - s_tick;
                LONGLONG dwDistDiff{}, dwDistThreshold;
                dwDistThreshold = (rcGadget.bottom - rcGadget.top) * windowDirection;
                dwDistDiff = (dwDistThreshold + windowDirection) * g_scbi->GetProgression(dwTickDiff / (3.3 * animCoef));
                if (g_pctx->windowAnim && dwTickDiff / (3.3 * animCoef) <= 1)
                {
                    SetWindowPos(shutdownwnd->GetHWND(), NULL, NULL, NULL, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top + dwDistDiff,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
                }
                else
                {
                    dwDistDiff = dwDistThreshold;
                    SetWindowPos(shutdownwnd->GetHWND(), NULL, NULL, NULL, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top + dwDistDiff,
                        SWP_NOMOVE | SWP_NOZORDER);
                    KillTimer(hWnd, wParam - 1);
                    KillTimer(hWnd, wParam);
                    if (windowDirection == -1)
                        AdvancedOptions->SetLayoutPos(-3);
                    g_dialogAnimation = false;
                }
                break;
            }
            return 0;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
        SendMessageW(shutdownwnd->GetHWND(), WM_USER + 2, NULL, id);
        return 0;
    }

    void PerformOperation(Element* elem, Event* iev)
    {
        if (iev->uidType == DDIconButton::Click)
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

                WCHAR pszTitle[32];
                for (int i = 0; i < 6; i++)
                {
                    delayedshutdownstatuses[i] = false;
                }
                if (elem == StatusCancel)
                {
                    WCHAR pszContent[96];
                    LoadStrFromRes(pszTitle, 32, 4024);
                    LoadStrFromRes(pszContent, 96, 4039);
                    DestroyShutdownDialog();
                    ShowNotification(pszTitle, pszContent);
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
                if (delay > 0)
                {
                    LoadStrFromRes(pszTitle, 32, 4024);
                    ShowNotification(pszTitle, GetNotificationString(pressedID, delay));
                }
            }
        }
    }

    void ToggleAdvancedOptions(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click && !g_dialogAnimation)
        {
            AdvancedOptions->SetLayoutPos(3);
            CSafeElementPtr<RichText> AdvancedOptionsArrow;
            AdvancedOptionsArrow.Assign((RichText*)regElem(L"AdvancedOptionsArrow", elem));
            elem->SetSelected(!elem->GetSelected());
            short angle = elem->GetSelected() ? 180 : -180;
            GTRANS_DESC transDesc[1];
            TransitionStoryboardInfo tsbInfo = {};
            TriggerRotate(AdvancedOptionsArrow, transDesc, 0, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, angle, 0.0f, 0.5f, 0.5f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, AdvancedOptionsArrow->GetDisplayNode(), &tsbInfo);
            BYTE timerID = (elem->GetSelected()) ? 3 : 1;
            SetTimer(hShutdownTimer, timerID, 0, nullptr);
            g_dialogAnimation = true;
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
                    LoadStrFromRes(reasonStr, 128, 8261, L"user32.dll");
                    break;
                case 1:
                    shutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED;
                    LoadStrFromRes(reasonStr, 128, 8262, L"user32.dll");
                    break;
                case 2:
                    shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE;
                    LoadStrFromRes(reasonStr, 128, 8250, L"user32.dll");
                    break;
                case 3:
                    shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED;
                    LoadStrFromRes(reasonStr, 128, 8251, L"user32.dll");
                    break;
                case 4:
                    shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION;
                    LoadStrFromRes(reasonStr, 128, 8252, L"user32.dll");
                    break;
                case 5:
                    shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED;
                    LoadStrFromRes(reasonStr, 128, 8253, L"user32.dll");
                    break;
                case 6:
                    shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SYSTEMRESTORE;
                    LoadStrFromRes(reasonStr, 128, 8272, L"user32.dll");
                    break;
                case 7:
                    shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SYSTEMRESTORE | SHTDN_REASON_FLAG_PLANNED;
                    LoadStrFromRes(reasonStr, 128, 8271, L"user32.dll");
                    break;
                case 8:
                    shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG;
                    LoadStrFromRes(reasonStr, 128, 8256, L"user32.dll");
                    break;
                case 9:
                    shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG | SHTDN_REASON_FLAG_PLANNED;
                    LoadStrFromRes(reasonStr, 128, 8257, L"user32.dll");
                    break;
                case 10:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE;
                    LoadStrFromRes(reasonStr, 128, 8260, L"user32.dll");
                    break;
                case 11:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED;
                    LoadStrFromRes(reasonStr, 128, 8268, L"user32.dll");
                    break;
                case 12:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED;
                    LoadStrFromRes(reasonStr, 128, 8293, L"user32.dll");
                    break;
                case 13:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_HUNG;
                    LoadStrFromRes(reasonStr, 128, 8258, L"user32.dll");
                    break;
                case 14:
                    shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_UNSTABLE;
                    LoadStrFromRes(reasonStr, 128, 8259, L"user32.dll");
                    break;
                case 15:
                    shutdownReason = SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY;
                    LoadStrFromRes(reasonStr, 128, 8299, L"user32.dll");
                    break;
                case 16:
                    shutdownReason = SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY | SHTDN_REASON_FLAG_PLANNED;
                    LoadStrFromRes(reasonStr, 128, 8300, L"user32.dll");
                    break;
                case 17:
                    shutdownReason = SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_NETWORK_CONNECTIVITY;
                    LoadStrFromRes(reasonStr, 128, 8301, L"user32.dll");
                    break;
            }
        }
    }

    void DisplayShutdownDialog()
    {
        WCHAR caption[64];
        GetDialogString(caption, 64, 2000, L"shutdownux.dll", NULL, NULL);
        HWND hWndShutdown = FindWindowW(L"DD_ShutdownHost", caption);
        if (hWndShutdown) return;
        unsigned long key3 = 0;
        int ShutdownReasonUI = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\Reliability", L"ShutdownReasonUI");
        int sizeX = 500 * g_pctx->flScaleFactor;
        int sizeY = 400 * g_pctx->flScaleFactor;
        RECT dimensions, rcGadgetRect{};
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        DUIXmlParser::Create(&parserShutdown, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
        parserShutdown->SetXMLFromResource(IDR_UIFILE4, HINST_THISCOMPONENT, HINST_THISCOMPONENT);
        NativeHWNDHost::Create(L"DD_ShutdownHost", caption, nullptr, nullptr, (dimensions.left + dimensions.right - sizeX) / 2, (dimensions.bottom - sizeY) / 3 + dimensions.top / 1.33, sizeX, sizeY, NULL, WS_POPUP | WS_BORDER, nullptr, 0x43, &shutdownwnd);
        HWNDElement::Create(shutdownwnd->GetHWND(), true, 0x10, nullptr, &key3, (Element**)&parentShutdown);
        Microsoft::WRL::ComPtr<ITaskbarList> pTaskbarList = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList, (void**)&pTaskbarList)))
        {
            if (SUCCEEDED(pTaskbarList->HrInit()))
                pTaskbarList->DeleteTab(shutdownwnd->GetHWND());
        }
        parserShutdown->CreateElement(L"ShutDownWindows", parentShutdown, nullptr, nullptr, &pShutdown);
        WndProcShutdown = (WNDPROC)SetWindowLongPtrW(shutdownwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)ShutdownWindowProc);
        pShutdown->SetVisible(true);
        pShutdown->EndDefer(key3);
        shutdownwnd->Host(pShutdown);
        g_dialogopen = true;
        CSafeElementPtr<DDScalableElement> FakeTitlebar;
        FakeTitlebar.Assign((DDScalableElement*)regElem(L"FakeTitlebar", pShutdown));
        CSafeElementPtr<DDScalableElement> TitlebarText;
        TitlebarText.Assign((DDScalableElement*)regElem(L"TitlebarText", pShutdown));
        CSafeElementPtr<DDScalableElement> Logo;
        Logo.Assign((DDScalableElement*)regElem(L"Logo", pShutdown));
        CSafeElementPtr<Element> StatusBar;
        StatusBar.Assign(regElem(L"StatusBar", pShutdown));
        CSafeElementPtr<Element> ShutdownEventTracker;
        ShutdownEventTracker.Assign(regElem(L"ShutdownEventTracker", pShutdown));
        CSafeElementPtr<DDScalableButton> AdvancedOptionsExpando;
        AdvancedOptionsExpando.Assign((DDScalableButton*)regElem(L"AdvancedOptionsExpando", pShutdown));
        RestartWinRE = (DDScalableButton*)regElem(L"RestartWinRE", pShutdown);
        RestartBIOS = (DDScalableButton*)regElem(L"RestartBIOS", pShutdown);
        FIRMWARE_TYPE firmwareType{};
        GetFirmwareType(&firmwareType);
        if (firmwareType == FirmwareTypeUefi) RestartBIOS->SetLayoutPos(-1);
        AdvancedOptions = regElem(L"AdvancedOptions", pShutdown);
        CSafeElementPtr<RichText> moon;
        moon.Assign((DDScalableRichText*)regElem(L"moon", pShutdown));
        CSafeElementPtr<RichText> stars;
        stars.Assign((DDScalableRichText*)regElem(L"stars", pShutdown));
        LoadStrFromRes(reasonStr, 128, 8261, L"user32.dll");
        for (int i = 0; i < 6; i++)
        {
            if (delayedshutdownstatuses[i] == true)
            {
                parserShutdown->CreateElement(L"StatusBar", nullptr, nullptr, nullptr, (Element**)&StatusBarResid);
                StatusBar->Add((Element**)&StatusBarResid, 1);
                StatusText = regElem(L"StatusText", StatusBarResid);
                StatusCancel = (DDScalableTouchButton*)regElem(L"StatusCancel", StatusBarResid);
                assignFn(StatusCancel, PerformOperation);
                StatusText->SetContentString(GetNotificationString(i + 1, savedremaining).c_str());
                if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2)
                {
                    CSafeElementPtr<DDScalableElement> ReasonText;
                    ReasonText.Assign((DDScalableElement*)regElem(L"ReasonText", StatusBarResid));
                    ReasonText->SetLayoutPos(-1);
                    WCHAR cReason[128], cReasonBuf[32];
                    LoadStrFromRes(cReasonBuf, 32, 4038);
                    StringCchPrintfW(cReason, 128, cReasonBuf, reasonStr);
                    ReasonText->SetContentString(cReason);
                }
                GetGadgetRect(StatusBarResid->GetDisplayNode(), &rcGadgetRect, 0xC);
                sizeY += rcGadgetRect.bottom - rcGadgetRect.top;
                break;
            }
        }
        if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2)
        {
            shutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER;
            WCHAR pszTitle[64], pszReason[96];
            Element* ShutdownEventTrackerResid{};
            parserShutdown->CreateElement(L"ShutdownEventTracker", nullptr, nullptr, nullptr, (Element**)&ShutdownEventTrackerResid);
            ShutdownEventTracker->Add((Element**)&ShutdownEventTrackerResid, 1);
            CSafeElementPtr<DDScalableElement> SETText;
            SETText.Assign((DDScalableElement*)regElem(L"SETText", ShutdownEventTrackerResid));
            GetDialogString(pszTitle, 64, 2210, L"shutdownext.dll", NULL, NULL);
            SETText->SetContentString(pszTitle);
            DDCombobox* SETReason = (DDCombobox*)regElem(L"SETReason", ShutdownEventTracker);
            for (short s = 8261; s <= 8262; s++)
            {
                LoadStrFromRes(pszReason, 96, s, L"user32.dll");
                SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, pszReason);
            }
            for (short s = 8250; s <= 8253; s++)
            {
                LoadStrFromRes(pszReason, 96, s, L"user32.dll");
                SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, pszReason);
            }
            for (short s = 8272; s >= 8271; s--)
            {
                LoadStrFromRes(pszReason, 96, s, L"user32.dll");
                SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, pszReason);
            }
            for (short s = 8256; s <= 8257; s++)
            {
                LoadStrFromRes(pszReason, 96, s, L"user32.dll");
                SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, pszReason);
            }
            LoadStrFromRes(pszReason, 96, 8260, L"user32.dll");
            SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, pszReason);
            LoadStrFromRes(pszReason, 96, 8268, L"user32.dll");
            SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, pszReason);
            LoadStrFromRes(pszReason, 96, 8293, L"user32.dll");
            SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, pszReason);
            for (short s = 8258; s <= 8259; s++)
            {
                LoadStrFromRes(pszReason, 96, s, L"user32.dll");
                SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, pszReason);
            }
            for (short s = 8299; s <= 8301; s++)
            {
                LoadStrFromRes(pszReason, 96, s, L"user32.dll");
                SETReason->InsertSelection(DDCombobox::MAX_SELECTIONS, pszReason);
            }
            SETReason->SetSelection(0);
            assignFn(SETReason, UpdateShutdownReasonCode);
            GetGadgetRect(ShutdownEventTracker->GetDisplayNode(), &rcGadgetRect, 0xC);
            sizeY += rcGadgetRect.bottom - rcGadgetRect.top;
        }
        SetWindowPos(shutdownwnd->GetHWND(), nullptr, 0, 0, sizeX, sizeY, SWP_NOMOVE | SWP_NOZORDER);
        WCHAR cBuffer[64], cBuffer2[64];
        int dpiAdjusted = (g_pctx->dpi * 96.0) / g_pctx->dpiLaunch;
        LoadStrFromRes(cBuffer2, 64, 208);
        StringCchPrintfW(cBuffer, 64, cBuffer2, static_cast<int>((min(GetSystemMetricsForDpi(SM_CXSMICON, dpiAdjusted), GetSystemMetricsForDpi(SM_CYSMICON, dpiAdjusted))) * 0.75));
        moon->SetFont(cBuffer);
        LoadStrFromRes(cBuffer2, 64, 210);
        StringCchPrintfW(cBuffer, 64, cBuffer2, static_cast<int>((min(GetSystemMetricsForDpi(SM_CXSMICON, dpiAdjusted), GetSystemMetricsForDpi(SM_CYSMICON, dpiAdjusted))) * 0.375));
        stars->SetFont(cBuffer);
        TitlebarText->SetContentString(caption);
        WCHAR* WindowsBuildStr;
        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
        int WindowsBuild = _wtoi(WindowsBuildStr);
        free(WindowsBuildStr);
        int WindowsRev = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\BuildLayers\\ShellCommon", L"BuildQfe");
        BOOL value = TRUE;
        LPWSTR sheetName = (LPWSTR)L"shutdownstyle";
        if (!g_pctx->theme)
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
            pShutdown->SetBackgroundColor(0);
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
        SwitchUser = (DDIconButton*)regElem(L"SwitchUser", pShutdown), SignOut = (DDIconButton*)regElem(L"SignOut", pShutdown), SleepButton = (DDIconButton*)regElem(L"SleepButton", pShutdown),
        Hibernate = (DDIconButton*)regElem(L"Hibernate", pShutdown), Shutdown = (DDIconButton*)regElem(L"Shutdown", pShutdown), Restart = (DDIconButton*)regElem(L"Restart", pShutdown);
        DDIconButton* buttons[6] = { SwitchUser, SignOut, SleepButton, Hibernate, Shutdown, Restart };
        CSafeElementPtr<DDCheckBox> delaytoggle;
        delaytoggle.Assign((DDCheckBox*)regElem(L"delaytoggle", pShutdown));
        delayseconds = (DDScalableTouchEdit*)regElem(L"delayseconds", pShutdown);
        delayseconds->SetContentString(L"0");
        for (auto btn : buttons)
        {
            assignFn(btn, PerformOperation);
        }
        assignFn(AdvancedOptionsExpando, ToggleAdvancedOptions);
        assignFn(delaytoggle, ToggleDelayOption);
        assignFn(RestartWinRE, AdvancedShutdown);
        assignFn(RestartBIOS, AdvancedShutdown);
        HMODULE hShlwapi = GetModuleHandleW(L"shlwapi.dll");
        if (hShlwapi)
        {
            pfnSHCreateWorkerWindowW SHCreateWorkerWindowW =
                (pfnSHCreateWorkerWindowW)GetProcAddress(hShlwapi, "SHCreateWorkerWindowW");
            hShutdownTimer = SHCreateWorkerWindowW(ShutdownTimerProc, HWND_MESSAGE, 0, 0, nullptr);
        }
    }

    void DestroyShutdownDialog()
    {
        if (shutdownwnd)
        {
            CSafeElementPtr<DDScalableElement> delaysecondsbackground;
            delaysecondsbackground.Assign((DDScalableElement*)regElem(L"delaysecondsbackground", pShutdown));
            if (delaysecondsbackground) delaysecondsbackground->Destroy(true);
            DWORD animCoef = g_pctx->animCoef;
            if (g_pctx->AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            if (g_pctx->windowAnim)
                AnimateWindow(shutdownwnd->GetHWND(), (120 * animCoef / 100), AW_BLEND | AW_HIDE);
            else
                shutdownwnd->ShowWindow(SW_HIDE);
            pShutdown->DestroyAll(true);
            shutdownwnd->DestroyWindow();
            DestroyWindow(hShutdownTimer);
            g_dialogAnimation = false;
            g_dialogopen = false;
        }
    }
}
