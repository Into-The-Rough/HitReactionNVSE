#pragma once
#include "common.h"

struct Actor;
struct TESObjectWEAP;
struct Explosion;
struct Projectile;
struct SpellItem;

struct NiTransform {
	NiMatrix33 rotate;
	NiPoint3 translate;
	float scale;
};
static_assert(sizeof(NiTransform) == 0x34, "NiTransform");
static_assert(offsetof(NiTransform, translate) == 0x24, "NiTransform::translate");

struct NiAVObjectView {
	UInt8 pad00[0x18];
	void* parent;
	void* collisionObject;
	UInt8 pad20[0x14];
	NiTransform localTransform;
	NiTransform worldTransform;
};
static_assert(sizeof(NiAVObjectView) == 0x9C, "NiAVObject");
static_assert(offsetof(NiAVObjectView, collisionObject) == 0x1C, "NiAVObject::collisionObject");
static_assert(offsetof(NiAVObjectView, localTransform) == 0x34, "NiAVObject::localTransform");
static_assert(offsetof(NiAVObjectView, worldTransform) == 0x68, "NiAVObject::worldTransform");

struct TESFormView {
	void* vtable;
	UInt8 typeID;
	UInt8 pad05[7];
	UInt32 refID;
};
static_assert(offsetof(TESFormView, typeID) == 0x04, "TESForm::typeID");
static_assert(offsetof(TESFormView, refID) == 0x0C, "TESForm::refID");

struct ActorView {
	UInt8 pad00[0xA4];
	void* actorValueOwnerVtable;
};
static_assert(offsetof(ActorView, actorValueOwnerVtable) == 0xA4, "Actor::actorValueOwner");

struct TimeGlobalView {
	UInt8 pad00[0x0C];
	float secondsPassed;
};
static_assert(offsetof(TimeGlobalView, secondsPassed) == 0x0C, "TimeGlobal::secondsPassed");

struct bhkNiCollisionObjectView {
	UInt8 pad00[0x10];
	void* worldObject;
};
static_assert(offsetof(bhkNiCollisionObjectView, worldObject) == 0x10, "bhkNiCollisionObject::worldObject");

struct bhkWorldObjectView {
	UInt8 pad00[0x08];
	void* refObject;
};
static_assert(offsetof(bhkWorldObjectView, refObject) == 0x08, "bhkRefObject::refObject");

struct hkpRigidBodyView {
	UInt8 pad00[0x28];
	UInt8 collisionType;
	UInt8 pad29[0xBF];
	UInt8 motionType;
};
static_assert(offsetof(hkpRigidBodyView, collisionType) == 0x28, "hkpWorldObject::collisionType");
static_assert(offsetof(hkpRigidBodyView, motionType) == 0xE8, "hkpRigidBody::motionType");

struct ActorHitData {
	Actor* source;          //0x00
	Actor* target;          //0x04
	union {                 //0x08
		Projectile* projectile;
		Explosion* explosion;
	};
	UInt32 weaponAV;        //0x0C
	SInt32 hitLocation;     //0x10
	float healthDmg;        //0x14
	float wpnBaseDmg;      //0x18
	float fatigueDmg;       //0x1C
	float limbDmg;          //0x20
	float blockDTMod;       //0x24
	float armorDmg;         //0x28
	float weaponDmg;        //0x2C
	TESObjectWEAP* weapon;  //0x30
	float weapHealthPerc;   //0x34
	NiPoint3 impactPos;     //0x38
	NiPoint3 impactAngle;   //0x44
	SpellItem* critHitEffect; //0x50
	void* ptr54;            //0x54
	UInt32 flags;           //0x58
	float dmgMult;          //0x5C
	SInt32 unk60;           //0x60
};
static_assert(sizeof(ActorHitData) == 0x64, "ActorHitData");
static_assert(offsetof(ActorHitData, flags) == 0x58, "ActorHitData::flags");

enum ActorHitFlags {
	kHitFlag_IsCritical = 0x04,
	kHitFlag_IsFatal = 0x10,
	kHitFlag_Explosion = 0x2000
};

inline void** g_thePlayer = (void**)0x11DEA3C;
inline TimeGlobalView* g_timeGlobal = (TimeGlobalView*)0x11F6394;
inline NiUpdateData g_defaultUpdateData = {0};

inline void Console_Print(const char* fmt, ...) {
	void* mgr = CdeclCall<void*>(0x71B160, true);
	if (!mgr) return;
	va_list args;
	va_start(args, fmt);
	ThisCall(0x71D0A0, mgr, fmt, args);
	va_end(args);
}

//vtable offset 0x1D0 = TESObjectREFR::Get3D
inline void* GetNiNode(void* refr) {
	if (!refr) return nullptr;
	void** vtbl = *(void***)refr;
	typedef void* (__thiscall* Fn)(void*);
	return ((Fn)vtbl[0x1D0 / 4])(refr);
}

//0x4AAE30 = BSUtilities::GetObjectByName
inline void* FindBone(void* rootNode, const char* name) {
	if (!rootNode || !name) return nullptr;
	return CdeclCall<void*>(0x4AAE30, rootNode, name);
}

inline NiPoint3 GetWorldPosition(void* node) {
	return ((NiAVObjectView*)node)->worldTransform.translate;
}

inline NiMatrix33* GetWorldRotation(void* node) {
	return &((NiAVObjectView*)node)->worldTransform.rotate;
}

inline NiMatrix33* GetLocalRotation(void* node) {
	return &((NiAVObjectView*)node)->localTransform.rotate;
}

inline UInt32 GetRefID(void* form) {
	return ((TESFormView*)form)->refID;
}

inline void* LookupForm(UInt32 refID) {
	return CdeclCall<void*>(0x4839C0, refID);
}

inline bool IsDead(void* actor) {
	return ThisCall<bool>(0x8844F0, actor, true);
}

inline bool IsDying(void* actor) {
	return ThisCall<bool>(0x87D6A0, actor);
}

//vtable offset 0x274
inline bool IsInCombat(void* actor) {
	void** vtbl = *(void***)actor;
	typedef bool (__thiscall* Fn)(void*, bool);
	return ((Fn)vtbl[0x274 / 4])(actor, false);
}

inline bool IsCharacter(void* form) {
	return ((TESFormView*)form)->typeID == 0x3B; //kFormType_Character
}

//0x893590 = Actor::GetHealthPercentage
inline float GetHealthRatio(void* actor) {
	return (float)ThisCall<double>(0x893590, actor);
}

//actor value owner at +0xa4
inline float GetMaxHealth(void* actor) {
	void* avOwner = &((ActorView*)actor)->actorValueOwnerVtable;
	void** vtbl = *(void***)avOwner;
	typedef int (__thiscall* GetBaseAVI_t)(void*, UInt32);
	int val = ((GetBaseAVI_t)vtbl[0])(avOwner, 16);
	return (float)(val > 0 ? val : 1);
}

inline void* GetWorldObject(void* node) {
	if (!node) return nullptr;
	void* collisionObject = ((NiAVObjectView*)node)->collisionObject;
	if (!collisionObject) return nullptr;
	return ((bhkNiCollisionObjectView*)collisionObject)->worldObject;
}

inline void* GetRigidBodyRefObject(void* node) {
	//fixed bodies also accept velocity writes
	void* worldObj = GetWorldObject(node);
	if (!worldObj) return nullptr;
	void* hkObj = ((bhkWorldObjectView*)worldObj)->refObject;
	if (!hkObj) return nullptr;
	hkpRigidBodyView* rigidBody = (hkpRigidBodyView*)hkObj;
	if (rigidBody->collisionType != 1) return nullptr;
	UInt8 motionType = rigidBody->motionType;
	if (motionType != 2 && motionType != 3 && motionType != 6) return nullptr;
	return hkObj;
}

//0x560DC0 = hkpRigidBody::getLinearVelocity, returns &motion.linVelocity
inline bool GetLinearSpeedSq(void* node, float& out) {
	void* hkObj = GetRigidBodyRefObject(node);
	if (!hkObj) return false;
	float* v = ThisCall<float*>(0x560DC0, hkObj);
	out = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	return true;
}

inline bool SetLinearVelocity(void* node, const NiPoint3& velocity) {
	void* hkObj = GetRigidBodyRefObject(node);
	if (!hkObj) return false;
	alignas(16) hkVector4 hkVelocity = {velocity.x, velocity.y, velocity.z, 0.0f};
	ThisCall(0x5616D0, hkObj, &hkVelocity); //hkpRigidBody::setLinearVelocity
	return true;
}

inline bool SetAngularVelocity(void* node, const NiPoint3& velocity) {
	void* hkObj = GetRigidBodyRefObject(node);
	if (!hkObj) return false;
	alignas(16) hkVector4 hkVelocity = {velocity.x, velocity.y, velocity.z, 0.0f};
	ThisCall(0x561800, hkObj, &hkVelocity); //hkpRigidBody::setAngularVelocity
	return true;
}
