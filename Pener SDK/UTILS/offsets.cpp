#include "../includes.h"

#include "offsets.h"


namespace OFFSETS
{
	uintptr_t m_iHealth;
	uintptr_t m_fFlags;
	uintptr_t m_vecVelocity;
	uintptr_t m_flLowerBodyYawTarget;
	uintptr_t deadflag;
	uintptr_t m_vecOrigin;
	uintptr_t m_iTeamNum;
	uintptr_t m_nTickBase;
	uintptr_t m_bDormant;
	uintptr_t animstate;
	uintptr_t m_Collision;
	uintptr_t m_angEyeAngles;
	uintptr_t m_flSimulationTime;
	uintptr_t m_vecViewOffset;
	uintptr_t m_dwBoneMatrix;
	uintptr_t m_aimPunchAngle;
	uintptr_t m_bGunGameImmunity;
	uintptr_t m_nForceBone;
	uintptr_t m_flPoseParameter;
	uintptr_t dwGlowObjectManager;
	uintptr_t m_flNextPrimaryAttack;
	uintptr_t m_flNextAttack;
	uintptr_t m_hActiveWeapon;
	uintptr_t m_ArmorValue;
	uintptr_t m_bHasHelmet;
	uintptr_t m_iObserverMode;
	uintptr_t m_bIsScoped;
	uintptr_t m_iAccount;
	uintptr_t m_iPlayerC4;
	uintptr_t dwPlayerResource;
	uintptr_t m_iItemDefinitionIndex;

	void InitOffsets()
	{
		m_iHealth = 0x100;
		m_fFlags = 0x104;
		m_vecVelocity = 0x114;
		m_flLowerBodyYawTarget = 0x3A74;
		deadflag = 0x31D4;
		m_vecOrigin = 0x138;
		m_iTeamNum = 0xF4;
		m_nTickBase = 0x342C;
		m_bDormant = 0xED;
		animstate = 0x3900;
		m_Collision = 0x31C;
		m_angEyeAngles = 0xB32C;
		m_flSimulationTime = 0x268;
		m_vecViewOffset = 0x108;
		m_dwBoneMatrix = 0x26A8;
		m_aimPunchAngle = 0x302C;
		m_bGunGameImmunity = 0x3928;
		m_nForceBone = 0x268C;
		m_flPoseParameter = 0x2774;
		dwGlowObjectManager = 0x5223730;
		m_flNextPrimaryAttack = 0x3218;
		m_flNextAttack = 0x2D70;
		m_hActiveWeapon = 0x2EF8;
		m_ArmorValue = 0xB328;
		m_bHasHelmet = 0xB31C;
		m_iObserverMode = 0x3374;
		m_bIsScoped = 0x390A;
		m_iAccount = 0xB314;
		m_iPlayerC4 = 0x165C;
		dwPlayerResource = 0x3112F6C;
		m_iItemDefinitionIndex = 0x2FAA;
	}
}