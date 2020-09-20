/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Manages and applies all Deadlocked patches.
 * 
 * NOTES :
 * 		Each offset is determined per app id.
 * 		This is to ensure compatibility between versions of Deadlocked/Gladiator.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include "appid.h"
#include "common.h"
#include "player.h"
#include "pad.h"
#include "time.h"
#include "module.h"
#include "game.h"
#include "gamesettings.h"



#if APPID == DL_APPID

/*
 * ------------------------------------------------
 * ----------- START DEADLOCKED OFFSETS -----------
 * ------------------------------------------------
 */

/*
 * Camera speed patch offsets.
 * Each offset is used by PatchCameraSpeed() to change the max
 * camera speed setting in game.
 */
#define CAMERA_SPEED_PATCH_OFF1			(*(u16*)0x00561BB8)
#define CAMERA_SPEED_PATCH_OFF2			(*(u16*)0x00561BDC)


/*
 * ------------------------------------------------
 * ------------ END DEADLOCKED OFFSETS ------------
 * ------------------------------------------------
 */

#endif

// 
void processSpectate(void);


/*
 * NAME :		patchCameraSpeed
 * 
 * DESCRIPTION :
 * 			Patches in-game camera speed setting to max out at 200%.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchCameraSpeed()
{
	// Check if the value is the default max of 64
	// This is to ensure that we only write here when
	// we're in game and the patch hasn't already been applied
	if (CAMERA_SPEED_PATCH_OFF1 == 0x40)
	{
		CAMERA_SPEED_PATCH_OFF1 = 0x80;
		CAMERA_SPEED_PATCH_OFF2 = 0x81;
	}
}


/*
 * NAME :		processGameModules
 * 
 * DESCRIPTION :
 * 
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void processGameModules()
{
	// Start at the first game module
	GameModule * module = GLOBAL_GAME_MODULES_START;

	// Game settings
	GameSettings * gamesettings = GLOBAL_GAMESETTINGS;

	// Iterate through all the game modules until we hit an empty one
	while (module->GameEntrypoint || module->LobbyEntrypoint)
	{
		// Ensure we have game settings
		if (gamesettings)
		{
			// Check the module is enabled
			if (module->State > GAMEMODULE_OFF)
			{
				// If in game, run game entrypoint
				if (GAME_ACTIVE)
				{
					// Check if the game hasn't ended
					// We also give the module a second after the game has ended to
					// do some end game logic
					if (!GAME_HAS_ENDED || GAME_TIME < (GAME_TIME_ENDGAME + TIME_SECOND))
					{
						// Invoke module
						if (module->GameEntrypoint)
							module->GameEntrypoint(module);
					}
					// Game has ended so turn off if temporarily on
					else if (module->State == GAMEMODULE_TEMP_ON)
					{
						module->State = GAMEMODULE_OFF;
					}
				}
				else
				{
					// If the game has started and we're no longer in game
					// Then it must have ended
					if (gamesettings->GameStartTime > 0 && GAME_TIME > gamesettings->GameStartTime && GAME_HAS_ENDED && module->State == GAMEMODULE_TEMP_ON)
						module->State = GAMEMODULE_OFF;
				}
			}

			// Invoke lobby module if still active
			if (!GAME_ACTIVE && module->State > GAMEMODULE_OFF && module->LobbyEntrypoint)
			{
				module->LobbyEntrypoint(module);
			}
		}
		// If we aren't in a game then try to turn the module off
		// ONLY if it's temporarily enabled
		else if (module->State == GAMEMODULE_TEMP_ON)
		{
			module->State = GAMEMODULE_OFF;
		}
		else if (module->State == GAMEMODULE_ALWAYS_ON)
		{
			// Invoke lobby module if still active
			if (!GAME_ACTIVE && module->LobbyEntrypoint)
			{
				module->LobbyEntrypoint(module);
			}
		}

		++module;
	}
}


/*
 * NAME :		main
 * 
 * DESCRIPTION :
 * 			Applies all patches and modules.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int main (void)
{
	// Patch camera speed
	patchCameraSpeed();

	// Process game modules
	processGameModules();

	// Process spectate
	//processSpectate();

	Player * localPlayer = PLAYER_STRUCT_ARRAY[0];
	if (localPlayer)
	{
		PadButtonStatus * pad2 = (PadButtonStatus*)((u8*)localPlayer->Paddata + 0x80);
		if ((localPlayer->Paddata->btns & PAD_LEFT) != (pad2->btns & PAD_LEFT))
		{
			if ((localPlayer->Paddata->btns & PAD_LEFT) == 0)
				printf("%d: PAD LEFT DOWN (0:%04x, 1:%04x)\n", GAME_TIME, localPlayer->Paddata->btns, pad2->btns);
			else
				printf("%d: PAD LEFT UP (0:%04x, 1:%04x)\n", GAME_TIME, localPlayer->Paddata->btns, pad2->btns);
		}
	}

	return 0;
}
