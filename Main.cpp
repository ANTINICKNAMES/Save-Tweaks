#include "plugin.h"

#include <CWorld.h>

#include "IniReader.h"
#include <format>

#include "AntistealingMeasures.h"

using namespace plugin;

//bool bDisableHPOverride = false;
int32_t iHPOverrideCount = 200;
bool bRestoreHealthAfterSave = true;

//bool bDisableHPOverride = false;
int32_t iArmourOverrideCount = 255;
bool bRestoreArmourAfterSave = true;

bool bOverrideHungerHours = false;

bool bRestoreSprintWhenSaving = true;
bool bRestoreBreathWhenSaving = false;

bool bRestoreSprintAfterSaving = false;
bool bRestoreBreathAfterSaving = false;

int32_t iPassTime = 360;
bool bLoseWanterWhenSaving = false;
std::string sSaveFileName;

char(&DefaultPCSaveFileName)[MAX_PATH] = *(char (*)[MAX_PATH])0xC16F18;

struct SaveTweaks_SA 
{
	SaveTweaks_SA()
	{
		Plugin_HookProcessor::CheckForPluginStatus();

		CIniReader ini(PLUGIN_PATH("SaveTweaks.ini"));

		auto section = std::format("{}", "SAVE");

		//bDisableHPOverride = ini.ReadBoolean(section, "DISABLE_HP_OVERRIDE", false);
		iHPOverrideCount = ini.ReadInteger(section, "HP_OVERRIDE_COUNT", -1);
		bRestoreHealthAfterSave = ini.ReadBoolean(section, "RESTORE_PLAYER_HEALTH_AFTER_SAVING", true);

		//bDisableArmourOverride = ini.ReadBoolean(section, "DISABLE_ARMOUR_OVERRIDE", false);
		iArmourOverrideCount = ini.ReadInteger(section, "ARMOUR_OVERRIDE_COUNT", -1);
		bRestoreArmourAfterSave = ini.ReadBoolean(section, "RESTORE_PLAYER_ARMOUR_AFTER_SAVING", false);

		bOverrideHungerHours = ini.ReadBoolean(section, "OVERRIDE_HUNGER_HOURS", true);

		bRestoreSprintWhenSaving = ini.ReadBoolean(section, "RESTORE_SPRINT_WHEN_SAVING", true);
		bRestoreBreathWhenSaving = ini.ReadBoolean(section, "RESTORE_BREATH_WHEN_SAVING", false);

		bRestoreSprintAfterSaving = ini.ReadBoolean(section, "RESTORE_SPRINT_AFTER_SAVE_LOAD", false);
		bRestoreBreathAfterSaving = ini.ReadBoolean(section, "RESTORE_BREATH_AFTER_SAVE_LOAD", false);

		iPassTime = ini.ReadInteger(section, "PASS_TIME_MINS", -1);
		bLoseWanterWhenSaving = ini.ReadBoolean(section, "LOSE_WANTED_WHEN_SAVING", true);

		sSaveFileName = ini.ReadString(section, "SAVE_FILE_NAME", "GTASAsf");

		//(drop check if player health is greater than 200)
		//if (bDisableHPOverride)
			//patch::RedirectJump(0x618F6D, (void*)0x618F64);

		//if (!bDisableHPOverride && iHPOverrideCount > 0)
		if (iHPOverrideCount > 0)
		{
			//patch::SetUInt(0x618F60 + 1, iHPOverrideCount);
			injector::WriteMemory<uint8_t>(0x618F60 + 1, iHPOverrideCount, true);
			patch::SetInt(0x618F6D + 4, iHPOverrideCount);
		}

		// restore health nop
		if (!bRestoreHealthAfterSave)
			patch::Nop(0x618F81, 6);

		// NumHoursDidntEat override nop
		if (!bOverrideHungerHours)
			patch::Nop(0x618FB4, 9);

		// restore sprint energy and breath before loading
		//if (bRestoreSprintBreathWhenSaving)
			patch::RedirectCall(0x618FC7, RestoreSprintEnergyAndBreathAtSaving); // this shit also purposes for armor

		// restore sprint energy and breath after loading
		//if (bRestoreSprintBreathAfterSaving)
			patch::RedirectCall(0x618F05, RestoreSprintEnergyAndBreathAtSaveLoad);

		if (iPassTime > -1)
			patch::SetInt(0x618F9B + 1, iPassTime);

		if (!bLoseWanterWhenSaving)
			patch::Nop(0x618FCC, 19);

		// set my savefile names
		patch::ReplaceFunction(0x619040, InitSaveFileNames);
	};

	static void __cdecl RestoreSprintEnergyAndBreathAtSaving()
	{
		if (bRestoreSprintWhenSaving)
			FindPlayerPed()->ResetSprintEnergy();

		if (bRestoreBreathWhenSaving)
			FindPlayerPed()->ResetPlayerBreath();

		// armour stuff

		if (bRestoreArmourAfterSave)
		{
			int8_t iArmourToRestore = CWorld::Players[CWorld::PlayerInFocus].m_nMaxArmour;

			if (iArmourOverrideCount > -1 && iArmourToRestore > iArmourOverrideCount)
				iArmourToRestore = iArmourOverrideCount;

			CWorld::Players[0].m_pPed->m_fArmour = (float)iArmourToRestore;
		}
	}

	static void __cdecl RestoreSprintEnergyAndBreathAtSaveLoad()
	{
		if (bRestoreSprintAfterSaving)
			FindPlayerPed()->ResetSprintEnergy();
		if (bRestoreBreathAfterSaving)
			FindPlayerPed()->ResetPlayerBreath();

		CTheScripts::Process();
	}

	static int __cdecl InitSaveFileNames(char* userPath)
	{
		return sprintf_s(DefaultPCSaveFileName, "%s\\%s", userPath, sSaveFileName.data());
	}
} SaveTweaks_SA;
