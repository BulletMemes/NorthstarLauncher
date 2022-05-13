#include "pch.h"
#include "host.h"
#include "convar.h"
#include "modmanager.h"
#include "commandprint.h"
#include "mapsprint.h"
#include "r2engine.h"

typedef void (*Host_InitType)(bool bDedicated);
Host_InitType Host_Init;
void Host_InitHook(bool bDedicated) 
{
	Host_Init(bDedicated);

	// get all mod convars
	std::vector<std::string> vModConvarNames;
	for (auto& mod : g_pModManager->m_loadedMods)
		for (auto& cvar : mod.ConVars)
			vModConvarNames.push_back(cvar->Name);

	// strip hidden and devonly cvar flags
	int iCvarsAltered = 0;
	for (auto& pair : g_pCVar->DumpToMap())
	{
		// don't remove from mod cvars
		if (std::find(vModConvarNames.begin(), vModConvarNames.end(), pair.second->m_pszName) != vModConvarNames.end())
			continue;

		// strip flags
		int flags = pair.second->GetFlags();
		if (flags & FCVAR_DEVELOPMENTONLY)
		{
			flags &= ~FCVAR_DEVELOPMENTONLY;
			iCvarsAltered++;
		}

		if (flags & FCVAR_HIDDEN)
		{
			flags &= ~FCVAR_HIDDEN;
			iCvarsAltered++;
		}

		pair.second->m_nFlags = flags;
	}

	spdlog::info("Removed {} hidden/devonly cvar flags", iCvarsAltered);

	// make servers able to run disconnect on clients
	g_pCVar->FindCommand("disconnect")->m_nFlags |= FCVAR_SERVER_CAN_EXECUTE;

	// need to initialise these after host_init since they do stuff to preexisting concommands/convars
	InitialiseCommandPrint();
	InitialiseMapsPrint();

	// run client autoexec if on client
	if (!bDedicated)
		R2::Cbuf_AddText(R2::Cbuf_GetCurrentPlayer(), "exec autoexec_ns_client", R2::cmd_source_t::kCommandSrcCode);
}

ON_DLL_LOAD("engine.dll", Host_Init, [](HMODULE baseAddress)
{ 
	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x155EA0, &Host_InitHook, reinterpret_cast<LPVOID*>(&Host_Init));
})