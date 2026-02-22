#include "pch.h"
#include "..\DirectDesktop.h"
#include "AnimationHelper.h"
#include "..\coreui\BitmapHelper.h"
#include "..\coreui\StyleModifier.h"

using namespace DirectUI;

namespace DirectDesktop
{
    DWORD g_animCoef;

    DWORD WINAPI DestroyElement(LPVOID lpParam)
    {
        DelayedElementActions* dea = (DelayedElementActions*)lpParam;
        Sleep(dea->dwMillis);
        dea = (DelayedElementActions*)lpParam;
        if (dea->pe)
        {
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 1);
        }
        else delete dea;
        return 0;
    }

    DWORD WINAPI HideElement(LPVOID lpParam)
    {
        DelayedElementActions* dea = (DelayedElementActions*)lpParam;
        Sleep(dea->dwMillis);
        dea = (DelayedElementActions*)lpParam;
        if (dea->pe)
        {
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 2);
        }
        else delete dea;
        return 0;
    }

    // Original author: AllieTheFox, modified by WinExperiments
    void EulerRotationToAxisRotation(GTRANS_VALUE* pvEulerRotation1, GTRANS_VALUE* pvEulerRotation2)
    {
        if (pvEulerRotation1 && pvEulerRotation2)
        {
            // Convert Euler angles from degree to radian
            float flX1 = pvEulerRotation1->flX / 114.591559026164641;
            float flY1 = pvEulerRotation1->flY / 114.591559026164641;
            float flZ1 = pvEulerRotation1->flZ / 114.591559026164641;
            float flX2 = pvEulerRotation2->flX / 114.591559026164641;
            float flY2 = pvEulerRotation2->flY / 114.591559026164641;
            float flZ2 = pvEulerRotation2->flZ / 114.591559026164641;

            // Calculate sines and cosines of the angles
            float flCX1 = cosf(flX1), flSX1 = sinf(flX1);
            float flCY1 = cosf(flY1), flSY1 = sinf(flY1);
            float flCZ1 = cosf(flZ1), flSZ1 = sinf(flZ1);
            float flCX2 = cosf(flX2), flSX2 = sinf(flX2);
            float flCY2 = cosf(flY2), flSY2 = sinf(flY2);
            float flCZ2 = cosf(flZ2), flSZ2 = sinf(flZ2);

            // Create quaternions
            float flQW1 = flCX1 * flCY1 * flCZ1 + flSX1 * flSY1 * flSZ1;
            float flQX1 = flSX1 * flCY1 * flCZ1 - flCX1 * flSY1 * flSZ1;
            float flQY1 = flCX1 * flSY1 * flCZ1 + flSX1 * flCY1 * flSZ1;
            float flQZ1 = flCX1 * flCY1 * flSZ1 - flSX1 * flSY1 * flCZ1;
            float flQW2 = flCX2 * flCY2 * flCZ2 + flSX2 * flSY2 * flSZ2;
            float flQX2 = flSX2 * flCY2 * flCZ2 - flCX2 * flSY2 * flSZ2;
            float flQY2 = flCX2 * flSY2 * flCZ2 + flSX2 * flCY2 * flSZ2;
            float flQZ2 = flCX2 * flCY2 * flSZ2 - flSX2 * flSY2 * flCZ2;

            // Create difference quaternion (Q2 multiplied by conjugate of Q1)
            float flQW = flQW2 * flQW1 + flQX2 * flQX1 + flQY2 * flQY1 + flQZ2 * flQZ1;
            float flQX = flQW2 * -flQX1 + flQX2 * flQW1 - flQY2 * flQZ1 + flQZ2 * flQY1;
            float flQY = flQW2 * -flQY1 + flQX2 * flQZ1 + flQY2 * flQW1 - flQZ2 * flQX1;
            float flQZ = flQW2 * -flQZ1 - flQX2 * flQY1 + flQY2 * flQX1 + flQZ2 * flQW1;

            // Normalize quaternions
            if (flQW < 0.0f)
                flQW = -flQW, flQX = -flQX, flQY = -flQY, flQZ = -flQZ;
            if (flQW1 < 0.0f)
                flQW1 = -flQW1, flQX1 = -flQX1, flQY1 = -flQY1, flQZ1 = -flQZ1;
            if (flQW2 < 0.0f)
                flQW2 = -flQW2, flQX2 = -flQX2, flQY2 = -flQY2, flQZ2 = -flQZ2;

            float flInvDist = sqrtf(1.0f - flQW * flQW);

            // Set axis rotation values
            pvEulerRotation1->flX = flQX / flInvDist;
            pvEulerRotation1->flY = flQY / flInvDist;
            pvEulerRotation1->flZ = flQZ / flInvDist;
            pvEulerRotation2->flX = flQX / flInvDist;
            pvEulerRotation2->flY = flQY / flInvDist;
            pvEulerRotation2->flZ = flQZ / flInvDist;

            // Set angle rotation values
            float flProjection1 = flQX1 * pvEulerRotation1->flX + flQY1 * pvEulerRotation1->flY + flQZ1 * pvEulerRotation1->flZ;
            float flProjection2 = flQX2 * pvEulerRotation2->flX + flQY2 * pvEulerRotation2->flY + flQZ2 * pvEulerRotation2->flZ;
            pvEulerRotation1->flScalar = atan2f(flProjection1, flQW1) * 114.591559026164641;
            pvEulerRotation2->flScalar = atan2f(flProjection2, flQW2) * 114.591559026164641;
        }
    }

    void TriggerTranslate(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialPosX, float initialPosY, float targetPosX, float targetPosY, bool fHide, bool fDestroy, bool fAutoPos)
    {
        DWORD animCoef = g_animCoef;
        POINT ptZero{}, ptLoc{};
        if (fAutoPos) pe->GetParent()->MapElementPoint(pe, &ptZero, &ptLoc);
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        flDelay *= (animCoef / 100.0f);
        flDuration *= (animCoef / 100.0f);
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = 0x201;
        rgTrans[transIndex].nProperty = 1;
        rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
        rgTrans[transIndex].flDelay = flDelay;
        rgTrans[transIndex].flDuration = flDuration;
        rgTrans[transIndex].Curve.ptfl1.x = rX0;
        rgTrans[transIndex].Curve.ptfl1.y = rY0;
        rgTrans[transIndex].Curve.ptfl2.x = rX1;
        rgTrans[transIndex].Curve.ptfl2.y = rY1;
        rgTrans[transIndex].vInitial.flX = initialPosX + ptLoc.x;
        rgTrans[transIndex].vInitial.flY = initialPosY + ptLoc.y;
        rgTrans[transIndex].vEnd.flX = targetPosX + ptLoc.x;
        rgTrans[transIndex].vEnd.flY = targetPosY + ptLoc.y;
        float flDEA = DWMActive ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && !pe->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerFade(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialOpacity, float targetOpacity, bool fHide, bool fDestroy, bool fStuckFade)
    {
        DWORD animCoef = g_animCoef;
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        flDelay *= (animCoef / 100.0f);
        flDuration *= (animCoef / 100.0f);
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = fStuckFade ? 0xD : 0x9;
        rgTrans[transIndex].nProperty = 2;
        rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
        rgTrans[transIndex].flDelay = flDelay;
        rgTrans[transIndex].flDuration = flDuration;
        rgTrans[transIndex].Curve.ptfl1.x = rX0;
        rgTrans[transIndex].Curve.ptfl1.y = rY0;
        rgTrans[transIndex].Curve.ptfl2.x = rX1;
        rgTrans[transIndex].Curve.ptfl2.y = rY1;
        rgTrans[transIndex].vInitial.flScalar = initialOpacity;
        rgTrans[transIndex].vEnd.flScalar = targetOpacity;
        float flDEA = DWMActive ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && !pe->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerScaleIn(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialScaleX, float initialScaleY, float initialOriginX, float initialOriginY,
        float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        DWORD animCoef = g_animCoef;
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        flDelay *= (animCoef / 100.0f);
        flDuration *= (animCoef / 100.0f);
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = 0x201;
        rgTrans[transIndex].nProperty = 3;
        rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
        rgTrans[transIndex].flDelay = flDelay;
        rgTrans[transIndex].flDuration = flDuration;
        rgTrans[transIndex].Curve.ptfl1.x = rX0;
        rgTrans[transIndex].Curve.ptfl1.y = rY0;
        rgTrans[transIndex].Curve.ptfl2.x = rX1;
        rgTrans[transIndex].Curve.ptfl2.y = rY1;
        rgTrans[transIndex].vInitial.flX = initialScaleX;
        rgTrans[transIndex].vInitial.flY = initialScaleY;
        rgTrans[transIndex].vInitial.flOriginX = initialOriginX;
        rgTrans[transIndex].vInitial.flOriginY = initialOriginY;
        rgTrans[transIndex].vEnd.flX = targetScaleX;
        rgTrans[transIndex].vEnd.flY = targetScaleY;
        rgTrans[transIndex].vEnd.flOriginX = targetOriginX;
        rgTrans[transIndex].vEnd.flOriginY = targetOriginY;
        float flDEA = DWMActive ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && !pe->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerScaleOut(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        DWORD animCoef = g_animCoef;
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        flDelay *= (animCoef / 100.0f);
        flDuration *= (animCoef / 100.0f);
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = 0x204;
        rgTrans[transIndex].nProperty = 3;
        rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
        rgTrans[transIndex].flDelay = flDelay;
        rgTrans[transIndex].flDuration = flDuration;
        rgTrans[transIndex].Curve.ptfl1.x = rX0;
        rgTrans[transIndex].Curve.ptfl1.y = rY0;
        rgTrans[transIndex].Curve.ptfl2.x = rX1;
        rgTrans[transIndex].Curve.ptfl2.y = rY1;
        rgTrans[transIndex].vEnd.flX = targetScaleX;
        rgTrans[transIndex].vEnd.flY = targetScaleY;
        rgTrans[transIndex].vEnd.flOriginX = targetOriginX;
        rgTrans[transIndex].vEnd.flOriginY = targetOriginY;
        float flDEA = DWMActive ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && !pe->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerRotate(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialAngle, float targetAngle, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        DWORD animCoef = g_animCoef;
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        flDelay *= (animCoef / 100.0f);
        flDuration *= (animCoef / 100.0f);
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = 0x201;
        rgTrans[transIndex].nProperty = 4;
        rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
        rgTrans[transIndex].flDelay = flDelay;
        rgTrans[transIndex].flDuration = flDuration;
        rgTrans[transIndex].Curve.ptfl1.x = rX0;
        rgTrans[transIndex].Curve.ptfl1.y = rY0;
        rgTrans[transIndex].Curve.ptfl2.x = rX1;
        rgTrans[transIndex].Curve.ptfl2.y = rY1;
        rgTrans[transIndex].vInitial.flScalar = initialAngle;
        rgTrans[transIndex].vEnd.flScalar = targetAngle;
        rgTrans[transIndex].vEnd.flOriginX = targetOriginX;
        rgTrans[transIndex].vEnd.flOriginY = targetOriginY;
        float flDEA = DWMActive ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && !pe->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerSkew(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialAngleX, float initialAngleY, float targetAngleX, float targetAngleY, bool fHide, bool fDestroy)
    {
        DWORD animCoef = g_animCoef;
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        flDelay *= (animCoef / 100.0f);
        flDuration *= (animCoef / 100.0f);
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = 0x201;
        rgTrans[transIndex].nProperty = 5;
        rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
        rgTrans[transIndex].flDelay = flDelay;
        rgTrans[transIndex].flDuration = flDuration;
        rgTrans[transIndex].Curve.ptfl1.x = rX0;
        rgTrans[transIndex].Curve.ptfl1.y = rY0;
        rgTrans[transIndex].Curve.ptfl2.x = rX1;
        rgTrans[transIndex].Curve.ptfl2.y = rY1;
        rgTrans[transIndex].vInitial.flX = initialAngleX;
        rgTrans[transIndex].vInitial.flY = initialAngleY;
        rgTrans[transIndex].vInitial.flOriginX = 0.5f;
        rgTrans[transIndex].vInitial.flOriginY = 0.5f;
        rgTrans[transIndex].vEnd.flX = targetAngleX;
        rgTrans[transIndex].vEnd.flY = targetAngleY;
        rgTrans[transIndex].vEnd.flOriginX = 0.5f;
        rgTrans[transIndex].vEnd.flOriginY = 0.5f;
        float flDEA = DWMActive ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && !pe->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerClip(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialLeft, float initialTop, float initialRight, float initialBottom,
        float targetLeft, float targetTop, float targetRight, float targetBottom, bool fHide, bool fDestroy)
    {
        DWORD animCoef = g_animCoef;
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        flDelay *= (animCoef / 100.0f);
        flDuration *= (animCoef / 100.0f);
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = 0x201;
        rgTrans[transIndex].nProperty = 6;
        rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
        rgTrans[transIndex].flDelay = flDelay;
        rgTrans[transIndex].flDuration = flDuration;
        rgTrans[transIndex].Curve.ptfl1.x = rX0;
        rgTrans[transIndex].Curve.ptfl1.y = rY0;
        rgTrans[transIndex].Curve.ptfl2.x = rX1;
        rgTrans[transIndex].Curve.ptfl2.y = rY1;
        rgTrans[transIndex].vInitial.flX = initialRight;
        rgTrans[transIndex].vInitial.flY = initialBottom;
        rgTrans[transIndex].vInitial.flOriginX = initialLeft;
        rgTrans[transIndex].vInitial.flOriginY = initialTop;
        rgTrans[transIndex].vEnd.flX = targetRight;
        rgTrans[transIndex].vEnd.flY = targetBottom;
        rgTrans[transIndex].vEnd.flOriginX = targetLeft;
        rgTrans[transIndex].vEnd.flOriginY = targetTop;
        float flDEA = DWMActive ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && !pe->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerRotate3D(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialEulerAngleX, float initialEulerAngleY, float initialEulerAngleZ,
        float targetEulerAngleX, float targetEulerAngleY, float targetEulerAngleZ, float targetOriginAxisX, float targetOriginAxisY, float targetOriginAxisZ, bool fHide, bool fDestroy)
    {
        DWORD animCoef = g_animCoef;
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        flDelay *= (animCoef / 100.0f);
        flDuration *= (animCoef / 100.0f);
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = 0x201;
        rgTrans[transIndex].nProperty = 9;
        rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
        rgTrans[transIndex].flDelay = flDelay;
        rgTrans[transIndex].flDuration = flDuration;
        rgTrans[transIndex].Curve.ptfl1.x = rX0;
        rgTrans[transIndex].Curve.ptfl1.y = rY0;
        rgTrans[transIndex].Curve.ptfl2.x = rX1;
        rgTrans[transIndex].Curve.ptfl2.y = rY1;
        rgTrans[transIndex].vInitial.flX = initialEulerAngleX;
        rgTrans[transIndex].vInitial.flY = initialEulerAngleY;
        rgTrans[transIndex].vInitial.flZ = initialEulerAngleZ;
        rgTrans[transIndex].vEnd.flX = targetEulerAngleX;
        rgTrans[transIndex].vEnd.flY = targetEulerAngleY;
        rgTrans[transIndex].vEnd.flZ = targetEulerAngleZ;
        EulerRotationToAxisRotation(&(rgTrans[transIndex].vInitial), &(rgTrans[transIndex].vEnd));
        rgTrans[transIndex].vEnd.flOriginX = targetOriginAxisX;
        rgTrans[transIndex].vEnd.flOriginY = targetOriginAxisY;
        rgTrans[transIndex].vEnd.flOriginZ = targetOriginAxisZ;
        float flDEA = DWMActive ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && !pe->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }

    void TriggerCrossfade(Element* pe, float flDelay, float flDuration)
    {
        if (pe->GetVisible())
        {
            Element* peClone{};
            Element::Create(0, pe->GetRoot(), nullptr, &peClone);
            pe->GetRoot()->Add(&peClone, 1);
            AddLayeredRef(peClone->GetDisplayNode());
            SetGadgetStyle(peClone->GetDisplayNode(), NULL, NULL);
            RECT rcGadget;
            GetGadgetRect(pe->GetDisplayNode(), &rcGadget, 0xC);
            peClone->SetLayoutPos(-2);
            peClone->SetX(rcGadget.left);
            peClone->SetY(rcGadget.top);
            peClone->SetWidth(rcGadget.right - rcGadget.left);
            peClone->SetHeight(rcGadget.bottom - rcGadget.top);
            HBITMAP hbmOld;
            GetGadgetBitmap(pe->GetDisplayNode(), &hbmOld, &rcGadget);
            IterateBitmap(hbmOld, UndoPremultiplication, 1, 0, 1, NULL);
            CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(hbmOld, 7, 0xffffffff, false, false, false);
            if (spvBitmap)
                peClone->SetValue(Element::BackgroundProp, 1, spvBitmap);
            DeleteObject(hbmOld);
            GTRANS_DESC rgTrans[3];
            TransitionStoryboardInfo tsbInfo = {};
            TriggerFade(pe, rgTrans, 0, flDelay, flDuration * 0.9f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerFade(peClone, rgTrans, 1, flDelay + flDuration * 0.1f, flDuration, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, false);
            TriggerScaleOut(peClone, rgTrans, 2, 0.0f, flDuration + 0.05f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, false, true); // Give the element some time to get destroyed
            ScheduleGadgetTransitions_DWMCheck(0, 3, rgTrans, nullptr, &tsbInfo);
        }
    }
} 