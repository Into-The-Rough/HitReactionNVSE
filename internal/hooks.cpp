#include "hit_reaction.h"
#include "game.h"
#include "config.h"

namespace HitReaction {

typedef void* (__thiscall* CopyHitData_t)(void* data, ActorHitData* hit);
static CopyHitData_t g_origCopyHitData = nullptr;
static CopyHitData_t g_origCopyMHP = nullptr;
static CopyHitData_t g_origCopyHP = nullptr;
static bool g_suppressCopyHitDataHook = false;
static CallDetour g_copyHitDataDetour;

static void TryHitReaction(ActorHitData* hit) {
	if (hit && hit->target)
		OnHit(hit);
}

static void* __fastcall Hook_CopyHitData(void* data, void* edx, ActorHitData* hit) {
	if (!g_suppressCopyHitDataHook)
		TryHitReaction(hit);
	return g_origCopyHitData ? g_origCopyHitData(data, hit) : data;
}

static void* __fastcall CopyHitHookMHP(void* proc, void* edx, ActorHitData* hit) {
	TryHitReaction(hit);
	bool oldSuppress = g_suppressCopyHitDataHook;
	g_suppressCopyHitDataHook = true;
	void* result = g_origCopyMHP ? g_origCopyMHP(proc, hit) : nullptr;
	g_suppressCopyHitDataHook = oldSuppress;
	return result;
}

static void* __fastcall CopyHitHookHP(void* proc, void* edx, ActorHitData* hit) {
	TryHitReaction(hit);
	bool oldSuppress = g_suppressCopyHitDataHook;
	g_suppressCopyHitDataHook = true;
	void* result = g_origCopyHP ? g_origCopyHP(proc, hit) : nullptr;
	g_suppressCopyHitDataHook = oldSuppress;
	return result;
}

typedef void (__thiscall* UpdateAnimations_t)(void*);
static UpdateAnimations_t g_origUpdateAnimations = nullptr;
static CallDetour g_updateAnimationsDetour;

static void __fastcall Hook_UpdateAnimations(void* tesMain, void* edx) {
	g_origUpdateAnimations(tesMain);
	ApplyBones();
}

static UInt32 g_finishDyingReturn = 0x8F74A2;

static void __fastcall Hook_FinishDyingStartTremor(void* actor) {
	OnFinishDying(actor);
}

static __declspec(naked) void Hook_FinishDying() {
	__asm {
		pushfd
		pushad
		mov ecx, [ebp+8]
		call Hook_FinishDyingStartTremor
		popad
		popfd
		mov ecx, [ebp+8]
		push ecx
		mov ecx, [ebp+8]
		jmp dword ptr [g_finishDyingReturn]
	}
}

bool InitHooks() {
	constexpr UInt32 kCall_CopyHitData = 0x92C488;
	constexpr UInt32 kCall_UpdateAnimations = 0x86EC9B;
	constexpr UInt32 kHook_FinishDying = 0x8F749B;
	//mov ecx,[ebp+8] / push ecx / mov ecx,[ebp+8], re-executed by the hook before resuming
	static const UInt8 kFinishDyingBytes[7] = {0x8B, 0x4D, 0x08, 0x51, 0x8B, 0x4D, 0x08};

	//install atomically
	if (*(UInt8*)kCall_CopyHitData != 0xE8) return false;
	if (*(UInt8*)kCall_UpdateAnimations != 0xE8) return false;
	if (memcmp((void*)kHook_FinishDying, kFinishDyingBytes, 7) != 0) return false;

	if (!g_updateAnimationsDetour.Install(kCall_UpdateAnimations, (UInt32)Hook_UpdateAnimations))
		return false;
	g_origUpdateAnimations = (UpdateAnimations_t)g_updateAnimationsDetour.GetOriginalTarget();

	if (!g_copyHitDataDetour.Install(kCall_CopyHitData, (UInt32)Hook_CopyHitData)) {
		g_updateAnimationsDetour.Restore();
		g_origUpdateAnimations = nullptr;
		return false;
	}
	g_origCopyHitData = (CopyHitData_t)g_copyHitDataDetour.GetOriginalTarget();

	constexpr UInt32 kVanillaCopyHitDataThunk = 0x92C400;
	constexpr UInt32 kVtbl_HP = 0x1087FD8;
	constexpr UInt32 kVtbl_MHP = 0x10897C0;
	UInt32 currentHP = *(UInt32*)kVtbl_HP;
	UInt32 currentMHP = *(UInt32*)kVtbl_MHP;
	if (currentHP != kVanillaCopyHitDataThunk) {
		g_origCopyHP = (CopyHitData_t)currentHP;
		SafeWrite32(kVtbl_HP, (UInt32)CopyHitHookHP);
	}
	if (currentMHP != kVanillaCopyHitDataThunk) {
		g_origCopyMHP = (CopyHitData_t)currentMHP;
		SafeWrite32(kVtbl_MHP, (UInt32)CopyHitHookMHP);
	}

	WriteRelJump(kHook_FinishDying, (UInt32)Hook_FinishDying);
	SafeWrite8(kHook_FinishDying + 5, 0x90);
	SafeWrite8(kHook_FinishDying + 6, 0x90);
	return true;
}

} //namespace HitReaction
