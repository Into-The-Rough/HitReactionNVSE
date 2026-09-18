#include "hit_reaction.h"
#include "game.h"
#include "config.h"

namespace HitReaction {

StateEntry g_states[kMaxStates];
UInt32 g_stateCount = 0;

State* FindState(UInt32 refID) {
	for (UInt32 i = 0; i < g_stateCount; i++)
		if (g_states[i].refID == refID)
			return &g_states[i].state;
	return nullptr;
}

State* GetOrCreateState(UInt32 refID) {
	State* state = FindState(refID);
	if (state) return state;
	if (g_stateCount >= kMaxStates) return nullptr;
	StateEntry& entry = g_states[g_stateCount++];
	entry.refID = refID;
	memset(&entry.state, 0, sizeof(entry.state));
	return &entry.state;
}

static void EraseState(UInt32 index) {
	if (index >= g_stateCount) return;
	--g_stateCount;
	if (index != g_stateCount)
		g_states[index] = g_states[g_stateCount];
}

static const char* kBoneNames[kBone_Count] = {
	"Bip01 Spine", "Bip01 Spine1", "Bip01 Spine2",
	"Bip01 Neck", "Bip01 Head",
	"Bip01 Pelvis",
	"Bip01 L Thigh", "Bip01 L Calf",
	"Bip01 R Thigh", "Bip01 R Calf",
	"Bip01 L UpperArm", "Bip01 L Forearm",
	"Bip01 R UpperArm", "Bip01 R Forearm",
	"Bip01 L Clavicle", "Bip01 R Clavicle"
};

static void EulerToMatrix(NiMatrix33* out, float x, float y, float z) {
	float cx = cosf(x), sx = sinf(x);
	float cy = cosf(y), sy = sinf(y);
	float cz = cosf(z), sz = sinf(z);
	out->m[0][0] = cy * cz;
	out->m[0][1] = cy * sz;
	out->m[0][2] = -sy;
	out->m[1][0] = sx * sy * cz - cx * sz;
	out->m[1][1] = sx * sy * sz + cx * cz;
	out->m[1][2] = sx * cy;
	out->m[2][0] = cx * sy * cz + sx * sz;
	out->m[2][1] = cx * sy * sz - sx * cz;
	out->m[2][2] = cx * cy;
}

static void MatMul(NiMatrix33* out, const NiMatrix33* a, const NiMatrix33* b) {
	NiMatrix33 tmp;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			tmp.m[i][j] = a->m[i][0] * b->m[0][j]
			             + a->m[i][1] * b->m[1][j]
			             + a->m[i][2] * b->m[2][j];
	*out = tmp;
}

static NiPoint3 MatVecMul(const NiMatrix33* m, const NiPoint3* v) {
	return {
		m->m[0][0] * v->x + m->m[0][1] * v->y + m->m[0][2] * v->z,
		m->m[1][0] * v->x + m->m[1][1] * v->y + m->m[1][2] * v->z,
		m->m[2][0] * v->x + m->m[2][1] * v->y + m->m[2][2] * v->z,
	};
}

static NiPoint3 ComputeWorldDir(ActorHitData* hit, void* niNode) {
	NiPoint3 targetPos = GetWorldPosition(niNode);
	NiPoint3 worldDir;
	if (hit->source) {
		void* srcNode = GetNiNode((void*)hit->source);
		if (srcNode) {
			NiPoint3 srcPos = GetWorldPosition(srcNode);
			worldDir.x = targetPos.x - srcPos.x;
			worldDir.y = targetPos.y - srcPos.y;
			worldDir.z = 0;
		} else {
			worldDir = {0, 1.0f, 0};
		}
	} else {
		worldDir.x = targetPos.x - hit->impactPos.x;
		worldDir.y = targetPos.y - hit->impactPos.y;
		worldDir.z = 0;
	}
	return Normalize(worldDir);
}

static NiPoint3 WorldToLocal(const NiPoint3& worldDir, void* node) {
	NiMatrix33* worldRot = GetWorldRotation(node);
	NiMatrix33 invRot;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			invRot.m[i][j] = worldRot->m[j][i];
	return MatVecMul(&invRot, &worldDir);
}

static float RandRange(float min, float max) {
	return min + RandFloat() * (max - min);
}

static float Lerp(float a, float b, float t) {
	return a + (b - a) * Clampf(t, 0.0f, 1.0f);
}

static NiPoint3 SubVec(const NiPoint3& a, const NiPoint3& b) {
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static bool IsHeadLoc(SInt32 loc) {
	return loc == 1 || loc == 2 || loc == 13;
}

static bool GetImpactOffsetLocal(ActorHitData* hit, void* niNode, SInt32 loc, NiPoint3& out) {
	const char* boneName = IsHeadLoc(loc) ? "Bip01 Head" : "Bip01 Spine2";
	void* bone = FindBone(niNode, boneName);
	NiPoint3 refPos = GetWorldPosition(bone ? bone : niNode);
	NiPoint3 worldOffset = {
		hit->impactPos.x - refPos.x,
		hit->impactPos.y - refPos.y,
		hit->impactPos.z - refPos.z
	};
	float magSq = worldOffset.x * worldOffset.x + worldOffset.y * worldOffset.y + worldOffset.z * worldOffset.z;
	if (magSq < 0.25f || magSq > 65536.0f)
		return false;
	out = WorldToLocal(worldOffset, niNode);
	return true;
}

static void RotateLocalDir(NiPoint3& dir, float angle) {
	float lenSq = dir.x * dir.x + dir.y * dir.y;
	if (lenSq < 0.0001f) {
		dir = {0, 1.0f, 0};
		return;
	}
	float c = cosf(angle);
	float s = sinf(angle);
	float x = dir.x * c - dir.y * s;
	float y = dir.x * s + dir.y * c;
	float invLen = 1.0f / sqrtf(x * x + y * y);
	dir.x = x * invLen;
	dir.y = y * invLen;
}

static void AddUpperImpulseVariation(ActorHitData* hit, void* niNode, SInt32 loc, NiPoint3& localDir, NiPoint3& axisMul) {
	bool head = IsHeadLoc(loc);
	bool torso = loc == 0;
	if (!head && !torso)
		return;

	NiPoint3 impactOffset = {0, 0, 0};
	bool hasImpact = GetImpactOffsetLocal(hit, niNode, loc, impactOffset);
	float sideBias = hasImpact ? Clampf(impactOffset.x / (head ? 18.0f : 48.0f), -1.0f, 1.0f) : 0.0f;
	float frontBias = hasImpact ? Clampf(impactOffset.y / (head ? 18.0f : 42.0f), -1.0f, 1.0f) : 0.0f;
	float verticalBias = hasImpact ? Clampf(impactOffset.z / (head ? 18.0f : 70.0f), -1.0f, 1.0f) : 0.0f;

	float angleDeg = RandRange(head ? -5.0f : -8.0f, head ? 5.0f : 8.0f);
	angleDeg += sideBias * (head ? 4.0f : 6.0f);
	angleDeg += frontBias * (head ? 1.5f : 2.5f);
	RotateLocalDir(localDir, angleDeg * 0.01745329f);

	axisMul.x = RandRange(0.80f, 1.20f);
	axisMul.y = RandRange(0.80f, 1.20f);
	axisMul.z = RandRange(0.80f, 1.20f);
	if (hasImpact) {
		axisMul.x *= 1.0f + Clampf(frontBias * 0.05f - verticalBias * 0.04f, -0.08f, 0.08f);
		axisMul.y *= 1.0f + Clampf(-sideBias * (head ? 0.08f : 0.06f), -0.10f, 0.10f);
		axisMul.z *= 1.0f + Clampf(sideBias * (head ? 0.08f : 0.06f), -0.10f, 0.10f);
		axisMul.x = Clampf(axisMul.x, 0.80f, 1.20f);
		axisMul.y = Clampf(axisMul.y, 0.80f, 1.20f);
		axisMul.z = Clampf(axisMul.z, 0.80f, 1.20f);
	}
}

struct TorsoPoseProfile {
	float spineW, spine1W, spine2W;
	float pelvisYawW, pelvisRollW;
	float pitchMul, yawMul, rollMul;
	float headFollowMul;
};

static void BlendProfile(TorsoPoseProfile& p, const TorsoPoseProfile& target, float t) {
	p.spineW = Lerp(p.spineW, target.spineW, t);
	p.spine1W = Lerp(p.spine1W, target.spine1W, t);
	p.spine2W = Lerp(p.spine2W, target.spine2W, t);
	p.pelvisYawW = Lerp(p.pelvisYawW, target.pelvisYawW, t);
	p.pelvisRollW = Lerp(p.pelvisRollW, target.pelvisRollW, t);
	p.pitchMul = Lerp(p.pitchMul, target.pitchMul, t);
	p.yawMul = Lerp(p.yawMul, target.yawMul, t);
	p.rollMul = Lerp(p.rollMul, target.rollMul, t);
	p.headFollowMul = Lerp(p.headFollowMul, target.headFollowMul, t);
}

static void StoreTorsoProfile(State& s, const TorsoPoseProfile& p, float sideBias) {
	s.torsoProfileActive = true;
	s.torsoSpineW = p.spineW;
	s.torsoSpine1W = p.spine1W;
	s.torsoSpine2W = p.spine2W;
	s.torsoPelvisYawW = p.pelvisYawW;
	s.torsoPelvisRollW = p.pelvisRollW;
	s.torsoPitchMul = p.pitchMul;
	s.torsoYawMul = p.yawMul;
	s.torsoRollMul = p.rollMul;
	s.torsoHeadFollowMul = p.headFollowMul;
	s.torsoSideBias = sideBias;
}

static bool GetBoneWorldPos(void* niNode, const char* boneName, NiPoint3& out) {
	void* bone = FindBone(niNode, boneName);
	if (!bone)
		return false;
	out = GetWorldPosition(bone);
	return true;
}

static bool GetTorsoImpactInputs(ActorHitData* hit, void* niNode, float& sideBias, float& frontBias, float& height01) {
	NiPoint3 pelvisPos, spine1Pos, spine2Pos;
	if (!GetBoneWorldPos(niNode, "Bip01 Pelvis", pelvisPos)
		|| !GetBoneWorldPos(niNode, "Bip01 Spine1", spine1Pos)
		|| !GetBoneWorldPos(niNode, "Bip01 Spine2", spine2Pos))
		return false;

	NiPoint3 impact = hit->impactPos;
	NiPoint3 fromSpine1World = SubVec(impact, spine1Pos);
	float magSq = fromSpine1World.x * fromSpine1World.x + fromSpine1World.y * fromSpine1World.y + fromSpine1World.z * fromSpine1World.z;
	if (magSq < 0.25f || magSq > 65536.0f)
		return false;

	NiPoint3 fromSpine1 = WorldToLocal(fromSpine1World, niNode);
	NiPoint3 fromPelvis = WorldToLocal(SubVec(impact, pelvisPos), niNode);
	NiPoint3 torsoSpan = WorldToLocal(SubVec(spine2Pos, pelvisPos), niNode);
	float heightDen = fabsf(torsoSpan.z);
	if (heightDen < 20.0f || heightDen > 160.0f)
		heightDen = 80.0f;

	sideBias = Clampf(fromSpine1.x / 48.0f, -1.0f, 1.0f);
	frontBias = Clampf(fromSpine1.y / 42.0f, -1.0f, 1.0f);
	height01 = Clampf(fromPelvis.z / heightDen, 0.0f, 1.2f);
	return true;
}

static void BuildTorsoProfile(State& s, ActorHitData* hit, void* niNode) {
	TorsoPoseProfile p = {0.35f, 0.65f, 0.95f, 0.10f, 0.12f, 1.0f, 1.0f, 1.0f, 1.0f};
	float sideBias = 0.0f;
	float frontBias = 0.0f;
	float height01 = 0.55f;
	if (!GetTorsoImpactInputs(hit, niNode, sideBias, frontBias, height01)) {
		StoreTorsoProfile(s, p, 0.0f);
		return;
	}

	const TorsoPoseProfile low = {0.70f, 0.55f, 0.35f, 0.18f, 0.20f, 1.15f, 0.65f, 0.70f, 0.55f};
	const TorsoPoseProfile high = {0.20f, 0.55f, 1.10f, 0.04f, 0.06f, 0.70f, 1.25f, 1.30f, 1.25f};
	const TorsoPoseProfile side = {0.25f, 0.75f, 1.00f, 0.18f, 0.20f, 0.55f, 1.35f, 1.40f, 0.85f};

	float lowW = Clampf((0.45f - height01) / 0.35f, 0.0f, 1.0f);
	float highW = Clampf((height01 - 0.60f) / 0.35f, 0.0f, 1.0f);
	if (lowW > highW)
		BlendProfile(p, low, lowW * 0.85f);
	else
		BlendProfile(p, high, highW * 0.85f);

	float sideW = Clampf((fabsf(sideBias) - 0.20f) / 0.80f, 0.0f, 1.0f);
	BlendProfile(p, side, sideW * 0.70f);
	p.pitchMul *= 1.0f + Clampf(frontBias * 0.08f, -0.08f, 0.08f);

	StoreTorsoProfile(s, p, sideBias * sideW);
}

static void ApplyBoneRotation(void* bone, float pitch, float yaw, float roll) {
	if (!bone) return;
	NiMatrix33* localRot = GetLocalRotation(bone);
	NiMatrix33 additive;
	//3ds max biped axes: x=yaw, y=roll, z=pitch
	EulerToMatrix(&additive, yaw, roll, pitch);
	NiMatrix33 result;
	MatMul(&result, &additive, localRot);
	*localRot = result;
}

static void PopulateBoneCache(State& s, void* niNode) {
	if (s.boneRoot == niNode)
		return;
	s.boneRoot = niNode;
	for (int i = 0; i < kBone_Count; i++)
		s.bones[i] = FindBone(niNode, kBoneNames[i]);
}

static void InvalidateBoneCache(State& s) {
	s.boneRoot = nullptr;
	memset(s.bones, 0, sizeof(s.bones));
}

static void ApplyWeightedBone(State& s, int boneIdx, const NiPoint3& rot, float weight, float maxDeg) {
	void* bone = s.bones[boneIdx];
	if (!bone) return;
	NiPoint3 br = ClampVec({rot.x * weight, rot.y * weight, rot.z * weight}, maxDeg);
	ApplyBoneRotation(bone, br.x, br.y, br.z);
}

static void SimulateChannel(NiPoint3& rot, NiPoint3& vel, float& intensity, float dt, float dampMult = 1.0f) {
	intensity *= fmaxf(1.0f - Config::fDecay * dt, 0.0f);
	float damp = Config::fDamping * dampMult;
	NiPoint3 accel;
	accel.x = -rot.x * Config::fStiffness - vel.x * damp;
	accel.y = -rot.y * Config::fStiffness - vel.y * damp;
	accel.z = -rot.z * Config::fStiffness - vel.z * damp;
	vel.x = Clampf(vel.x + accel.x * dt, -50.0f, 50.0f);
	vel.y = Clampf(vel.y + accel.y * dt, -50.0f, 50.0f);
	vel.z = Clampf(vel.z + accel.z * dt, -50.0f, 50.0f);
	rot.x += vel.x * dt;
	rot.y += vel.y * dt;
	rot.z += vel.z * dt;
	rot = ClampVec(rot, Config::fMaxDeg);
}

void OnHit(ActorHitData* hit) {
	if (!hit || !hit->target) return;
	void* target = (void*)hit->target;
	UInt32 refID = GetRefID(target);

	if (!Config::bAffectPlayer && target == *g_thePlayer) return;
	if (!IsCharacter(target)) return;

	if (hit->flags & kHitFlag_Explosion) return;

	SInt32 loc = hit->hitLocation;
	if (loc == 14) return;

	void* niNode = GetNiNode(target);
	if (!niNode) return;

	float maxHP = GetMaxHealth(target);
	float effectiveHP = (Config::fHPCap > 0) ? fminf(maxHP, Config::fHPCap) : maxHP;
	effectiveHP = fmaxf(effectiveHP, 1.0f);
	float healthDmg = fmaxf(hit->healthDmg, 0.0f);
	float weaponDmg = fmaxf(hit->wpnBaseDmg, 0.0f);
	float relDmg = healthDmg / effectiveHP;

	NiPoint3 worldDir = ComputeWorldDir(hit, niNode);
	NiPoint3 localDir = WorldToLocal(worldDir, niNode);

	bool targetDead = IsDead(target);
	bool fatalHit = targetDead || (hit->flags & kHitFlag_IsFatal) != 0;
	State* state = FindState(refID);
	if (Config::bEnableDeathTremor && fatalHit) {
		if (!state)
			state = GetOrCreateState(refID);
		if (state)
			PrimeDeathTremor(*state, worldDir, localDir.x >= 0.0f ? 1.0f : -1.0f);
	}

	if (targetDead || (state && state->dead))
		return;
	if (Config::fReactionChance < 1.0f && RandFloat() > Config::fReactionChance)
		return;
	float skipChance = IsHeadLoc(loc) ? Config::fHeadSkipChance : Config::fSkipChance;
	if (relDmg < Config::fSkipThreshold && RandFloat() < skipChance)
		return;
	if (Config::fHitCooldown > 0.0f && state && state->hitCooldown > 0.0f)
		return;

	float armorFactor = 1.0f;
	if (Config::fArmorAbsorb > 0 && weaponDmg > 0.01f) {
		float absorbed = 1.0f - Clampf(healthDmg / weaponDmg, 0, 1.0f);
		armorFactor = 1.0f - absorbed * Config::fArmorAbsorb;
	}

	float wpnFactor = Clampf(sqrtf(weaponDmg / Config::fRefDmg), 0.05f, 2.0f);
	float hpFactor = Clampf(sqrtf(relDmg), Config::fHPFloor, 1.5f);
	float dmgFactor = wpnFactor * hpFactor;
	float randomMul = Config::fRandomMin + RandFloat() * (Config::fRandomMax - Config::fRandomMin);
	float strength = Config::fImpulse * dmgFactor * armorFactor * randomMul;
	if (!state)
		state = GetOrCreateState(refID);
	if (!state) return;
	State& s = *state;

	bool isArm = (loc >= 3 && loc <= 6);
	bool isLower = (loc >= 7 && loc <= 12);
	bool isTorso = loc == 0;
	if (isTorso)
		BuildTorsoProfile(s, hit, niNode);
	else if (!isLower)
		s.torsoProfileActive = false;

	NiPoint3 impulseMul = {1.0f, 1.0f, 1.0f};
	if (!isArm && !isLower)
		AddUpperImpulseVariation(hit, niNode, loc, localDir, impulseMul);

	float pitchImp = localDir.y * strength * Config::fPitchScale * impulseMul.x;
	float rollImp = localDir.x * strength * Config::fRollScale * impulseMul.z;
	float yawImp = -localDir.x * strength * Config::fYawScale * impulseMul.y;
	if (isTorso && s.torsoProfileActive) {
		yawImp += -s.torsoSideBias * strength * Config::fYawScale * 0.45f;
		rollImp += s.torsoSideBias * strength * Config::fRollScale * 0.55f;
	}

	if (isLower) {
		s.lowerVel.x += pitchImp * Config::fLegMult;
		s.lowerVel.y += yawImp * Config::fLegMult;
		s.lowerVel.z += rollImp * Config::fLegMult;
		s.lowerIntensity = 1.0f;
		if (loc <= 9) s.lastLowerLoc = 1;
		else s.lastLowerLoc = 2;
		if (healthDmg >= 15.0f) {
			float buckleMag = Clampf(healthDmg * 0.1f, 0.5f, 1.0f) * 200.0f * Config::fKneeBuckleMult;
			s.buckleVel += buckleMag;
			s.buckleSide = s.lastLowerLoc;
		}
	} else {
		if (IsHeadLoc(loc)) {
			s.headHit = true;
			s.lastUpperLoc = 0;
		} else {
			s.headHit = false;
			if (loc == 0) s.lastUpperLoc = 0;
			else if (loc == 3 || loc == 4) s.lastUpperLoc = 1;
			else if (loc == 5 || loc == 6) s.lastUpperLoc = 2;
		}

		if (isArm) {
			float armStrength = strength * Config::fArmMult;
			s.armVel.x += localDir.y * armStrength * Config::fPitchScale * 0.45f;
			s.armVel.y += -localDir.x * armStrength * Config::fYawScale * 0.85f;
			s.armVel.z += localDir.x * armStrength * Config::fRollScale * 0.95f;
			s.armIntensity = 1.0f;

			s.upperVel.x += pitchImp * 0.06f;
			s.upperVel.y += yawImp * 0.45f;
			s.upperVel.z += rollImp * 0.55f;
			s.upperIntensity = 0.85f;
		} else {
			s.upperVel.x += pitchImp;
			s.upperVel.y += yawImp;
			s.upperVel.z += rollImp;
			s.upperIntensity = 1.0f;
		}
	}

	bool isCritical = (hit->flags & kHitFlag_IsCritical) != 0;
	if (isCritical && loc == 0 && !s.spinning) {
		float spinDir = (localDir.x >= 0) ? 1.0f : -1.0f;
		s.spinVel = spinDir * 4.0f;
		s.spinAngle = 0;
		s.spinning = true;
	}
	s.hitCooldown = Config::fHitCooldown;
}

void Simulate() {
	if (CdeclCall<bool>(0x702360)) return;
	float dt = g_timeGlobal->secondsPassed;
	if (dt <= 0.0f || dt > 0.5f) dt = 0.016f;

	UInt32 i = 0;
	while (i < g_stateCount) {
		State& s = g_states[i].state;

		if (s.dead) {
			++i;
			continue;
		}

		if (s.dying) {
			s.deathPrimeTimer -= dt;
			if (s.deathPrimeTimer <= 0.0f)
				s.dying = false;
		}
		if (s.awaitingFinish) {
			s.awaitTimer -= dt;
			if (s.awaitTimer <= 0.0f)
				s.awaitingFinish = false;
		}
		if (s.hitCooldown > 0.0f) {
			s.hitCooldown -= dt;
			if (s.hitCooldown < 0.0f)
				s.hitCooldown = 0.0f;
		}

		//substep explicit euler for stability
		for (float remaining = dt; remaining > 0.0f; remaining -= 0.0166667f) {
			float step = remaining > 0.0166667f ? 0.0166667f : remaining;

			SimulateChannel(s.upperRot, s.upperVel, s.upperIntensity, step);
			SimulateChannel(s.armRot, s.armVel, s.armIntensity, step, 1.2f);
			SimulateChannel(s.lowerRot, s.lowerVel, s.lowerIntensity, step, 1.5f);

			float bAccel = -s.buckleRot * 160.0f - s.buckleVel * 12.0f;
			s.buckleVel = Clampf(s.buckleVel + bAccel * step, -50.0f, 50.0f);
			s.buckleRot += s.buckleVel * step;
			float maxBuckle = 45.0f * 0.01745329f;
			s.buckleRot = Clampf(s.buckleRot, -maxBuckle, maxBuckle);

			if (s.spinning) {
				float spinStiffness = 30.0f;
				float spinDamping = 6.0f;
				float maxSpin = 1.0472f; //60 deg
				float sAccel = -s.spinAngle * spinStiffness - s.spinVel * spinDamping;
				s.spinVel += sAccel * step;
				s.spinAngle += s.spinVel * step;
				s.spinAngle = Clampf(s.spinAngle, -maxSpin, maxSpin);
				if (fabsf(s.spinAngle) < 0.01f && fabsf(s.spinVel) < 0.01f) {
					s.spinAngle = 0;
					s.spinVel = 0;
					s.spinning = false;
				}
			}
		}

		if (!s.dead && !s.dying && !s.awaitingFinish && s.currentHunch < 0.001f
				&& fabsf(s.buckleRot) < 0.001f && !s.spinning) {
			float mag = fabsf(s.upperRot.x) + fabsf(s.upperRot.y) + fabsf(s.upperRot.z)
			          + fabsf(s.upperVel.x) + fabsf(s.upperVel.y) + fabsf(s.upperVel.z)
			          + fabsf(s.armRot.x) + fabsf(s.armRot.y) + fabsf(s.armRot.z)
			          + fabsf(s.armVel.x) + fabsf(s.armVel.y) + fabsf(s.armVel.z)
			          + fabsf(s.lowerRot.x) + fabsf(s.lowerRot.y) + fabsf(s.lowerRot.z)
			          + fabsf(s.lowerVel.x) + fabsf(s.lowerVel.y) + fabsf(s.lowerVel.z)
			          + fabsf(s.buckleVel);
			if (mag < 0.01f) {
				EraseState(i);
				continue;
			}
		}

		++i;
	}
}

void ApplyBones() {
	if (!g_stateCount) return;
	if (CdeclCall<bool>(0x702360)) return;

	void* player = *g_thePlayer;
	if (!player) return;

	void* playerNode = GetNiNode(player);
	NiPoint3 playerPos = {0, 0, 0};
	if (playerNode)
		playerPos = GetWorldPosition(playerNode);

	float cullDistSq = Config::fCullDist * Config::fCullDist;
	float dt = g_timeGlobal->secondsPassed;
	if (dt <= 0.0f || dt > 0.5f) dt = 0.016f;

	UInt32 i = 0;
	while (i < g_stateCount) {
		State& s = g_states[i].state;

		void* form = LookupForm(g_states[i].refID);
		if (!form) { EraseState(i); continue; }
		bool isDying = s.dying && IsDying(form);

		void* niNode = GetNiNode(form);
		if (!niNode) {
			InvalidateBoneCache(s);
			//preserve pending death state across 3d loss
			if (s.dying || s.awaitingFinish) {
				++i;
				continue;
			}
			EraseState(i);
			continue;
		}

		NiPoint3 pos = GetWorldPosition(niNode);
		float dx = pos.x - playerPos.x;
		float dy = pos.y - playerPos.y;
		float dz = pos.z - playerPos.z;
		bool inRange = dx * dx + dy * dy + dz * dz <= cullDistSq;

		if (isDying && !s.dead && Config::bEnableDeathTremor && Config::fDeathTremorDuration > 0.0f) {
			if (RollTremorChance(s)) {
				StartCorpseTremor(s, true);
			} else {
				//keep the decision until the death hook
				s.dying = false;
				s.awaitingFinish = true;
				s.awaitTimer = 10.0f;
			}
		}

		if (s.dead) {
			//don't strand a paused tremor after reload
			bool modeEnabled = s.primedTremor ? Config::bEnableDeathTremor : Config::bEnablePostDeathTremor;
			if (!modeEnabled) {
				PopulateBoneCache(s, niNode);
				ClearDeathTremor(s, niNode);
				EraseState(i);
				continue;
			}
			if (s.deathDelay > 0.0f) {
				s.deathDelay -= dt;
				if (s.deathDelay < 0.0f)
					s.deathDelay = 0.0f;
				++i;
				continue;
			}

			//pause while external physics owns the body
			bool gated = false;
			if (inRange) {
				PopulateBoneCache(s, niNode);
				gated = !CorpseSettled(s, niNode);
				if (gated)
					s.lastRootLinMag = 0.0f;
			}

			if (!gated) {
				s.deathTimer -= dt;
				if (s.deathTimer <= 0.0f) {
					PopulateBoneCache(s, niNode);
					ClearDeathTremor(s, niNode);
					EraseState(i);
					continue;
				}
				if (inRange)
					ApplyDeathTremor(s, niNode, s.deathTimer, s.tremorDuration, s.primedTremor ? 1.0f : Config::fPostDeathTremorMult);
			}

			++i;
			continue;
		}

		if (!inRange) {
			if (s.currentHunch > 0.0f)
				s.currentHunch *= expf(-4.0f * dt);
			++i;
			continue;
		}

		PopulateBoneCache(s, niNode);

		NiPoint3 upperRot = ScaleVec(s.upperRot, s.upperIntensity);
		NiPoint3 armRot = ScaleVec(s.armRot, s.armIntensity);
		NiPoint3 lowerRot = ScaleVec(s.lowerRot, s.lowerIntensity);
		float upperMag = fabsf(upperRot.x) + fabsf(upperRot.y) + fabsf(upperRot.z);
		float armMag = fabsf(armRot.x) + fabsf(armRot.y) + fabsf(armRot.z);
		float lowerMag = fabsf(lowerRot.x) + fabsf(lowerRot.y) + fabsf(lowerRot.z);

		float hunchAngle = 0;
		bool hasHunch = false;
		if (Config::bEnablePosture && !IsInCombat(form)) {
			float healthRatio = GetHealthRatio(form);
			float hunchRatio = Clampf(1.0f - healthRatio, 0, 1.0f);
			float targetHunch = hunchRatio * hunchRatio * Config::fPostureMaxDeg * 0.01745329f;
			s.currentHunch += (targetHunch - s.currentHunch) * (1.0f - expf(-4.0f * dt));
			hunchAngle = s.currentHunch;
			hasHunch = hunchAngle > 0.001f;
		} else if (s.currentHunch > 0.001f) {
			s.currentHunch *= expf(-4.0f * dt);
			hunchAngle = s.currentHunch;
			hasHunch = hunchAngle > 0.001f;
		}

		bool hasSpin = s.spinning && fabsf(s.spinAngle) > 0.001f;
		if (hasSpin) {
			NiPoint3 spinRot = {0, s.spinAngle, 0};
			ApplyWeightedBone(s, kBone_Spine, spinRot, 0.3f, 60.0f);
			ApplyWeightedBone(s, kBone_Spine1, spinRot, 0.5f, 60.0f);
			ApplyWeightedBone(s, kBone_Spine2, spinRot, 0.7f, 60.0f);
		}

		if (upperMag > 0.001f || hasHunch) {
			bool useTorsoProfile = upperMag > 0.001f && !s.headHit && s.lastUpperLoc == 0 && s.torsoProfileActive;
			float spineScale = s.headHit ? 0.08f : (s.lastUpperLoc ? 0.70f : 1.0f);
			float pitchMul = useTorsoProfile ? s.torsoPitchMul : 1.0f;
			float yawMul = useTorsoProfile ? s.torsoYawMul : 1.0f;
			float rollMul = useTorsoProfile ? s.torsoRollMul : 1.0f;
			NiPoint3 spineRot = {
				-upperRot.x * spineScale * pitchMul - hunchAngle,
				upperRot.y * spineScale * 0.45f * yawMul,
				upperRot.z * spineScale * 0.65f * rollMul
			};
			float pelvisYawW = useTorsoProfile ? s.torsoPelvisYawW : 0.10f;
			float pelvisRollW = useTorsoProfile ? s.torsoPelvisRollW : 0.12f;
			float spineW = useTorsoProfile ? s.torsoSpineW : 0.35f;
			float spine1W = useTorsoProfile ? s.torsoSpine1W : 0.65f;
			float spine2W = useTorsoProfile ? s.torsoSpine2W : 0.95f;
			ApplyWeightedBone(s, kBone_Pelvis, {0, -spineRot.y * pelvisYawW, -spineRot.z * pelvisRollW}, 1.0f, Config::fMaxDeg);
			ApplyWeightedBone(s, kBone_Spine, spineRot, spineW, Config::fMaxDeg + Config::fPostureMaxDeg);
			ApplyWeightedBone(s, kBone_Spine1, spineRot, spine1W, Config::fMaxDeg + Config::fPostureMaxDeg);
			ApplyWeightedBone(s, kBone_Spine2, spineRot, spine2W, Config::fMaxDeg + Config::fPostureMaxDeg);

			if (s.headHit) {
				NiPoint3 headRot = {-upperRot.x * 1.45f - hunchAngle * 1.2f, -upperRot.y * 1.35f, -upperRot.z * 1.35f};
				ApplyWeightedBone(s, kBone_Neck, headRot, 0.45f, Config::fMaxHeadDeg + Config::fPostureMaxDeg);
				ApplyWeightedBone(s, kBone_Head, headRot, 0.85f, Config::fMaxHeadDeg + Config::fPostureMaxDeg);
			} else {
				float hf = s.lastUpperLoc ? Config::fHeadFollow * 0.25f : Config::fHeadFollow;
				if (useTorsoProfile)
					hf *= s.torsoHeadFollowMul;
				NiPoint3 headFollow = {-upperRot.x * hf - hunchAngle * 1.5f, -upperRot.y * hf, -upperRot.z * hf};
				ApplyWeightedBone(s, kBone_Neck, headFollow, 0.4f, Config::fMaxHeadDeg + Config::fPostureMaxDeg);
				ApplyWeightedBone(s, kBone_Head, headFollow, 1.0f, Config::fMaxHeadDeg + Config::fPostureMaxDeg);
			}

			if (upperMag > 0.001f && !s.headHit && s.lastUpperLoc == 0) {
				ApplyWeightedBone(s, kBone_LUpperArm, upperRot, Config::fArmPassive, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_LForearm, upperRot, Config::fForearmPassive, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_RUpperArm, upperRot, Config::fArmPassive, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_RForearm, upperRot, Config::fForearmPassive, Config::fMaxDeg);
			}
		}

		if (armMag > 0.001f) {
			NiPoint3 shoulderRot = {
				armRot.x * 0.35f + upperRot.x * 0.65f,
				armRot.y * 0.40f + upperRot.y * 0.75f,
				armRot.z * 0.40f + upperRot.z * 0.75f
			};
			if (s.lastUpperLoc == 1) {
				ApplyWeightedBone(s, kBone_LClavicle, shoulderRot, Config::fClavicleHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_LUpperArm, armRot, Config::fArmHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_LForearm, armRot, Config::fForearmHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_RUpperArm, upperRot, Config::fArmPassive * 0.15f, Config::fMaxDeg);
			} else if (s.lastUpperLoc == 2) {
				ApplyWeightedBone(s, kBone_RClavicle, shoulderRot, Config::fClavicleHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_RUpperArm, armRot, Config::fArmHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_RForearm, armRot, Config::fForearmHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_LUpperArm, upperRot, Config::fArmPassive * 0.15f, Config::fMaxDeg);
			}
		}

		if (lowerMag > 0.001f) {
			NiPoint3 legRot = {lowerRot.x, 0, 0};
			ApplyWeightedBone(s, kBone_Pelvis, legRot, Config::fPelvisHit, Config::fMaxDeg);
			if (s.lastLowerLoc == 1) {
				ApplyWeightedBone(s, kBone_LThigh, legRot, Config::fThighHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_LCalf, legRot, Config::fCalfHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_RThigh, legRot, Config::fThighPassive, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_RCalf, legRot, Config::fCalfPassive, Config::fMaxDeg);
			} else if (s.lastLowerLoc == 2) {
				ApplyWeightedBone(s, kBone_RThigh, legRot, Config::fThighHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_RCalf, legRot, Config::fCalfHit, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_LThigh, legRot, Config::fThighPassive, Config::fMaxDeg);
				ApplyWeightedBone(s, kBone_LCalf, legRot, Config::fCalfPassive, Config::fMaxDeg);
			}
		}

		bool hasBuckle = fabsf(s.buckleRot) > 0.001f;
		if (hasBuckle) {
			NiPoint3 buckle = {s.buckleRot, 0, 0};
			if (s.buckleSide == 1) {
				ApplyWeightedBone(s, kBone_LCalf, buckle, 1.0f, 45.0f);
				ApplyWeightedBone(s, kBone_LThigh, buckle, 0.3f, 45.0f);
			} else if (s.buckleSide == 2) {
				ApplyWeightedBone(s, kBone_RCalf, buckle, 1.0f, 45.0f);
				ApplyWeightedBone(s, kBone_RThigh, buckle, 0.3f, 45.0f);
			}
		}

		bool needsUpdate = lowerMag > 0.001f || upperMag > 0.001f || armMag > 0.001f || hasHunch || hasBuckle || hasSpin;
		if (needsUpdate) {
			void* updateFrom = s.bones[lowerMag > 0.001f || hasBuckle ? kBone_Pelvis : (armMag > 0.001f ? kBone_Spine2 : kBone_Spine)];
			if (!updateFrom) updateFrom = FindBone(niNode, "Bip01");
			if (updateFrom) ThisCall(0xA59C60, updateFrom, &g_defaultUpdateData);
		}

		++i;
	}
}

void Clear() {
	g_stateCount = 0;
}

} //namespace HitReaction
