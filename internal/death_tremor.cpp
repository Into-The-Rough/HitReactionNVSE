#include "hit_reaction.h"
#include "game.h"
#include "config.h"

namespace HitReaction {

static float Pulse(float t, float center, float halfWidth) {
	if (halfWidth <= 0.0f) return 0.0f;
	float x = 1.0f - fabsf(t - center) / halfWidth;
	if (x <= 0.0f) return 0.0f;
	return x * x * (3.0f - 2.0f * x);
}

//don't fight external ragdoll motion
bool CorpseSettled(State& s, void* rootNode) {
	float speedSq;
	if (!GetLinearSpeedSq(rootNode, speedSq) && !GetLinearSpeedSq(s.bones[kBone_Pelvis], speedSq))
		return true;
	float threshold = fmaxf(0.75f, s.lastRootLinMag * 1.5f);
	return speedSq <= threshold * threshold;
}

void ClearDeathTremor(State& s, void* rootNode) {
	bool settled = CorpseSettled(s, rootNode);
	s.lastRootLinMag = 0.0f;
	if (!settled) return;
	NiPoint3 zero = {0.0f, 0.0f, 0.0f};
	if (rootNode) {
		SetLinearVelocity(rootNode, zero);
		SetAngularVelocity(rootNode, zero);
	}
	SetLinearVelocity(s.bones[kBone_Pelvis], zero);
	SetAngularVelocity(s.bones[kBone_Pelvis], zero);
	SetAngularVelocity(s.bones[kBone_Head], zero);
	SetAngularVelocity(s.bones[kBone_LUpperArm], zero);
	SetAngularVelocity(s.bones[kBone_RUpperArm], zero);
	SetAngularVelocity(s.bones[kBone_LThigh], zero);
	SetAngularVelocity(s.bones[kBone_RThigh], zero);
}

//one roll per death
bool RollTremorChance(State& s) {
	if (!s.tremorRolled) {
		s.tremorRolled = true;
		if (Config::fDeathTremorChance >= 1.0f)
			s.tremorPassed = true;
		else if (Config::fDeathTremorChance <= 0.0f)
			s.tremorPassed = false;
		else
			s.tremorPassed = RandFloat() < Config::fDeathTremorChance;
	}
	return s.tremorPassed;
}

void PrimeDeathTremor(State& s, const NiPoint3& worldDir, float side) {
	s.dying = true;
	s.deathPrimeTimer = 0.6f;
	s.deathWorldDir = Normalize({worldDir.x, worldDir.y, 0.0f});
	s.deathSide = side >= 0.0f ? 1.0f : -1.0f;
}

void StartCorpseTremor(State& s, bool primed) {
	s.dying = false;
	s.dead = true;
	s.awaitingFinish = false;
	s.primedTremor = primed;
	float delayMin = Config::fDeathTremorDelayMin;
	float delayMax = Config::fDeathTremorDelayMax;
	s.deathDelay = (delayMax > delayMin) ? (delayMin + RandFloat() * (delayMax - delayMin)) : delayMin;
	float baseDuration = primed ? Config::fDeathTremorDuration : Config::fPostDeathTremorDuration;
	s.tremorDuration = baseDuration * (0.9f + RandFloat() * 0.35f);
	s.deathTimer = s.tremorDuration;
	s.tremorStrengthScale = 0.85f + RandFloat() * 0.45f;
	s.tremorPhase = (RandFloat() - 0.5f) * 0.08f;
	s.tremorVariant = (UInt32)(RandFloat() * 3.0f);
}

void OnFinishDying(void* actor) {
	if (!actor) return;
	if (!Config::bAffectPlayer && actor == *g_thePlayer) return;
	if (!IsCharacter(actor)) return;

	UInt32 refID = GetRefID(actor);
	State* found = FindState(refID);
	if (found) {
		State& s = *found;
		if (s.dead) return;
		s.awaitingFinish = false;
		if (s.dying) {
			if (Config::bEnableDeathTremor && Config::fDeathTremorDuration > 0.0f && RollTremorChance(s))
				StartCorpseTremor(s, true);
			else
				s.dying = false;
			return;
		}
		if (s.tremorRolled && !s.tremorPassed) return;
	}

	if (!Config::bEnablePostDeathTremor) return;
	State* state = GetOrCreateState(refID);
	if (!state) return;
	State& s = *state;
	if (!RollTremorChance(s)) return;
	s.deathWorldDir = {0.0f, 1.0f, 0.0f};
	s.deathSide = (refID & 1) ? 1.0f : -1.0f;
	StartCorpseTremor(s, false);
}

void ApplyDeathTremor(State& s, void* rootNode, float remaining, float duration, float scale) {
	bool enabled = s.primedTremor ? Config::bEnableDeathTremor : Config::bEnablePostDeathTremor;
	if (!enabled || duration <= 0.0f || scale <= 0.0f) return;
	if (!rootNode) return;

	float progressBase = Clampf((duration - remaining) / duration, 0.0f, 1.0f);
	float progress = Clampf(progressBase + s.tremorPhase, 0.0f, 1.0f);
	float envelope = Clampf(1.0f - progressBase * 0.55f, 0.0f, 1.0f);
	float driveA = 0.0f;
	float driveB = 0.0f;

	switch (s.tremorVariant % 3) {
	case 0: {
		float p1 = Pulse(progress, 0.08f, 0.045f);
		float p2 = Pulse(progress, 0.16f, 0.042f);
		float p3 = Pulse(progress, 0.25f, 0.040f);
		float p4 = Pulse(progress, 0.35f, 0.040f);
		float p5 = Pulse(progress, 0.46f, 0.038f);
		float p6 = Pulse(progress, 0.58f, 0.038f);
		float p7 = Pulse(progress, 0.70f, 0.036f);
		float p8 = Pulse(progress, 0.82f, 0.034f);
		float p9 = Pulse(progress, 0.92f, 0.032f);
		driveA = 1.28f * p1 - 1.14f * p2 + 1.02f * p3 - 0.94f * p4 + 0.84f * p5 - 0.76f * p6 + 0.68f * p7 - 0.56f * p8 + 0.44f * p9;
		driveB = 0.32f * p1 + 0.44f * p2 - 0.26f * p3 + 0.36f * p4 - 0.22f * p5 + 0.28f * p6 - 0.18f * p7 + 0.18f * p8 - 0.12f * p9;
		float flutter = sinf(progress * 136.0f) * 0.14f * envelope;
		driveA += flutter;
		driveB += flutter * 0.42f;
		break;
	}
	case 1: {
		float tonic = Pulse(progress, 0.08f, 0.12f);
		float c1 = Pulse(progress, 0.28f, 0.055f);
		float c2 = Pulse(progress, 0.41f, 0.05f);
		float c3 = Pulse(progress, 0.54f, 0.05f);
		float c4 = Pulse(progress, 0.68f, 0.045f);
		float c5 = Pulse(progress, 0.82f, 0.04f);
		driveA = 1.85f * tonic + 0.96f * c1 - 1.18f * c2 + 0.98f * c3 - 0.82f * c4 + 0.66f * c5;
		driveB = -0.22f * tonic + 0.42f * c1 + 0.76f * c2 - 0.58f * c3 + 0.46f * c4 - 0.34f * c5;
		driveA += sinf(progress * 96.0f + 1.2f) * 0.22f * envelope;
		driveB += cosf(progress * 58.0f + 0.5f) * 0.13f * envelope;
		break;
	}
	default: {
		float q1 = Pulse(progress, 0.12f + s.tremorPhase * 0.12f, 0.045f);
		float q2 = Pulse(progress, 0.21f - s.tremorPhase * 0.10f, 0.042f);
		float q3 = Pulse(progress, 0.31f + s.tremorPhase * 0.08f, 0.040f);
		float q4 = Pulse(progress, 0.42f - s.tremorPhase * 0.08f, 0.038f);
		float q5 = Pulse(progress, 0.54f + s.tremorPhase * 0.06f, 0.036f);
		float q6 = Pulse(progress, 0.66f - s.tremorPhase * 0.06f, 0.034f);
		float q7 = Pulse(progress, 0.79f + s.tremorPhase * 0.04f, 0.032f);
		float q8 = Pulse(progress, 0.91f, 0.030f);
		driveA = 1.18f * q1 - 0.86f * q2 + 1.02f * q3 - 0.78f * q4 + 0.90f * q5 - 0.66f * q6 + 0.56f * q7 - 0.38f * q8;
		driveB = 0.42f * q1 + 0.16f * q2 - 0.32f * q3 + 0.40f * q4 - 0.24f * q5 + 0.28f * q6 - 0.16f * q7 + 0.12f * q8;
		float buzz = (sinf(progress * 146.0f + 0.8f) + cosf(progress * 118.0f + 1.7f) * 0.60f) * 0.14f * envelope;
		driveA += buzz;
		driveB -= buzz * 0.48f;
		break;
	}
	}

	driveA *= envelope;
	driveB *= envelope;
	if (fabsf(driveA) + fabsf(driveB) <= 0.001f) {
		ClearDeathTremor(s, rootNode);
		return;
	}

	NiPoint3 forward = Normalize({s.deathWorldDir.x, s.deathWorldDir.y, 0.0f});
	NiPoint3 lateral = {-forward.y, forward.x, 0.0f};
	float base = Clampf(Config::fDeathTremorStrength * 2.5f * scale * s.tremorStrengthScale, 0.0f, 260.0f);
	float side = s.deathSide >= 0.0f ? 1.0f : -1.0f;
	NiPoint3 rootAng = {0.0f, 0.0f, 0.0f};
	NiPoint3 rootLin = {0.0f, 0.0f, 0.0f};
	NiPoint3 pelvisAng = {0.0f, 0.0f, 0.0f};
	NiPoint3 headAng = {0.0f, 0.0f, 0.0f};
	NiPoint3 lArmAng = {0.0f, 0.0f, 0.0f};
	NiPoint3 rArmAng = {0.0f, 0.0f, 0.0f};
	NiPoint3 lLegAng = {0.0f, 0.0f, 0.0f};
	NiPoint3 rLegAng = {0.0f, 0.0f, 0.0f};

	switch (s.tremorVariant % 3) {
	case 0: {
		rootAng = AddVec(
			ScaleVec(lateral, base * (0.35f * driveA + 0.10f * driveB)),
			ScaleVec(forward, base * (0.18f * driveB - 0.04f * driveA))
		);
		rootLin = AddVec(
			ScaleVec(lateral, base * (0.04f * driveA)),
			ScaleVec(forward, base * (0.02f * driveB))
		);
		pelvisAng = AddVec(
			ScaleVec(lateral, base * (0.55f * driveA + 0.18f * driveB)),
			ScaleVec(forward, base * (0.26f * driveB - 0.05f * driveA))
		);
		headAng = AddVec(
			ScaleVec(lateral, base * (-1.50f * driveB - 0.18f * driveA)),
			ScaleVec(forward, base * (-0.72f * driveA))
		);
		float armL = 0.62f * driveA + 0.34f * driveB;
		float armR = -0.58f * driveA + 0.30f * driveB;
		float legL = 0.24f * driveA + 0.12f * driveB;
		float legR = -0.22f * driveA + 0.10f * driveB;
		lArmAng = AddVec(ScaleVec(lateral, base * armL), ScaleVec(forward, base * (0.70f * armL)));
		rArmAng = AddVec(ScaleVec(lateral, base * armR), ScaleVec(forward, base * (0.70f * armR)));
		lLegAng = AddVec(ScaleVec(lateral, base * legL), ScaleVec(forward, base * (0.55f * legL)));
		rLegAng = AddVec(ScaleVec(lateral, base * legR), ScaleVec(forward, base * (0.55f * legR)));
		break;
	}
	case 1: {
		float kickMain = 1.30f * driveA + 0.35f * driveB;
		float kickCounter = -0.42f * driveA + 0.24f * driveB;
		float lateralBias = side * (0.20f * driveA + 0.05f * driveB);
		rootAng = {0.0f, 0.0f, 0.0f};
		rootLin = {0.0f, 0.0f, 0.0f};
		pelvisAng = AddVec(
			ScaleVec(lateral, base * (0.14f * lateralBias)),
			ScaleVec(forward, base * (0.06f * kickMain))
		);
		headAng = AddVec(
			ScaleVec(lateral, base * (-0.06f * lateralBias)),
			ScaleVec(forward, base * (-0.05f * driveA))
		);
		if (side > 0.0f) {
			lArmAng = AddVec(ScaleVec(lateral, base * (0.18f * kickCounter)), ScaleVec(forward, base * (0.30f * kickCounter)));
			rArmAng = AddVec(ScaleVec(lateral, base * (0.72f * kickMain)), ScaleVec(forward, base * (0.78f * kickMain)));
			lLegAng = AddVec(ScaleVec(lateral, base * (0.14f * kickCounter)), ScaleVec(forward, base * (0.26f * kickCounter)));
			rLegAng = AddVec(ScaleVec(lateral, base * (0.90f * kickMain)), ScaleVec(forward, base * (1.00f * kickMain)));
		} else {
			lArmAng = AddVec(ScaleVec(lateral, base * (0.72f * kickMain)), ScaleVec(forward, base * (0.78f * kickMain)));
			rArmAng = AddVec(ScaleVec(lateral, base * (0.18f * kickCounter)), ScaleVec(forward, base * (0.30f * kickCounter)));
			lLegAng = AddVec(ScaleVec(lateral, base * (0.90f * kickMain)), ScaleVec(forward, base * (1.00f * kickMain)));
			rLegAng = AddVec(ScaleVec(lateral, base * (0.14f * kickCounter)), ScaleVec(forward, base * (0.26f * kickCounter)));
		}
		break;
	}
	default: {
		rootAng = AddVec(
			ScaleVec(lateral, base * (1.45f * driveA + 0.24f * driveB)),
			ScaleVec(forward, base * (0.95f * driveB - 0.08f * driveA))
		);
		rootLin = AddVec(
			ScaleVec(lateral, base * (0.10f * driveA)),
			ScaleVec(forward, base * (0.12f * driveB + 0.03f * driveA))
		);
		pelvisAng = AddVec(
			ScaleVec(lateral, base * (2.85f * driveA + 0.55f * driveB)),
			ScaleVec(forward, base * (1.55f * driveB - 0.16f * driveA))
		);
		headAng = AddVec(
			ScaleVec(lateral, base * (-0.72f * driveB)),
			ScaleVec(forward, base * (-0.55f * driveA))
		);
		float armDrive = 0.52f * driveA + 0.18f * driveB;
		float legDrive = 0.90f * driveA + 0.38f * driveB;
		lArmAng = AddVec(ScaleVec(lateral, base * armDrive), ScaleVec(forward, base * (0.62f * armDrive)));
		rArmAng = AddVec(ScaleVec(lateral, base * (-0.85f * armDrive)), ScaleVec(forward, base * (-0.52f * armDrive)));
		lLegAng = AddVec(ScaleVec(lateral, base * legDrive), ScaleVec(forward, base * (1.18f * legDrive)));
		rLegAng = AddVec(ScaleVec(lateral, base * (-0.92f * legDrive)), ScaleVec(forward, base * (-1.08f * legDrive)));
		break;
	}
	}

	SetLinearVelocity(rootNode, rootLin);
	SetAngularVelocity(rootNode, rootAng);
	SetLinearVelocity(s.bones[kBone_Pelvis], ScaleVec(rootLin, 0.65f));
	SetAngularVelocity(s.bones[kBone_Pelvis], pelvisAng);
	SetAngularVelocity(s.bones[kBone_Head], headAng);
	SetAngularVelocity(s.bones[kBone_LUpperArm], lArmAng);
	SetAngularVelocity(s.bones[kBone_RUpperArm], rArmAng);
	SetAngularVelocity(s.bones[kBone_LThigh], lLegAng);
	SetAngularVelocity(s.bones[kBone_RThigh], rLegAng);
	s.lastRootLinMag = sqrtf(rootLin.x * rootLin.x + rootLin.y * rootLin.y + rootLin.z * rootLin.z);
}

} //namespace HitReaction
