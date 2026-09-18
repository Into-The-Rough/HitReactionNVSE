#include "internal/common.h"
#include "internal/game.h"
#include "internal/config.h"
#include "internal/hit_reaction.h"

static bool g_hooksInstalled = false;

static void MessageHandler(NVSEMessage* msg) {
	if (msg->type == kMessage_PostPostLoad && !g_hooksInstalled) {
		g_hooksInstalled = HitReaction::InitHooks();
		if (!g_hooksInstalled)
			Console_Print("HitReactionNVSE: incompatible hook conflict, plugin disabled");
	}
	else if (msg->type == kMessage_MainGameLoop) {
		HitReaction::Simulate();
	}
	else if (msg->type == kMessage_ExitToMainMenu || msg->type == kMessage_PreLoadGame) {
		HitReaction::Clear();
	}
	else if (msg->type == kMessage_ReloadConfig) {
		if (msg->data && msg->dataLen > 0) {
			if (g_iniPath[0] && _stricmp((const char*)msg->data, "HitReactionNVSE") == 0) {
				Config::Load(g_iniPath);
				Console_Print("HitReactionNVSE: config reloaded");
			}
		}
	}
}

extern "C" {

__declspec(dllexport) bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "HitReactionNVSE";
	info->version = 128;
	if (nvse->isEditor) return false;
	if (nvse->isNogore) return false; //nogore exe has shifted addresses
	return true;
}

__declspec(dllexport) bool NVSEPlugin_Load(const NVSEInterface* nvse) {
	srand(GetTickCount());
	int len = snprintf(g_iniPath, sizeof(g_iniPath), "%sData\\config\\HitReactionNVSE.ini", nvse->GetRuntimeDirectory());
	if (len < 0 || len >= (int)sizeof(g_iniPath))
		g_iniPath[0] = 0;
	if (g_iniPath[0])
		Config::Load(g_iniPath);

	g_pluginHandle = nvse->GetPluginHandle();
	g_msgInterface = (NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging);
	if (g_msgInterface)
		g_msgInterface->RegisterListener(g_pluginHandle, "NVSE", (void*)MessageHandler);

	return true;
}

}

BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
	return TRUE;
}
