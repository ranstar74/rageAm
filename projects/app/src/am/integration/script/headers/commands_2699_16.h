//
// File: commands_2699_16.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

// THIS FILE IS AUTOGENERATED

#if APP_BUILD_2699_16 | APP_BUILD_2699_16_RELEASE_NO_OPT

#include "../types.h"

// TODO: GET_ENTITY_MATRIX

inline void scrSetClockTime(int h, int m, int s)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xaa27c85e5be092aa);
	scrValue params[3];
	params[0].Int = h;
	params[1].Int = m;
	params[2].Int = s;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 3;
	info.Params = params;
	s_Handler(info);
}

inline int scrGetClockHours()
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x09fc827691dace59);
	int result;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 0;
	info.Params = nullptr;
	s_Handler(info);
	return result;
}

inline int scrGetClockMinutes()
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x80f97d7d29800a1a);
	int result;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 0;
	info.Params = nullptr;
	s_Handler(info);
	return result;
}

inline int scrGetClockSeconds()
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xaa2844cad88768b5);
	int result;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 0;
	info.Params = nullptr;
	s_Handler(info);
	return result;
}

inline scrShapetestStatus scrGetShapeTestResult(scrShapetestIndex shapeTestGuid, bool& bHitSomething, scrVector& vPos, scrVector& vNormal, scrEntityIndex& EntityIndex)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xb2d581bd7446bbe9);
	scrShapetestStatus result;
	scrValue params[5];
	int bHitSomething_;
	params[0].Int = shapeTestGuid;
	params[1].Reference = (scrValue*)&bHitSomething_;
	params[2].Reference = (scrValue*)&vPos;
	params[3].Reference = (scrValue*)&vNormal;
	params[4].Reference = (scrValue*)&EntityIndex;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 5;
	info.Params = params;
	s_Handler(info);
	bHitSomething = bHitSomething_;
	for (int i = 0; i < info.BufferCount; i++)
		*info.Orig[i] = info.Buffer[i];
	return result;
}

inline scrShapetestIndex scrStartShapeTestMouseCursorLosProbe(scrVector& vProbeStartPosOut, scrVector& vProbeEndPosOut, int LOSFlags = SCRIPT_INCLUDE_MOVER, scrEntityIndex ExcludeEntityIndex = {}, int Options = SCRIPT_SHAPETEST_OPTION_DEFAULT)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x34686dbb08c1704f);
	scrShapetestIndex result;
	scrValue params[5];
	params[0].Reference = (scrValue*)&vProbeStartPosOut;
	params[1].Reference = (scrValue*)&vProbeEndPosOut;
	params[2].Int = LOSFlags;
	params[3].Int = ExcludeEntityIndex;
	params[4].Int = Options;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 5;
	info.Params = params;
	s_Handler(info);
	for (int i = 0; i < info.BufferCount; i++)
		*info.Orig[i] = info.Buffer[i];
	return result;
}

inline scrCameraIndex scrCreateCamWithParams(const char* CameraName, scrVector vecPos, scrVector vecRot, float FOV = 65.0f, bool StartActivated = false, scrEulerRotOrder RotOrder = EULER_XYZ)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x6729fa3af971be2a);
	scrCameraIndex result;
	scrValue params[10];
	params[0].String = CameraName;
	params[1].Float = vecPos.X;
	params[2].Float = vecPos.Y;
	params[3].Float = vecPos.Z;
	params[4].Float = vecRot.X;
	params[5].Float = vecRot.Y;
	params[6].Float = vecRot.Z;
	params[7].Float = FOV;
	params[8].Int = StartActivated;
	params[9].Int = RotOrder;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 10;
	info.Params = params;
	s_Handler(info);
	return result;
}

inline bool scrIsCamActive(scrCameraIndex CameraIndex)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xa24fda4986456697);
	int result;
	scrValue params[1];
	params[0].Int = CameraIndex;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
	return result;
}

inline void scrCascadeShadowsSetCascadeBoundsScale(float scale)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x51c7ea47553be792);
	scrValue params[1];
	params[0].Float = scale;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
}

inline void scrCascadeShadowsSetAircraftMode(bool bEnable)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xea33835cb4cd38ac);
	scrValue params[1];
	params[0].Int = bEnable;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
}

inline void scrDestroyCam(scrCameraIndex CameraIndex, bool bShouldApplyAcrossAllThreads = false)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x588ddcb644c6486a);
	scrValue params[2];
	params[0].Int = CameraIndex;
	params[1].Int = bShouldApplyAcrossAllThreads;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
}

inline void scrUnlockMinimapAngle()
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x1c35fdd57f36fbea);
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 0;
	info.Params = nullptr;
	s_Handler(info);
}

inline void scrUnlockMinimapPosition()
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xbd18a1ef31f0166b);
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 0;
	info.Params = nullptr;
	s_Handler(info);
}

inline void scrRemoveBlip(scrBlipIndex& BlipId)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xffd8eb5462b07b59);
	scrValue params[1];
	params[0].Reference = (scrValue*)&BlipId;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
}

inline void scrCascadeShadowsInitSession()
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xa91ec7d49df9f229);
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 0;
	info.Params = nullptr;
	s_Handler(info);
}

inline void scrClearFocus()
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x5639e771f6085bf6);
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 0;
	info.Params = nullptr;
	s_Handler(info);
}

inline void scrSetCamActive(int cameraIndex, int activeState)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xdd786b89b15aa63f);
	scrValue params[2];
	params[0].Int = cameraIndex;
	params[1].Int = activeState;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetBlipSprite(scrBlipIndex blip, int sprite)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x1a5b5ba56167d412);
	scrValue params[2];
	params[0].Int = blip;
	params[1].Int = sprite;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetBlipColour(scrBlipIndex blip, int colour)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xa582ee8380437b1b);
	scrValue params[2];
	params[0].Int = blip;
	params[1].Int = colour;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetBlipFlashes(scrBlipIndex blip, bool bOnOff)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x9705014c37e60003);
	scrValue params[2];
	params[0].Int = blip;
	params[1].Int = bOnOff;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetBlipFlashInterval(scrBlipIndex blip, int timeInMilliseconds)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x1209f9274aff1a3d);
	scrValue params[2];
	params[0].Int = blip;
	params[1].Int = timeInMilliseconds;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetBlipScale(scrBlipIndex blip, float scale)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x293a9399e902a33b);
	scrValue params[2];
	params[0].Int = blip;
	params[1].Float = scale;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
}

inline scrBlipIndex scrAddBlipForCoord(scrVector VecCoors)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xc5b823372b67998a);
	scrBlipIndex result;
	scrValue params[3];
	params[0].Float = VecCoors.X;
	params[1].Float = VecCoors.Y;
	params[2].Float = VecCoors.Z;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 3;
	info.Params = params;
	s_Handler(info);
	return result;
}

inline void scrRenderScriptCams(
	bool setActive, bool doGameCanInterp, int duration = 3000, 
	bool shouldLockInterpolationSourceFrame = true,
	bool shouldApplyAcrossAllThreads = false,
	scrRenderingOptionFlags renderOptions = RO_NO_OPTIONS)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x850d4ef3d40fb068);
	scrValue params[6];
	params[0].Int = setActive;
	params[1].Int = doGameCanInterp;
	params[2].Int = duration;
	params[3].Int = shouldLockInterpolationSourceFrame;
	params[4].Int = shouldApplyAcrossAllThreads;
	params[5].Int = renderOptions;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 6;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetBlipCoords(int blipIndex, const scrVector& vecCoors)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xfb7acc9d9d6401a8);
	scrValue params[4];
	params[0].Int = blipIndex;
	params[1].Float = vecCoors.X;
	params[2].Float = vecCoors.Y;
	params[3].Float = vecCoors.Z;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 4;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetBlipRotation(int blipIndex, int degrees)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xb02954d4f69b7a25);
	scrValue params[2];
	params[0].Int = blipIndex;
	params[1].Int = degrees;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
}

inline void scrLockMinimapAngle(int angle)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xf3f07aaf13926729);
	scrValue params[1];
	params[0].Int = angle;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
}

inline int scrRound(float f)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xf2db717a73826179);
	int result;
	scrValue params[1];
	params[0].Float = f;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
	return result;
}

inline void scrLockMinimapPosition(float x, float y)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x262d43ebe3ca4397);
	scrValue params[2];
	params[0].Float = x;
	params[1].Float = y;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
}

inline float scrGetHeadingFromVector2D(float x, float y)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xd12efcab87804bed);
	float result;
	scrValue params[2];
	params[0].Float = x;
	params[1].Float = y;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
	return result;
}

inline void scrDisableAllControlActions(scrControlType control)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x58699da34f83b510);
	scrValue params[1];
	params[0].Int = control;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
}

inline void scrRequestScript(ConstString scriptName)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xaf76a37c80efd1d8);
	scrValue params[1];
	params[0].String = scriptName;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
}

inline scrThreadId scrStartNewScript(ConstString scriptName, int stackSize = 0)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xe81651ad79516e48);
	scrThreadId result;
	scrValue params[2];
	params[0].String = scriptName;
	params[1].Int = stackSize;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 2;
	info.Params = params;
	s_Handler(info);
	return result;
}

inline void scrTerminateThread(scrThreadId threadId)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x5b71484d4dac41e5);
	scrValue params[1];
	params[0].Int = threadId.Get();
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
}

// Computes square roof of a number
inline float scrSqrt(float f)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x71D93B57D07F9804);
	float result;
	scrValue params[1];
	params[0].Float = f;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
	return result;
}

/**
 * \brief The command will return TRUE if it finds collision, FALSE if not.
 * Tries to store the Z coordinate of the highest ground below the given point.
 * \param coors -
 * \param returnZ -
 * \param waterAsGround -
 * \param ignoreDistToWaterLevelCheck -
 * \return -
 */
inline bool scrGetGroundZFor3DCoord(const scrVector& coors, float& returnZ, bool waterAsGround = false, bool ignoreDistToWaterLevelCheck = false)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x9cd4cbf2bbe10f00);
	int result;
	scrValue params[6];
	params[0].Float = coors.X;
	params[1].Float = coors.Y;
	params[2].Float = coors.Z;
	params[3].Reference = (scrValue*)&returnZ;
	params[4].Int = waterAsGround;
	params[5].Int = ignoreDistToWaterLevelCheck;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 6;
	info.Params = params;
	s_Handler(info);
	return result;
}

/**
 * \brief Gets the player index for the main player.
 */
inline scrPedIndex scrPlayerGetID()
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xe2d3d51028f0428a);
	scrPedIndex result;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 0;
	info.Params = nullptr;
	s_Handler(info);
	return result;
}

inline void scrSetEntityCoordsNoOffset(scrObjectIndex entityIndex, const scrVector& newCoors, bool KeepTasks = false, bool KeepIK = false, bool doWarp = true)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xa539ede8da5bbc22);
	scrValue params[7];
	params[0].Int = entityIndex.Get();
	params[1].Float = newCoors.X;
	params[2].Float = newCoors.Y;
	params[3].Float = newCoors.Z;
	params[4].Int = KeepTasks;
	params[5].Int = KeepIK;
	params[6].Int = doWarp;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 7;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetEntityRotation(scrObjectIndex entityIndex, const scrVector& rot)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x8CE3D365F064F69E);
	scrValue params[6];
	params[0].Int = entityIndex.Get();
	params[1].Float = rot.X;
	params[2].Float = rot.Y;
	params[3].Float = rot.Z;
	params[4].Int = 0; // EULER_XYZ
	params[5].Int = 0; // doDeadCheck
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 6;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetEntityAsMissionEntity(scrObjectIndex entityIndex, bool scriptHostObject = true, bool grabFromOtherScript = false)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x2d58a6131679d82c);
	scrValue params[3];
	params[0].Int = entityIndex.Get();
	params[1].Int = scriptHostObject;
	params[2].Int = grabFromOtherScript;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 3;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetPedCoordsKeepVehicle(scrPedIndex pedIndex, const scrVector& newCoors)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xcb8859434b382fcc);
	scrValue params[4];
	params[0].Int = pedIndex.Get();
	params[1].Float = newCoors.X;
	params[2].Float = newCoors.Y;
	params[3].Float = newCoors.Z;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 4;
	info.Params = params;
	s_Handler(info);
}

inline void scrSetFocusPosAndVel(scrVector pos, scrVector vel)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x7d5c42a7cdb11db6);
	scrValue params[6];
	params[0].Float = pos.X;
	params[1].Float = pos.Y;
	params[2].Float = pos.Z;
	params[3].Float = vel.X;
	params[4].Float = vel.Y;
	params[5].Float = vel.Z;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 6;
	info.Params = params;
	s_Handler(info);
}

inline void scrUpdateLightsOnEntity(scrObjectIndex entityIndex)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0xc9d2355daf3fe0c3);
	scrValue params[1];
	params[0].Int = entityIndex;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
}

/**
 * \brief Create an object  with an offset (from the root the base) at the given coord.
 * \param model -
 * \param coors -
 * \param registerAsNetworkObject The new object will be created and synced on other machines if a network game is running
 * \param scriptHostObject If true, this object has been created by the host portion of a network script and is vital to that script - it must always exist regardless of who is hosting the script.
 * If false, the object has been created by the client portion of a network script and can be removed when the client who created it leaves the script session.
 * \param forceToBeObject If true, the object will always be forced to be an object type. This applies when creating an object that uses a door model. If this is false the object will be created as a door door type.
 * \return -
 */
inline scrObjectIndex scrCreateObject(u32 model, scrVector coors, bool registerAsNetworkObject = true, bool scriptHostObject = true, bool forceToBeObject = false)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x0e536d72ab30f4c8);
	scrObjectIndex result;
	scrValue params[7];
	params[0].Uns = model;
	params[1].Float = coors.X;
	params[2].Float = coors.Y;
	params[3].Float = coors.Z;
	params[4].Int = registerAsNetworkObject;
	params[5].Int = scriptHostObject;
	params[6].Int = forceToBeObject;
	scrInfo info;
	info.ResultPtr = (scrValue*)&result;
	info.ParamCount = 7;
	info.Params = params;
	s_Handler(info);
	return result;
}

inline void scrDeleteObject(scrObjectIndex& index)
{
	using namespace rageam::integration;
	static scrSignature s_Handler = scrLookupHandler(0x4bda5afd88c085eb);
	scrValue params[1];
	params[0].Reference = (scrValue*)&index;
	scrInfo info;
	info.ResultPtr = nullptr;
	info.ParamCount = 1;
	info.Params = params;
	s_Handler(info);
}

#endif
