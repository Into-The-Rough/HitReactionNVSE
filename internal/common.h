#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstddef>
#include <float.h>
#include <utility>

typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned int UInt32;
typedef int SInt32;

struct PluginInfo {
	enum { kInfoVersion = 1 };
	UInt32 infoVersion;
	const char* name;
	UInt32 version;
};

struct NVSEInterface {
	UInt32 nvseVersion;
	UInt32 runtimeVersion;
	UInt32 editorVersion;
	UInt32 isEditor;
	void* RegisterCommand;
	void* SetOpcodeBase;
	void* (*QueryInterface)(UInt32 id);
	UInt32 (*GetPluginHandle)(void);
	void* RegisterTypedCommand;
	const char* (*GetRuntimeDirectory)(void);
	UInt32 isNogore;
};

enum { kInterface_Messaging = 2 };

struct NVSEMessagingInterface {
	UInt32 version;
	void (*RegisterListener)(UInt32 pluginHandle, const char* sender, void* callback);
	void* Dispatch;
};

enum {
	kMessage_PostLoad = 0,
	kMessage_ExitGame,
	kMessage_ExitToMainMenu,
	kMessage_LoadGame,
	kMessage_SaveGame,
	kMessage_Precompile,
	kMessage_PreLoadGame,
	kMessage_ExitGame_Console,
	kMessage_PostLoadGame,
	kMessage_PostPostLoad,
	kMessage_RuntimeScriptError,
	kMessage_DeleteGame,
	kMessage_RenameGame,
	kMessage_RenameNewGame,
	kMessage_NewGame,
	kMessage_DeleteGameName,
	kMessage_RenameGameName,
	kMessage_RenameNewGameName,
	kMessage_DeferredInit,
	kMessage_ClearScriptDataCache,
	kMessage_MainGameLoop,
	kMessage_ReloadConfig = 25
};

struct NVSEMessage {
	const char* sender;
	UInt32 type;
	UInt32 dataLen;
	void* data;
};

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret ThisCall(UInt32 addr, void* _this, Args ...args) {
	return ((T_Ret(__thiscall*)(void*, Args...))addr)(_this, std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret CdeclCall(UInt32 addr, Args ...args) {
	return ((T_Ret(__cdecl*)(Args...))addr)(std::forward<Args>(args)...);
}

struct NiPoint3 { float x, y, z; };
struct alignas(16) hkVector4 { float x, y, z, w; };
struct NiMatrix33 { float m[3][3]; };
struct NiUpdateData {
	float timePassed;
	bool updateControllers;
	bool isMultiThreaded;
	UInt8 unk06;
	bool updateGeomorphs;
	bool updateShadowScene;
	UInt8 pad09[3];
};
static_assert(sizeof(NiUpdateData) == 0x0C, "NiUpdateData");

inline char g_iniPath[MAX_PATH] = {};
inline NVSEMessagingInterface* g_msgInterface = nullptr;
inline UInt32 g_pluginHandle = 0;

inline void SafeWrite8(UInt32 addr, UInt8 data) {
	DWORD old;
	VirtualProtect((void*)addr, 1, PAGE_EXECUTE_READWRITE, &old);
	*(UInt8*)addr = data;
	VirtualProtect((void*)addr, 1, old, &old);
	FlushInstructionCache(GetCurrentProcess(), (void*)addr, 1);
}

inline void SafeWrite32(UInt32 addr, UInt32 data) {
	DWORD old;
	VirtualProtect((void*)addr, 4, PAGE_EXECUTE_READWRITE, &old);
	*(UInt32*)addr = data;
	VirtualProtect((void*)addr, 4, old, &old);
	FlushInstructionCache(GetCurrentProcess(), (void*)addr, 4);
}

inline void WriteRelJump(UInt32 addr, UInt32 target) {
	SafeWrite8(addr, 0xE9);
	SafeWrite32(addr + 1, target - (addr + 5));
}

class CallDetour {
	UInt32 sourceAddr = 0;
	UInt32 originalTarget = 0;
	SInt32 originalRel = 0;
	SInt32 installedRel = 0;

public:
	bool Install(UInt32 source, UInt32 target) {
		if (sourceAddr || *(UInt8*)source != 0xE8)
			return false;
		DWORD old;
		if (!VirtualProtect((void*)source, 5, PAGE_EXECUTE_READWRITE, &old))
			return false;
		originalRel = *(SInt32*)(source + 1);
		originalTarget = source + 5 + originalRel;
		installedRel = target - (source + 5);
		*(SInt32*)(source + 1) = installedRel;
		sourceAddr = source;
		VirtualProtect((void*)source, 5, old, &old);
		FlushInstructionCache(GetCurrentProcess(), (void*)source, 5);
		return true;
	}

	bool Restore() {
		if (!sourceAddr || *(UInt8*)sourceAddr != 0xE8 || *(SInt32*)(sourceAddr + 1) != installedRel)
			return false;
		DWORD old;
		if (!VirtualProtect((void*)sourceAddr, 5, PAGE_EXECUTE_READWRITE, &old))
			return false;
		*(SInt32*)(sourceAddr + 1) = originalRel;
		VirtualProtect((void*)sourceAddr, 5, old, &old);
		FlushInstructionCache(GetCurrentProcess(), (void*)sourceAddr, 5);
		sourceAddr = 0;
		return true;
	}

	UInt32 GetOriginalTarget() const {
		return originalTarget;
	}
};

inline float Clampf(float v, float lo, float hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}

inline float RandFloat() {
	return rand() / (float)RAND_MAX;
}

inline NiPoint3 Normalize(NiPoint3 v) {
	float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len < 0.0001f) return {0, 1.0f, 0};
	float inv = 1.0f / len;
	return {v.x * inv, v.y * inv, v.z * inv};
}

inline NiPoint3 AddVec(const NiPoint3& a, const NiPoint3& b) {
	return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline NiPoint3 ScaleVec(const NiPoint3& v, float scale) {
	return {v.x * scale, v.y * scale, v.z * scale};
}

inline NiPoint3 ClampVec(NiPoint3 v, float maxDeg) {
	float r = maxDeg * 0.01745329f;
	v.x = Clampf(v.x, -r, r);
	v.y = Clampf(v.y, -r, r);
	v.z = Clampf(v.z, -r, r);
	return v;
}
