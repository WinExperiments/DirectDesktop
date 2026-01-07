#pragma once

using namespace DirectUI;

namespace DirectDesktop
{
	extern DWORD g_animCoef;
	extern bool g_AnimShiftKey;

	void TriggerTranslate(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialPosX, float initialPosY, float targetPosX, float targetPosY, bool fHide, bool fDestroy, bool fAutoPos);
	void TriggerFade(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialOpacity, float targetOpacity, bool fHide, bool fDestroy, bool fStuckFade);
	void TriggerScaleIn(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialScaleX, float initialScaleY, float initialOriginX, float initialOriginY,
		float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy);
	void TriggerScaleOut(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy);
	void TriggerRotate(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialAngle, float targetAngle, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy);
	void TriggerSkew(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1,	float initialAngleX, float initialAngleY, float targetAngleX, float targetAngleY, bool fHide, bool fDestroy);
	void TriggerClip(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialLeft, float initialTop, float initialRight, float initialBottom,
		float targetLeft, float targetTop, float targetRight, float targetBottom, bool fHide, bool fDestroy);
	void TriggerRotate3D(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialEulerAngleX, float initialEulerAngleY, float initialEulerAngleZ,
		float targetEulerAngleX, float targetEulerAngleY, float targetEulerAngleZ, float targetOriginAxisX, float targetOriginAxisY, float targetOriginAxisZ, bool fHide, bool fDestroy);
}