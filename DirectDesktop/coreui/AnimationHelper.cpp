#include "pch.h"
#include "..\DirectDesktop.h"
#include "AnimationHelper.h"

using namespace DirectUI;

namespace DirectDesktop
{

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

    void TriggerTranslate(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialPosX, float initialPosY, float targetPosX, float targetPosY, bool fHide, bool fDestroy)
    {
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
        rgTrans[transIndex].vInitial.flX = initialPosX;
        rgTrans[transIndex].vInitial.flY = initialPosY;
        rgTrans[transIndex].vEnd.flX = targetPosX;
        rgTrans[transIndex].vEnd.flY = targetPosY;
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerFade(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialOpacity, float targetOpacity, bool fHide, bool fDestroy)
    {
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = 0x9;
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
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerScaleIn(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialScaleX, float initialScaleY, float initialOriginX, float initialOriginY,
        float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
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
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerScaleOut(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
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
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
    void TriggerClip(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
        float rX0, float rY0, float rX1, float rY1, float initialScaleX, float initialScaleY, float initialOriginX, float initialOriginY,
        float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy)
    {
        rgTrans[transIndex].hgadChange = pe->GetDisplayNode();
        rgTrans[transIndex].nFlags = 0x205;
        rgTrans[transIndex].nProperty = 6;
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
        if (fDestroy)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hDestroy = CreateThread(nullptr, 0, DestroyElement, dea, NULL, nullptr);
            if (hDestroy) CloseHandle(hDestroy);
        }
        else if (fHide)
        {
            DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(flDuration * 1000), pe };
            HANDLE hHide = CreateThread(nullptr, 0, HideElement, dea, NULL, nullptr);
            if (hHide) CloseHandle(hHide);
        }
    }
}