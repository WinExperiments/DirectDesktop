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
        Element* pe;
        if (dea->ppe)
            pe = *(dea->ppe);
        else
            pe = dea->pe;
        if (pe)
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 1);
        else delete dea;
        return 0;
    }

    DWORD WINAPI HideElement(LPVOID lpParam)
    {
        DelayedElementActions* dea = (DelayedElementActions*)lpParam;
        Sleep(dea->dwMillis);
        dea = (DelayedElementActions*)lpParam;
        Element* pe;
        if (dea->ppe)
            pe = *(dea->ppe);
        else
            pe = dea->pe;
        if (pe)
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 2);
        else delete dea;
        return 0;
    }

    void CubicBezier()
    {

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

    void CheckHideDestroy(Element* pe, float flDelay, float flDuration, bool fHide, bool fDestroy)
    {
        float flDEA = (DWMActive && g_clientAnim) ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe, nullptr };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && g_clientAnim && !pe->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), pe, nullptr };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }

    void CheckHideDestroy_Ref(Element** ppe, float flDelay, float flDuration, bool fHide, bool fDestroy)
    {
        float flDEA = (DWMActive && g_clientAnim) ? flDuration * 1000 : 0.0f;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), nullptr, ppe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            if (DWMActive && g_clientAnim && !(*ppe)->GetVisible()) flDEA = flDelay * 1000;
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDEA), nullptr, ppe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }

    void _TriggerTranslate(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float* pflDelay, float* pflDuration,
        float rX0, float rY0, float rX1, float rY1, float initialPosX, float initialPosY, float targetPosX, float targetPosY, bool fAutoPos)
    {
        Element* pe = *ppe;
        if (pe)
        {
            DWORD animCoef = g_animCoef;
            POINT ptZero{}, ptLoc{};
            if (fAutoPos) pe->GetParent()->MapElementPoint(pe, &ptZero, &ptLoc);
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            *pflDelay *= (animCoef / 100.0f);
            *pflDuration *= (animCoef / 100.0f);
            rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
            rgTrans[transIndex].nFlags = 0x201;
            rgTrans[transIndex].nProperty = 1;
            rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
            rgTrans[transIndex].flDelay = *pflDelay;
            rgTrans[transIndex].flDuration = *pflDuration;
            rgTrans[transIndex].Curve.ptfl1.x = rX0;
            rgTrans[transIndex].Curve.ptfl1.y = rY0;
            rgTrans[transIndex].Curve.ptfl2.x = rX1;
            rgTrans[transIndex].Curve.ptfl2.y = rY1;
            rgTrans[transIndex].vInitial.flX = initialPosX + ptLoc.x;
            rgTrans[transIndex].vInitial.flY = initialPosY + ptLoc.y;
            rgTrans[transIndex].vEnd.flX = targetPosX + ptLoc.x;
            rgTrans[transIndex].vEnd.flY = targetPosY + ptLoc.y;
        }
    }
    void _TriggerFade(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float* pflDelay, float* pflDuration,
        float rX0, float rY0, float rX1, float rY1, float initialOpacity, float targetOpacity, bool fStuckFade)
    {
        Element* pe = *ppe;
        if (pe)
        {
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            *pflDelay *= (animCoef / 100.0f);
            *pflDuration *= (animCoef / 100.0f);
            rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
            rgTrans[transIndex].nFlags = fStuckFade ? 0xD : 0x9;
            rgTrans[transIndex].nProperty = 2;
            rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
            rgTrans[transIndex].flDelay = *pflDelay;
            rgTrans[transIndex].flDuration = *pflDuration;
            rgTrans[transIndex].Curve.ptfl1.x = rX0;
            rgTrans[transIndex].Curve.ptfl1.y = rY0;
            rgTrans[transIndex].Curve.ptfl2.x = rX1;
            rgTrans[transIndex].Curve.ptfl2.y = rY1;
            rgTrans[transIndex].vInitial.flScalar = initialOpacity;
            rgTrans[transIndex].vEnd.flScalar = targetOpacity;
            float flDEA = (DWMActive && g_clientAnim) ? *pflDuration * 1000 : 0.0f;
        }
    }
    void _TriggerScaleIn(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float* pflDelay, float* pflDuration,
        float rX0, float rY0, float rX1, float rY1, float initialScaleX, float initialScaleY, float initialOriginX, float initialOriginY,
        float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY)
    {
        Element* pe = *ppe;
        if (pe)
        {
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            *pflDelay *= (animCoef / 100.0f);
            *pflDuration *= (animCoef / 100.0f);
            rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
            rgTrans[transIndex].nFlags = 0x201;
            rgTrans[transIndex].nProperty = 3;
            rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
            rgTrans[transIndex].flDelay = *pflDelay;
            rgTrans[transIndex].flDuration = *pflDuration;
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
        }
    }
    void _TriggerScaleOut(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float* pflDelay, float* pflDuration,
        float rX0, float rY0, float rX1, float rY1, float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY)
    {
        Element* pe = *ppe;
        if (pe)
        {
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            *pflDelay *= (animCoef / 100.0f);
            *pflDuration *= (animCoef / 100.0f);
            rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
            rgTrans[transIndex].nFlags = 0x204;
            rgTrans[transIndex].nProperty = 3;
            rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
            rgTrans[transIndex].flDelay = *pflDelay;
            rgTrans[transIndex].flDuration = *pflDuration;
            rgTrans[transIndex].Curve.ptfl1.x = rX0;
            rgTrans[transIndex].Curve.ptfl1.y = rY0;
            rgTrans[transIndex].Curve.ptfl2.x = rX1;
            rgTrans[transIndex].Curve.ptfl2.y = rY1;
            rgTrans[transIndex].vEnd.flX = targetScaleX;
            rgTrans[transIndex].vEnd.flY = targetScaleY;
            rgTrans[transIndex].vEnd.flOriginX = targetOriginX;
            rgTrans[transIndex].vEnd.flOriginY = targetOriginY;
        }
    }
    void _TriggerRotate(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float* pflDelay, float* pflDuration,
        float rX0, float rY0, float rX1, float rY1, float initialAngle, float targetAngle, float targetOriginX, float targetOriginY)
    {
        Element* pe = *ppe;
        if (pe)
        {
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            *pflDelay *= (animCoef / 100.0f);
            *pflDuration *= (animCoef / 100.0f);
            rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
            rgTrans[transIndex].nFlags = 0x201;
            rgTrans[transIndex].nProperty = 4;
            rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
            rgTrans[transIndex].flDelay = *pflDelay;
            rgTrans[transIndex].flDuration = *pflDuration;
            rgTrans[transIndex].Curve.ptfl1.x = rX0;
            rgTrans[transIndex].Curve.ptfl1.y = rY0;
            rgTrans[transIndex].Curve.ptfl2.x = rX1;
            rgTrans[transIndex].Curve.ptfl2.y = rY1;
            rgTrans[transIndex].vInitial.flScalar = initialAngle;
            rgTrans[transIndex].vEnd.flScalar = targetAngle;
            rgTrans[transIndex].vEnd.flOriginX = targetOriginX;
            rgTrans[transIndex].vEnd.flOriginY = targetOriginY;
        }
    }
    void _TriggerSkew(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float* pflDelay, float* pflDuration,
        float rX0, float rY0, float rX1, float rY1, float initialAngleX, float initialAngleY, float targetAngleX, float targetAngleY)
    {
        Element* pe = *ppe;
        if (pe)
        {
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            *pflDelay *= (animCoef / 100.0f);
            *pflDuration *= (animCoef / 100.0f);
            rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
            rgTrans[transIndex].nFlags = 0x201;
            rgTrans[transIndex].nProperty = 5;
            rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
            rgTrans[transIndex].flDelay = *pflDelay;
            rgTrans[transIndex].flDuration = *pflDuration;
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
        }
    }
    void _TriggerClip(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float* pflDelay, float* pflDuration,
        float rX0, float rY0, float rX1, float rY1, float initialLeft, float initialTop, float initialRight, float initialBottom,
        float targetLeft, float targetTop, float targetRight, float targetBottom)
    {
        Element* pe = *ppe;
        if (pe)
        {
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            *pflDelay *= (animCoef / 100.0f);
            *pflDuration *= (animCoef / 100.0f);
            rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
            rgTrans[transIndex].nFlags = 0x201;
            rgTrans[transIndex].nProperty = 6;
            rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
            rgTrans[transIndex].flDelay = *pflDelay;
            rgTrans[transIndex].flDuration = *pflDuration;
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
            float flDEA = (DWMActive && g_clientAnim) ? *pflDuration * 1000 : 0.0f;
        }
    }
    void _TriggerRotate3D(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float* pflDelay, float* pflDuration,
        float rX0, float rY0, float rX1, float rY1, float initialEulerAngleX, float initialEulerAngleY, float initialEulerAngleZ,
        float targetEulerAngleX, float targetEulerAngleY, float targetEulerAngleZ, float targetOriginAxisX, float targetOriginAxisY, float targetOriginAxisZ)
    {
        Element* pe = *ppe;
        if (pe)
        {
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            *pflDelay *= (animCoef / 100.0f);
            *pflDuration *= (animCoef / 100.0f);
            rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
            rgTrans[transIndex].nFlags = 0x201;
            rgTrans[transIndex].nProperty = 9;
            rgTrans[transIndex].dwTicket = GetGadgetTicket(pe->GetDisplayNode());
            rgTrans[transIndex].flDelay = *pflDelay;
            rgTrans[transIndex].flDuration = *pflDuration;
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
            float flDEA = (DWMActive && g_clientAnim) ? *pflDuration * 1000 : 0.0f;
        }
    }

    void _TriggerCrossfade(Element** ppe, float flDelay, float flDuration, Element** ppeCloneOut)
    {
        Element* pe = *ppe;
        if (pe)
        {
            if (pe->GetVisible())
            {
                Element* peClone{};
                Element::Create(0, pe->GetRoot(), nullptr, &peClone);
                pe->GetRoot()->Add(&peClone, 1);
                AddLayeredRef(peClone->GetDisplayNode());
                SetGadgetFlags(peClone->GetDisplayNode(), NULL, NULL);
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
                TriggerFade(*ppe, rgTrans, 0, flDelay, flDuration * 0.9f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                TriggerFade(peClone, rgTrans, 1, flDelay + flDuration * 0.1f, flDuration, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, false);
                // Give the element some time to get destroyed
                TriggerScaleOut(peClone, rgTrans, 2, 0.0f, max(0.2f, flDuration + 0.05f), 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, false, true);
                ScheduleGadgetTransitions_DWMCheck(0, 3, rgTrans, nullptr, &tsbInfo);
                if (ppeCloneOut)
                    *ppeCloneOut = peClone;
            }
        }
    }

    void TriggerTranslate(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialPosX, float initialPosY, float targetPosX, float targetPosY, bool fHide, bool fDestroy, bool fAutoPos)
    {
        _TriggerTranslate(&pe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialPosX, initialPosY, targetPosX, targetPosY, fAutoPos);
        CheckHideDestroy(pe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerFade(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialOpacity, float targetOpacity, bool fHide, bool fDestroy, bool fStuckFade)
    {
        _TriggerFade(&pe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialOpacity, targetOpacity, fStuckFade);
        CheckHideDestroy(pe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerScaleIn(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialScaleX, float initialScaleY, float initialOriginX, float initialOriginY,
        float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        _TriggerScaleIn(&pe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialScaleX, initialScaleY, initialOriginX, initialOriginY,
            targetScaleX, targetScaleY, targetOriginX, targetOriginY);
        CheckHideDestroy(pe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerScaleOut(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        _TriggerScaleOut(&pe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, targetScaleX, targetScaleY, targetOriginX, targetOriginY);
        CheckHideDestroy(pe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerRotate(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialAngle, float targetAngle, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        _TriggerRotate(&pe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialAngle, targetAngle, targetOriginX, targetOriginY);
        CheckHideDestroy(pe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerSkew(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialAngleX, float initialAngleY, float targetAngleX, float targetAngleY, bool fHide, bool fDestroy)
    {
        _TriggerSkew(&pe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialAngleX, initialAngleY, targetAngleX, targetAngleY);
        CheckHideDestroy(pe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerClip(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialLeft, float initialTop, float initialRight, float initialBottom,
        float targetLeft, float targetTop, float targetRight, float targetBottom, bool fHide, bool fDestroy)
    {
        _TriggerClip(&pe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialLeft, initialTop, initialRight, initialBottom,
            targetLeft, targetTop, targetRight, targetBottom);
        CheckHideDestroy(pe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerRotate3D(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialEulerAngleX, float initialEulerAngleY, float initialEulerAngleZ,
        float targetEulerAngleX, float targetEulerAngleY, float targetEulerAngleZ, float targetOriginAxisX, float targetOriginAxisY, float targetOriginAxisZ, bool fHide, bool fDestroy)
    {
        _TriggerRotate3D(&pe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialEulerAngleX, initialEulerAngleY, initialEulerAngleZ,
            targetEulerAngleX, targetEulerAngleY, targetEulerAngleZ, targetOriginAxisX, targetOriginAxisY, targetOriginAxisZ);
        CheckHideDestroy(pe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerCrossfade(Element* pe, float flDelay, float flDuration, Element** ppeCloneOut)
    {
        _TriggerCrossfade(&pe, flDelay, flDuration, ppeCloneOut);
    }

    void TriggerTranslate_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialPosX, float initialPosY, float targetPosX, float targetPosY, bool fHide, bool fDestroy, bool fAutoPos)
    {
        _TriggerTranslate(ppe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialPosX, initialPosY, targetPosX, targetPosY, fAutoPos);
        CheckHideDestroy_Ref(ppe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerFade_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialOpacity, float targetOpacity, bool fHide, bool fDestroy, bool fStuckFade)
    {
        _TriggerFade(ppe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialOpacity, targetOpacity, fStuckFade);
        CheckHideDestroy_Ref(ppe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerScaleIn_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialScaleX, float initialScaleY, float initialOriginX, float initialOriginY,
        float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        _TriggerScaleIn(ppe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialScaleX, initialScaleY, initialOriginX, initialOriginY,
            targetScaleX, targetScaleY, targetOriginX, targetOriginY);
        CheckHideDestroy_Ref(ppe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerScaleOut_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        _TriggerScaleOut(ppe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, targetScaleX, targetScaleY, targetOriginX, targetOriginY);
        CheckHideDestroy_Ref(ppe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerRotate_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialAngle, float targetAngle, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        _TriggerRotate(ppe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialAngle, targetAngle, targetOriginX, targetOriginY);
        CheckHideDestroy_Ref(ppe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerSkew_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialAngleX, float initialAngleY, float targetAngleX, float targetAngleY, bool fHide, bool fDestroy)
    {
        _TriggerSkew(ppe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialAngleX, initialAngleY, targetAngleX, targetAngleY);
        CheckHideDestroy_Ref(ppe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerClip_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialLeft, float initialTop, float initialRight, float initialBottom,
        float targetLeft, float targetTop, float targetRight, float targetBottom, bool fHide, bool fDestroy)
    {
        _TriggerClip(ppe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialLeft, initialTop, initialRight, initialBottom,
            targetLeft, targetTop, targetRight, targetBottom);
        CheckHideDestroy_Ref(ppe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerRotate3D_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialEulerAngleX, float initialEulerAngleY, float initialEulerAngleZ,
        float targetEulerAngleX, float targetEulerAngleY, float targetEulerAngleZ, float targetOriginAxisX, float targetOriginAxisY, float targetOriginAxisZ, bool fHide, bool fDestroy)
    {
        _TriggerRotate3D(ppe, rgTrans, transIndex, &flDelay, &flDuration, rX0, rY0, rX1, rY1, initialEulerAngleX, initialEulerAngleY, initialEulerAngleZ,
            targetEulerAngleX, targetEulerAngleY, targetEulerAngleZ, targetOriginAxisX, targetOriginAxisY, targetOriginAxisZ);
        CheckHideDestroy_Ref(ppe, flDelay, flDuration, fHide, fDestroy);
    }
    void TriggerCrossfade_Ref(Element** ppe, float flDelay, float flDuration, Element** ppeCloneOut)
    {
        _TriggerCrossfade(ppe, flDelay, flDuration, ppeCloneOut);
    }


    SimpleCubicBezierInterpolator::SimpleCubicBezierInterpolator() : _rX0(0), _rY0(0), _rX1(1), _rY1(1)
    {
        for (int i = 0; i < 11; i++)
            _presetProgression[i] = 0.1 * i;
    }

    SimpleCubicBezierInterpolator::SimpleCubicBezierInterpolator(double rX0, double rY0, double rX1, double rY1)
    {
        this->SetCurve(rX0, rX1, rY0, rY1);
    }

    double SimpleCubicBezierInterpolator::GetProgression(double x)
    {
        if (x <= 0.0) return 0.0;
        if (x >= 1.0) return 1.0;

        double intervalStart = 0.0;
        int currentSample = 1;
        while (currentSample < 11 && _presetProgression[currentSample] <= x)
        {
            intervalStart += 0.1;
            currentSample++;
        }
        currentSample--;

        double dist = (x - _presetProgression[currentSample]) / (_presetProgression[currentSample + 1] - _presetProgression[currentSample]);
        double estimate = intervalStart + dist * 0.1;

        double t = _SolveForT(x, estimate);
        return _CalculateCoordRelativeToT(t, _rY0, _rY1);
    }

    void SimpleCubicBezierInterpolator::SetCurve(double rX0, double rY0, double rX1, double rY1)
    {
        _rX0 = rX0;
        _rY0 = rY0;
        _rX1 = rX1;
        _rY1 = rY1;
        for (int i = 0; i < 11; i++)
            _presetProgression[i] = _CalculateCoordRelativeToT(0.1 * i, rX0, rX1);
    }

    double SimpleCubicBezierInterpolator::_SimplePower2(double value)
    {
        return value * value;
    }

    double SimpleCubicBezierInterpolator::_SimplePower3(double value)
    {
        return value * value * value;
    }

    double SimpleCubicBezierInterpolator::_CalculateCoordRelativeToT(double t, double param1, double param2)
    {
        return 3.0 * t * _SimplePower2(1 - t) * param1 + 3.0 * _SimplePower2(t) * (1 - t) * param2 + _SimplePower3(t);
    }

    double SimpleCubicBezierInterpolator::_DifferentiateCoordRelativeToT(double t, double param1, double param2)
    {
        return 3.0 * _SimplePower2(1 - t) * param1 + 6.0 * t * (1 - t) * (param2 - param1) + 3.0 * _SimplePower2(t) * (1 - param2);
    }

    double SimpleCubicBezierInterpolator::_SolveForT(double x, double estimate)
    {
        for (int i = 0; i < 4; i++)
        {
            double derv = _DifferentiateCoordRelativeToT(estimate, _rX0, _rX1);
            if (derv == 0.0) return estimate;
            double currentX = _CalculateCoordRelativeToT(estimate, _rX0, _rX1) - x;
            estimate -= currentX / derv;
        }
        return estimate;
    }
} 