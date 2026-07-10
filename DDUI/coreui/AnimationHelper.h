#pragma once

#ifdef DDUI_EXPORTS
#define DDUIAPI __declspec(dllexport)
#else
#define DDUIAPI __declspec(dllimport)
#endif

using namespace DirectUI;

namespace DDUI
{
	DDUIAPI void TriggerTranslate(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialPosX, float initialPosY, float targetPosX, float targetPosY, bool fHide, bool fDestroy, bool fAutoPos);
	DDUIAPI void TriggerFade(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialOpacity, float targetOpacity, bool fHide, bool fDestroy, bool fStuckFade);
	DDUIAPI void TriggerScaleIn(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialScaleX, float initialScaleY, float initialOriginX, float initialOriginY,
		float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy);
	DDUIAPI void TriggerScaleOut(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy);
	DDUIAPI void TriggerRotate(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialAngle, float targetAngle, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy);
	DDUIAPI void TriggerSkew(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1,	float initialAngleX, float initialAngleY, float targetAngleX, float targetAngleY, bool fHide, bool fDestroy);
	DDUIAPI void TriggerClip(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialLeft, float initialTop, float initialRight, float initialBottom,
		float targetLeft, float targetTop, float targetRight, float targetBottom, bool fHide, bool fDestroy);
	DDUIAPI void TriggerRotate3D(Element* pe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialEulerAngleX, float initialEulerAngleY, float initialEulerAngleZ,
		float targetEulerAngleX, float targetEulerAngleY, float targetEulerAngleZ, float targetOriginAxisX, float targetOriginAxisY, float targetOriginAxisZ, bool fHide, bool fDestroy);
	DDUIAPI void TriggerCrossfade(Element* pe, float flDelay, float flDuration, Element** ppeCloneOut);

	DDUIAPI void TriggerTranslate_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialPosX, float initialPosY, float targetPosX, float targetPosY, bool fHide, bool fDestroy, bool fAutoPos);
	DDUIAPI void TriggerFade_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialOpacity, float targetOpacity, bool fHide, bool fDestroy, bool fStuckFade);
	DDUIAPI void TriggerScaleIn_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialScaleX, float initialScaleY, float initialOriginX, float initialOriginY,
		float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy);
	DDUIAPI void TriggerScaleOut_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float targetScaleX, float targetScaleY, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy);
	DDUIAPI void TriggerRotate_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialAngle, float targetAngle, float targetOriginX, float targetOriginY, bool fHide, bool fDestroy);
	DDUIAPI void TriggerSkew_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialAngleX, float initialAngleY, float targetAngleX, float targetAngleY, bool fHide, bool fDestroy);
	DDUIAPI void TriggerClip_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialLeft, float initialTop, float initialRight, float initialBottom,
		float targetLeft, float targetTop, float targetRight, float targetBottom, bool fHide, bool fDestroy);
	DDUIAPI void TriggerRotate3D_Ref(Element** ppe, GTRANS_DESC* rgTrans, UINT transIndex, float flDelay, float flDuration,
		float rX0, float rY0, float rX1, float rY1, float initialEulerAngleX, float initialEulerAngleY, float initialEulerAngleZ,
		float targetEulerAngleX, float targetEulerAngleY, float targetEulerAngleZ, float targetOriginAxisX, float targetOriginAxisY, float targetOriginAxisZ, bool fHide, bool fDestroy);
	DDUIAPI void TriggerCrossfade_Ref(Element** ppe, float flDelay, float flDuration, Element** ppeCloneOut);

	class SimpleCubicBezierInterpolator
	{
	public:
		DDUIAPI SimpleCubicBezierInterpolator();
		DDUIAPI SimpleCubicBezierInterpolator(double rX0, double rY0, double rX1, double rY1);
		DDUIAPI double GetProgression(double x);
		DDUIAPI void SetCurve(double rX0, double rY0, double rX1, double rY1);

	private:
		double _rX0, _rY0, _rX1, _rY1;
		double _presetProgression[11];
		inline double _SimplePower2(double value);
		inline double _SimplePower3(double value);
		inline double _CalculateCoordRelativeToT(double t, double param1, double param2);
		inline double _DifferentiateCoordRelativeToT(double t, double param1, double param2);
		inline double _SolveForT(double x, double estimate);
	};
}