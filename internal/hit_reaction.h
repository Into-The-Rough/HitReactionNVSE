#pragma once
#include "common.h"

struct ActorHitData;

namespace HitReaction {

enum BoneIdx {
	kBone_Spine, kBone_Spine1, kBone_Spine2,
	kBone_Neck, kBone_Head,
	kBone_Pelvis,
	kBone_LThigh, kBone_LCalf,
	kBone_RThigh, kBone_RCalf,
	kBone_LUpperArm, kBone_LForearm,
	kBone_RUpperArm, kBone_RForearm,
	kBone_LClavicle, kBone_RClavicle,
	kBone_Count
};

struct State {
	NiPoint3 upperRot, upperVel;
	NiPoint3 armRot, armVel;
	NiPoint3 lowerRot, lowerVel;
	float upperIntensity;
	float armIntensity;
	float lowerIntensity;
	float hitCooldown;
	SInt32 lastUpperLoc;
	SInt32 lastLowerLoc;
	bool headHit;
	bool torsoProfileActive;
	float torsoSpineW, torsoSpine1W, torsoSpine2W;
	float torsoPelvisYawW, torsoPelvisRollW;
	float torsoPitchMul, torsoYawMul, torsoRollMul;
	float torsoHeadFollowMul;
	float torsoSideBias;
	float currentHunch;
	float buckleRot, buckleVel;
	SInt32 buckleSide;
	float spinAngle, spinVel;
	bool spinning;
	void* boneRoot;
	void* bones[kBone_Count];
	bool dying;
	bool dead;
	bool primedTremor;
	bool tremorRolled;
	bool tremorPassed;
	bool awaitingFinish;
	float awaitTimer;
	float lastRootLinMag;
	float deathPrimeTimer;
	float deathDelay;
	float deathTimer;
	float tremorDuration;
	float tremorStrengthScale;
	float tremorPhase;
	UInt32 tremorVariant;
	NiPoint3 deathWorldDir;
	float deathSide;
};

struct StateEntry {
	UInt32 refID;
	State state;
};

enum { kMaxStates = 256 };
extern StateEntry g_states[kMaxStates];
extern UInt32 g_stateCount;

State* FindState(UInt32 refID);
State* GetOrCreateState(UInt32 refID);

void OnHit(ActorHitData* hit);
void Simulate();
void ApplyBones();
void Clear();
bool InitHooks();

void PrimeDeathTremor(State& s, const NiPoint3& worldDir, float side);
void StartCorpseTremor(State& s, bool primed);
void ClearDeathTremor(State& s, void* rootNode = nullptr);
void ApplyDeathTremor(State& s, void* rootNode, float remaining, float duration, float scale);
void OnFinishDying(void* actor);
bool CorpseSettled(State& s, void* rootNode);
bool RollTremorChance(State& s);

} //namespace HitReaction
