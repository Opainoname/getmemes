#pragma once

#include "../UTILS/offsets.h"
#include "../UTILS/interfaces.h"


namespace SDK
{
	class CSWeaponInfo
	{
	public:
		virtual ~CSWeaponInfo() {};
		char*		consoleName;			// 0x0004
		char		pad_0008[12];			// 0x0008
		int			iMaxClip1;				// 0x0014
		int			iMaxClip2;				// 0x0018
		int			iDefaultClip1;			// 0x001C
		int			iDefaultClip2;			// 0x0020
		char		pad_0024[8];			// 0x0024
		char*		szWorldModel;			// 0x002C
		char*		szViewModel;			// 0x0030
		char*		szDroppedModel;			// 0x0034
		char		pad_0038[4];			// 0x0038
		char*		N0000023E;				// 0x003C
		char		pad_0040[56];			// 0x0040
		char*		szEmptySound;			// 0x0078
		char		pad_007C[4];			// 0x007C
		char*		szBulletType;			// 0x0080
		char		pad_0084[4];			// 0x0084
		char*		szHudName;				// 0x0088
		char*		szWeaponName;			// 0x008C
		char		pad_0090[56];			// 0x0090
		int 		WeaponType;				// 0x00C8
		int			iWeaponPrice;			// 0x00CC
		int			iKillAward;				// 0x00D0
		char*		szAnimationPrefix;		// 0x00D4
		float		flCycleTime;			// 0x00D8
		float		flCycleTimeAlt;			// 0x00DC
		float		flTimeToIdle;			// 0x00E0
		float		flIdleInterval;			// 0x00E4
		bool		bFullAuto;				// 0x00E8
		char		pad_0x00E5[3];			// 0x00E9
		int			iDamage;				// 0x00EC
		float		flArmorRatio;			// 0x00F0
		int			iBullets;				// 0x00F4
		float		flPenetration;			// 0x00F8
		float		flFlinchVelocityModifierLarge;	// 0x00FC
		float		flFlinchVelocityModifierSmall;	// 0x0100
		float		flRange;				// 0x0104
		float		flRangeModifier;		// 0x0108
		float		flThrowVelocity;		// 0x010C
		char		pad_0x010C[12];			// 0x0110
		bool		bHasSilencer;			// 0x011C
		char		pad_0x0119[3];			// 0x011D
		char*		pSilencerModel;			// 0x0120
		int			iCrosshairMinDistance;	// 0x0124
		int			iCrosshairDeltaDistance;// 0x0128 - iTeam?
		float		flMaxPlayerSpeed;		// 0x012C
		float		flMaxPlayerSpeedAlt;	// 0x0130
		float		flSpread;				// 0x0134
		float		flSpreadAlt;			// 0x0138
		float		flInaccuracyCrouch;		// 0x013C
		float		flInaccuracyCrouchAlt;	// 0x0140
		float		flInaccuracyStand;		// 0x0144
		float		flInaccuracyStandAlt;	// 0x0148
		float		flInaccuracyJumpInitial;// 0x014C
		float		flInaccuracyJump;		// 0x0150
		float		flInaccuracyJumpAlt;	// 0x0154
		float		flInaccuracyLand;		// 0x0158
		float		flInaccuracyLandAlt;	// 0x015C
		float		flInaccuracyLadder;		// 0x0160
		float		flInaccuracyLadderAlt;	// 0x0164
		float		flInaccuracyFire;		// 0x0168
		float		flInaccuracyFireAlt;	// 0x016C
		float		flInaccuracyMove;		// 0x0170
		float		flInaccuracyMoveAlt;	// 0x0174
		float		flInaccuracyReload;		// 0x0178
		int			iRecoilSeed;			// 0x017C
		float		flRecoilAngle;			// 0x0180
		float		flRecoilAngleAlt;		// 0x0184
		float		flRecoilAngleVariance;	// 0x0188
		float		flRecoilAngleVarianceAlt;	// 0x018C
		float		flRecoilMagnitude;		// 0x0190
		float		flRecoilMagnitudeAlt;	// 0x0194
		float		flRecoilMagnitudeVariance;	// 0x0198
		float		flRecoilMagnitudeVarianceAlt;	// 0x019C
		float		flRecoveryTimeCrouch;	// 0x01A0
		float		flRecoveryTimeStand;	// 0x01A4
		float		flRecoveryTimeCrouchFinal;	// 0x01A8
		float		flRecoveryTimeStandFinal;	// 0x01AC
		int			iRecoveryTransitionStartBullet;// 0x01B0 
		int			iRecoveryTransitionEndBullet;	// 0x01B4
		bool		bUnzoomAfterShot;		// 0x01B8
		bool		bHideViewModelZoomed;	// 0x01B9
		char		pad_0x01B5[2];			// 0x01BA
		char		iZoomLevels[3];			// 0x01BC
		int			iZoomFOV[2];			// 0x01C0
		float		fZoomTime[3];			// 0x01C4
		char*		szWeaponClass;			// 0x01D4
		float		flAddonScale;			// 0x01D8
		char		pad_0x01DC[4];			// 0x01DC
		char*		szEjectBrassEffect;		// 0x01E0
		char*		szTracerEffect;			// 0x01E4
		int			iTracerFrequency;		// 0x01E8
		int			iTracerFrequencyAlt;	// 0x01EC
		char*		szMuzzleFlashEffect_1stPerson; // 0x01F0
		char		pad_0x01F4[4];			 // 0x01F4
		char*		szMuzzleFlashEffect_3rdPerson; // 0x01F8
		char		pad_0x01FC[4];			// 0x01FC
		char*		szMuzzleSmokeEffect;	// 0x0200
		float		flHeatPerShot;			// 0x0204
		char*		szZoomInSound;			// 0x0208
		char*		szZoomOutSound;			// 0x020C
		float		flInaccuracyPitchShift;	// 0x0210
		float		flInaccuracySoundThreshold;	// 0x0214
		float		flBotAudibleRange;		// 0x0218
		char		pad_0x0218[8];			// 0x0220
		char*		pWrongTeamMsg;			// 0x0224
		bool		bHasBurstMode;			// 0x0228
		char		pad_0x0225[3];			// 0x0229
		bool		bIsRevolver;			// 0x022C
		bool		bCannotShootUnderwater;	// 0x0230
	};

	struct Weapon_Info
	{
		char pad00[0xC8];
		int m_WeaponType;		// 0xC8
		char padCC[0x20];
		int m_Damage;			// 0xEC
		float m_ArmorRatio;		// 0xF0
		char padF4[0x4];
		float m_Penetration;	// 0xF8
		char padFC[0x8];
		float m_Range;			// 0x104
		float m_RangeModifier;	// 0x108
		char pad10C[0x10];
		bool m_HasSilencer;		// 0x11C
	};

	/*enum class ItemDefinitionIndex : int
	{
		INVALID = -1,
		WEAPON_DEAGLE = 1,
		WEAPON_ELITE = 2,
		WEAPON_FIVESEVEN = 3,
		WEAPON_GLOCK = 4,
		WEAPON_AK47 = 7,
		WEAPON_AUG = 8,
		WEAPON_AWP = 9,
		WEAPON_FAMAS = 10,
		WEAPON_G3SG1 = 11,
		WEAPON_GALILAR = 13,
		WEAPON_M249 = 14,
		WEAPON_M4A1 = 16,
		WEAPON_MAC10 = 17,
		WEAPON_P90 = 19,
		WEAPON_UMP45 = 24,
		WEAPON_XM1014 = 25,
		WEAPON_BIZON = 26,
		WEAPON_MAG7 = 27,
		WEAPON_NEGEV = 28,
		WEAPON_SAWEDOFF = 29,
		WEAPON_TEC9 = 30,
		WEAPON_TASER = 31,
		WEAPON_HKP2000 = 32,
		WEAPON_MP7 = 33,
		WEAPON_MP9 = 34,
		WEAPON_NOVA = 35,
		WEAPON_P250 = 36,
		WEAPON_SCAR20 = 38,
		WEAPON_SG556 = 39,
		WEAPON_SSG08 = 40,
		WEAPON_KNIFE = 42,
		WEAPON_FLASHBANG = 43,
		WEAPON_HEGRENADE = 44,
		WEAPON_SMOKEGRENADE = 45,
		WEAPON_MOLOTOV = 46,
		WEAPON_DECOY = 47,
		WEAPON_INCGRENADE = 48,
		WEAPON_C4 = 49,
		WEAPON_KNIFE_T = 59,
		WEAPON_M4A1_SILENCER = 60,
		WEAPON_USP_SILENCER = 61,
		WEAPON_CZ75A = 63,
		WEAPON_REVOLVER = 64,
		WEAPON_KNIFE_BAYONET = 500,
		WEAPON_KNIFE_FLIP = 505,
		WEAPON_KNIFE_GUT = 506,
		WEAPON_KNIFE_KARAMBIT = 507,
		WEAPON_KNIFE_M9_BAYONET = 508,
		WEAPON_KNIFE_TACTICAL = 509,
		WEAPON_KNIFE_FALCHION = 512,
		WEAPON_KNIFE_SURVIVAL_BOWIE = 514,
		WEAPON_KNIFE_BUTTERFLY = 515,
		WEAPON_KNIFE_PUSH = 516,
		GLOVE_STUDDED_BLOODHOUND = 5027,
		GLOVE_T_SIDE = 5028,
		GLOVE_CT_SIDE = 5029,
		GLOVE_SPORTY = 5030,
		GLOVE_SLICK = 5031,
		GLOVE_LEATHER_WRAP = 5032,
		GLOVE_MOTORCYCLE = 5033,
		GLOVE_SPECIALIST = 5034,
		MAX_ITEMDEFINITIONINDEX
	};*/



	class CBaseWeapon
	{
	public:
		float GetNextPrimaryAttack()
		{
			return *reinterpret_cast<float*>(uintptr_t(this) + OFFSETS::m_flNextPrimaryAttack);
		}
		int GetItemDefenitionIndex()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_iItemDefinitionIndex);
		}
		int WeaponID()
		{
			if (this != nullptr)
				return *reinterpret_cast<int*>((DWORD)this + 0x2F88);
		}

		bool is_knife()
		{
			int iWeaponID = this->GetItemDefenitionIndex();
			return (iWeaponID == 42 || iWeaponID == 59
				|| iWeaponID == 59 || iWeaponID == 41
				|| iWeaponID == 500 || iWeaponID == 505 || iWeaponID == 506
				|| iWeaponID == 507 || iWeaponID == 508 || iWeaponID == 509
				|| iWeaponID == 515);
		}

		bool is_grenade()
		{
			if (!this)
				return false;

			int WeaponId = this->GetItemDefenitionIndex();

			if (!WeaponId)
				return false;

			if (WeaponId == 43 || WeaponId == 44 || WeaponId == 45 || WeaponId == 46 || WeaponId == 47 || WeaponId == 48)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		float GetPostponeFireReadyTime()
		{
			return *reinterpret_cast<float*>(uintptr_t(this) + 0x32C4);
		}
		float GetSpreadCone()
		{
			typedef float(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 437)(this);
		}
		float GetInaccuracy()
		{
			typedef float(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 468)(this); //469
		}
		int GetLoadedAmmo() 
		{
			return *(int*)((DWORD)this + 0x3204);
		}
		void UpdateAccuracyPenalty()
		{
			typedef void(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 469)(this); //470
		}
		Weapon_Info* GetWeaponInfo()
		{
			if (!this) return nullptr;
			typedef Weapon_Info*(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 446)(this);
		}
		CSWeaponInfo* get_full_info()
		{
			if (!this) return nullptr;
			typedef CSWeaponInfo*(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 445)(this);
		}
	};

	class CBaseCSGrenade : CBaseWeapon
	{
	public:

		float GetThrowTime()
		{
			uintptr_t thr = 0x3344;
			return *(float*)((uintptr_t)this + thr);
		}
	};
}