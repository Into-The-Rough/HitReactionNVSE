#pragma once
#include "common.h"

namespace Config {
	inline float fImpulse = 50.0f;
	inline float fStiffness = 156.0f;
	inline float fDamping = 8.0f;
	inline float fDecay = 3.0f;
	inline float fPitchScale = 1.0f;
	inline float fRollScale = 0.7f;
	inline float fYawScale = 0.3f;
	inline float fMaxDeg = 25.0f;
	inline float fMaxHeadDeg = 35.0f;
	inline float fCullDist = 4096.0f;
	inline float fArmMult = 2.0f;
	inline float fArmPassive = 0.4f;
	inline float fForearmPassive = 0.7f;
	inline float fArmHit = 1.3f;
	inline float fForearmHit = 1.6f;
	inline float fClavicleHit = 0.7f;
	inline float fLegMult = 2.0f;
	inline float fThighPassive = 0.2f;
	inline float fCalfPassive = 0.1f;
	inline float fThighHit = 1.25f;
	inline float fCalfHit = 0.9f;
	inline float fPelvisHit = 0.65f;
	inline float fKneeBuckleMult = 1.25f;
	inline float fHPFloor = 0.2f;
	inline bool bEnablePosture = false;
	inline float fPostureMaxDeg = 15.0f;
	inline bool bEnableDeathTremor = true;
	inline bool bEnablePostDeathTremor = true;
	inline float fPostDeathTremorDuration = 2.4f;
	inline float fPostDeathTremorMult = 1.0f;
	inline float fDeathTremorDuration = 2.4f;
	inline float fDeathTremorStrength = 5.0f;
	inline float fDeathTremorChance = 1.0f;
	inline float fDeathTremorDelayMin = 0.16f;
	inline float fDeathTremorDelayMax = 0.24f;
	inline float fRandomMin = 0.55f;
	inline float fRandomMax = 1.4f;
	inline float fSkipThreshold = 0.1f;
	inline float fSkipChance = 0.35f;
	inline float fHeadSkipChance = 0.1f;
	inline float fHitCooldown = 0.18f;
	inline float fArmorAbsorb = 0.5f;
	inline float fRefDmg = 50.0f;
	inline float fHPCap = 200.0f;
	inline float fReactionChance = 0.98f;
	inline float fHeadFollow = 0.2f;
	inline bool bAffectPlayer = false;

	void Load(const char* path);
}
