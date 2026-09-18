#include "config.h"

namespace Config {

static float GetFloat(const char* sec, const char* key, float def, const char* path) {
	char buf[32];
	GetPrivateProfileStringA(sec, key, "", buf, sizeof(buf), path);
	if (!buf[0]) return def;
	float value = (float)atof(buf);
	return _finite(value) ? value : def;
}

static int GetInt(const char* sec, const char* key, int def, const char* path) {
	return GetPrivateProfileIntA(sec, key, def, path);
}

void Load(const char* path) {
	fImpulse = GetFloat("General", "fImpulse", fImpulse, path);
	fStiffness = GetFloat("General", "fStiffness", fStiffness, path);
	fDamping = GetFloat("General", "fDamping", fDamping, path);
	fDecay = GetFloat("General", "fDecay", fDecay, path);
	fPitchScale = GetFloat("General", "fPitchScale", fPitchScale, path);
	fRollScale = GetFloat("General", "fRollScale", fRollScale, path);
	fYawScale = GetFloat("General", "fYawScale", fYawScale, path);
	fMaxDeg = GetFloat("General", "fMaxDeg", fMaxDeg, path);
	fMaxHeadDeg = GetFloat("General", "fMaxHeadDeg", fMaxHeadDeg, path);
	fCullDist = GetFloat("General", "fCullDist", fCullDist, path);
	fArmMult = GetFloat("General", "fArmMult", fArmMult, path);
	fArmPassive = GetFloat("General", "fArmPassive", fArmPassive, path);
	fForearmPassive = GetFloat("General", "fForearmPassive", fForearmPassive, path);
	fArmHit = GetFloat("General", "fArmHit", fArmHit, path);
	fForearmHit = GetFloat("General", "fForearmHit", fForearmHit, path);
	fClavicleHit = GetFloat("General", "fClavicleHit", fClavicleHit, path);
	fLegMult = GetFloat("General", "fLegMult", fLegMult, path);
	fThighPassive = GetFloat("General", "fThighPassive", fThighPassive, path);
	fCalfPassive = GetFloat("General", "fCalfPassive", fCalfPassive, path);
	fThighHit = GetFloat("General", "fThighHit", fThighHit, path);
	fCalfHit = GetFloat("General", "fCalfHit", fCalfHit, path);
	fPelvisHit = GetFloat("General", "fPelvisHit", fPelvisHit, path);
	fKneeBuckleMult = GetFloat("General", "fKneeBuckleMult", fKneeBuckleMult, path);
	fHPFloor = GetFloat("Variation", "fHPFloor", fHPFloor, path);
	bEnablePosture = GetInt("Posture", "bEnablePosture", bEnablePosture ? 1 : 0, path) != 0;
	fPostureMaxDeg = GetFloat("Posture", "fPostureMaxDeg", fPostureMaxDeg, path);
	bEnableDeathTremor = GetInt("Death", "bEnableDeathTremor", bEnableDeathTremor ? 1 : 0, path) != 0;
	bEnablePostDeathTremor = GetInt("Death", "bEnablePostDeathTremor", bEnablePostDeathTremor ? 1 : 0, path) != 0;
	fPostDeathTremorDuration = GetFloat("Death", "fPostDeathTremorDuration", fPostDeathTremorDuration, path);
	fPostDeathTremorMult = GetFloat("Death", "fPostDeathTremorMult", fPostDeathTremorMult, path);
	fDeathTremorDuration = GetFloat("Death", "fDeathTremorDuration", fDeathTremorDuration, path);
	fDeathTremorStrength = GetFloat("Death", "fDeathTremorStrength", fDeathTremorStrength, path);
	fDeathTremorChance = GetFloat("Death", "fDeathTremorChance", fDeathTremorChance, path);
	fDeathTremorDelayMin = GetFloat("Death", "fDeathTremorDelayMin", fDeathTremorDelayMin, path);
	fDeathTremorDelayMax = GetFloat("Death", "fDeathTremorDelayMax", fDeathTremorDelayMax, path);
	fRandomMin = GetFloat("Variation", "fRandomMin", fRandomMin, path);
	fRandomMax = GetFloat("Variation", "fRandomMax", fRandomMax, path);
	fSkipThreshold = GetFloat("Variation", "fSkipThreshold", fSkipThreshold, path);
	fSkipChance = GetFloat("Variation", "fSkipChance", fSkipChance, path);
	fHeadSkipChance = GetFloat("Variation", "fHeadSkipChance", fHeadSkipChance, path);
	fHitCooldown = GetFloat("Variation", "fHitCooldown", fHitCooldown, path);
	fArmorAbsorb = GetFloat("Variation", "fArmorAbsorb", fArmorAbsorb, path);
	fRefDmg = GetFloat("Variation", "fRefDmg", fRefDmg, path);
	fHPCap = GetFloat("Variation", "fHPCap", fHPCap, path);
	fReactionChance = GetFloat("Variation", "fReactionChance", fReactionChance, path);
	fHeadFollow = GetFloat("General", "fHeadFollow", fHeadFollow, path);
	bAffectPlayer = GetInt("General", "bAffectPlayer", bAffectPlayer ? 1 : 0, path) != 0;

	fImpulse = fmaxf(fImpulse, 0.0f);
	fStiffness = fmaxf(fStiffness, 0.0f);
	fDamping = fmaxf(fDamping, 0.0f);
	fDecay = fmaxf(fDecay, 0.0f);
	fMaxDeg = fmaxf(fMaxDeg, 0.0f);
	fMaxHeadDeg = fmaxf(fMaxHeadDeg, 0.0f);
	fCullDist = fmaxf(fCullDist, 0.0f);
	fArmMult = fmaxf(fArmMult, 0.0f);
	fArmPassive = fmaxf(fArmPassive, 0.0f);
	fForearmPassive = fmaxf(fForearmPassive, 0.0f);
	fArmHit = fmaxf(fArmHit, 0.0f);
	fForearmHit = fmaxf(fForearmHit, 0.0f);
	fClavicleHit = fmaxf(fClavicleHit, 0.0f);
	fLegMult = fmaxf(fLegMult, 0.0f);
	fThighPassive = fmaxf(fThighPassive, 0.0f);
	fCalfPassive = fmaxf(fCalfPassive, 0.0f);
	fThighHit = fmaxf(fThighHit, 0.0f);
	fCalfHit = fmaxf(fCalfHit, 0.0f);
	fPelvisHit = fmaxf(fPelvisHit, 0.0f);
	fKneeBuckleMult = fmaxf(fKneeBuckleMult, 0.0f);
	fHeadFollow = Clampf(fHeadFollow, 0.0f, 1.0f);
	fHPFloor = Clampf(fHPFloor, 0.0f, 1.5f);
	fPostureMaxDeg = fmaxf(fPostureMaxDeg, 0.0f);
	fPostDeathTremorDuration = fmaxf(fPostDeathTremorDuration, 0.0f);
	fPostDeathTremorMult = fmaxf(fPostDeathTremorMult, 0.0f);
	fDeathTremorDuration = fmaxf(fDeathTremorDuration, 0.0f);
	fDeathTremorStrength = fmaxf(fDeathTremorStrength, 0.0f);
	fDeathTremorChance = Clampf(fDeathTremorChance, 0.0f, 1.0f);
	fDeathTremorDelayMin = fmaxf(fDeathTremorDelayMin, 0.0f);
	fDeathTremorDelayMax = fmaxf(fDeathTremorDelayMax, 0.0f);
	if (fDeathTremorDelayMin > fDeathTremorDelayMax) {
		float swap = fDeathTremorDelayMin;
		fDeathTremorDelayMin = fDeathTremorDelayMax;
		fDeathTremorDelayMax = swap;
	}
	fRandomMin = fmaxf(fRandomMin, 0.0f);
	fRandomMax = fmaxf(fRandomMax, 0.0f);
	if (fRandomMin > fRandomMax) {
		float swap = fRandomMin;
		fRandomMin = fRandomMax;
		fRandomMax = swap;
	}
	fSkipThreshold = Clampf(fSkipThreshold, 0.0f, 1.0f);
	fSkipChance = Clampf(fSkipChance, 0.0f, 1.0f);
	fHeadSkipChance = Clampf(fHeadSkipChance, 0.0f, 1.0f);
	fHitCooldown = fmaxf(fHitCooldown, 0.0f);
	fArmorAbsorb = Clampf(fArmorAbsorb, 0.0f, 1.0f);
	fRefDmg = fmaxf(fRefDmg, 0.001f);
	fHPCap = fmaxf(fHPCap, 0.0f);
	fReactionChance = Clampf(fReactionChance, 0.0f, 1.0f);
}

} //namespace Config
