//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "restart.h"

#include "colors.h"
#include "controls.h"
#include "credits.h"
#include "starmap.h"
#include "fmv.h"
#include "menustat.h"
#include "gamestr.h"
#include "globdata.h"
#include "intel.h"
#include "supermelee/melee.h"
#include "resinst.h"
#include "nameref.h"
#include "save.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "setupmenu.h"
#include "util.h"
#include "starcon.h"
#include "uqmversion.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/graphics/sdl/pure.h"
#include "libs/log.h"
#include "options.h"
#include "cons_res.h"

#include "libs/resource/stringbank.h"
// for StringBank_Create() & SplitString()

enum
{
	START_NEW_GAME = 0,
	LOAD_SAVED_GAME,
	PLAY_SUPER_MELEE,
	SETUP_GAME,
	QUIT_GAME
};

static BOOLEAN
PacksInstalled (void)
{
	BOOLEAN packsInstalled;

	if (!IS_HD)
		packsInstalled = TRUE;
	else
		packsInstalled = (HDPackPresent ? TRUE : FALSE);

	return packsInstalled;
}

#define CHOOSER_X (SCREEN_WIDTH >> 1)
#define CHOOSER_Y ((SCREEN_HEIGHT >> 1) - RES_SCALE (12))

#define LARGEST(a,b,c)(((a) > (b) && (a) > (c)) ? (a) : ((b) > (c)) ? (b) : (c))

void
DrawToolTips (int answer, RECT r)
{
	COUNT i;
	TEXT t;
	stringbank *bank = StringBank_Create ();
	const char *lines[30];
	const char *msg[30];
	int line_count, win_w[3], win_h;

	SetContextFont (TinyFont);

	t.pStr = GAME_STRING (MAINMENU_STRING_BASE + 66 + answer);
	line_count = SplitString (t.pStr, '\n', 30, lines, bank);

	/* Compute the dimensions of the label */
	/*win_h = line_count * RES_SCALE(8) + RES_SCALE (8);
	win_w[0] = 0;
	for (i = 0; i < line_count; i++)
	{
		int len = utf8StringCount (lines[i]);
		if (len > win_w[i])
		{
			win_w[i] = RES_SCALE (len);
			printf ("win_w[%d]: %d\n", i, win_w[i]);
		}
	}
	win_w[0] = LARGEST (win_w[0], win_w[1], win_w[2]);
	win_w[0] = (win_w[0] * 5);*/

	win_h = RES_SCALE (3 * 8 + 8);
	win_w[0] = RES_SCALE (49 * 5);
	r.extent = MAKE_EXTENT (win_w[0], win_h);
	r.corner.x = RES_SCALE (
		(RES_DESCALE (ScreenWidth) - RES_DESCALE (win_w[0])) >> 1);

	DrawStarConBox (&r, RES_SCALE (1), BUILD_COLOR_RGBA (24,24,24,255),
			BUILD_COLOR_RGBA (74,74,74,255), TRUE,
			BUILD_COLOR_RGBA (81,81,81,255));
	SetContextForeGroundColor (BLACK_COLOR);

	t.baseline.x = r.corner.x
		+ RES_SCALE (RES_DESCALE (r.extent.width) >> 1);
	t.baseline.y = r.corner.y + RES_SCALE (10)
			+ RES_SCALE (line_count < 2 ? 8 : (line_count > 2 ? 0 : 3));
	for (i = 0; i < line_count; i++)
	{
		t.pStr = lines[i];
		t.align = ALIGN_CENTER;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += RES_SCALE(8);
	}

	StringBank_Free (bank);
}

static void
DrawDiffChooser (FRAME diff, BYTE answer, BOOLEAN confirm)
{
	STAMP s;
	FONT oldFont;
	TEXT t;
	Color c;
	RECT r;
	COUNT i;

	s.origin = MAKE_POINT (CHOOSER_X, CHOOSER_Y);
	s.frame = diff;
	DrawStamp (&s);

	GetFrameRect (diff, &r);
	r.corner.y += CHOOSER_Y + r.extent.height + RES_SCALE (1);
	DrawToolTips (answer, r);

	oldFont = SetContextFont (MicroFont);

	c = SetContextForeGroundColor (MENU_TEXT_COLOR);
	t.align = ALIGN_CENTER;
	t.baseline.x = s.origin.x;
	t.baseline.y = s.origin.y - RES_SCALE (20);

	for (COUNT i = 0; i <= 2; i++)
	{
		t.pStr = GAME_STRING (MAINMENU_STRING_BASE + 56
				+ (!i ? 1 : (i > 1 ? 2 : 0)));

		SetContextForeGroundColor (
				(i == answer) ?
				(confirm ? c : MENU_HIGHLIGHT_COLOR) : BLACK_COLOR
			);
		font_DrawText (&t);

		t.baseline.y += RES_SCALE (23);
	}

	SetContextFont (oldFont);
	SetContextForeGroundColor (c);
}

static BOOLEAN
DoDiffChooser (FRAME diff)
{
	static TimeCount LastInputTime;
	static TimeCount InactTimeOut;
	RECT oldRect;
	STAMP s;
	CONTEXT oldContext;
	BOOLEAN response,done = FALSE;
	BYTE a = 1;

	InactTimeOut = (optMainMenuMusic ? 60 : 20) * ONE_SECOND;
	LastInputTime = GetTimeCounter ();

	oldContext = SetContext (ScreenContext);
	GetContextClipRect (&oldRect);
	s = SaveContextFrame (NULL);

	DrawDiffChooser (diff, a, FALSE);

	FlushGraphics ();
	FlushInput ();

	while (!done)
	{
		UpdateInputState ();
		if (PulsedInputState.menu[KEY_MENU_SELECT])
		{
			done = TRUE;
			response = TRUE;
			PlayMenuSound (MENU_SOUND_SUCCESS);
			DrawDiffChooser (diff, a, TRUE);
		}
		else if (PulsedInputState.menu[KEY_MENU_CANCEL]
				|| CurrentInputState.menu[KEY_EXIT])
		{
			done = TRUE;
			response = FALSE;
			DrawStamp (&s);
		}
		else if (PulsedInputState.menu[KEY_MENU_LEFT]
				|| PulsedInputState.menu[KEY_MENU_UP])
		{
			a--;
			if (a > 254)
				a = 2;
			PlayMenuSound (MENU_SOUND_MOVE);
			DrawDiffChooser (diff, a, FALSE);
			LastInputTime = GetTimeCounter();
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN]
				|| PulsedInputState.menu[KEY_MENU_RIGHT])
		{
			a++;
			if (a > 2)
				a = 0;
			PlayMenuSound (MENU_SOUND_MOVE);
			DrawDiffChooser (diff, a, FALSE);
			LastInputTime = GetTimeCounter ();
		}
		else if (GetTimeCounter () - LastInputTime > InactTimeOut
			&& !optRequiresRestart && PacksInstalled ())
		{	// timed out
			GLOBAL (CurrentActivity) = (ACTIVITY)~0;
			done = TRUE;
			response = FALSE;
		}

		SleepThread (ONE_SECOND / 30);
	}

	if (response)
	{
		optDifficulty = (!a ? OPTVAL_EASY :
				(a > 1 ? OPTVAL_HARD : OPTVAL_NORMAL));

		res_PutInteger ("mm.difficulty", optDifficulty);
	}

	DestroyDrawable (ReleaseDrawable (s.frame));
	FlushInput ();
	
	SetContextClipRect (&oldRect);
	SetContext (oldContext);

	return response;
}


// Draw the full restart menu. Nothing is done with selections.
static void
DrawRestartMenuGraphic (MENU_STATE *pMS)
{
	RECT r;
	STAMP s;
	TEXT t; 
	char *Credit;
	UNICODE buf[64];

	// Re-load all of the restart menu fonts and graphics so the text and
	// background show in the correct size and resolution.
	if (optRequiresRestart || !PacksInstalled ())
	{
		DestroyFont (TinyFont);
		DestroyFont (PlyrFont);
		DestroyFont (StarConFont);
		if (pMS->CurFrame)
		{
			DestroyDrawable (ReleaseDrawable (pMS->CurFrame));
			pMS->CurFrame = 0;
		}
	}

	// Load the different menus and fonts depending on the resolution factor
	if (!IS_HD)
	{
		if (optRequiresRestart || !PacksInstalled ())
		{
			TinyFont = LoadFont (TINY_FONT_FB);
			PlyrFont = LoadFont (PLAYER_FONT_FB);
			StarConFont = LoadFont (STARCON_FONT_FB);
		}
		if (pMS->CurFrame == 0)
			pMS->CurFrame = CaptureDrawable (
					LoadGraphic (RESTART_PMAP_ANIM));
	}
	else
	{
		if (optRequiresRestart || !PacksInstalled ())
		{
			TinyFont = LoadFont (TINY_FONT_HD);
			PlyrFont = LoadFont (PLAYER_FONT_HD);
			StarConFont = LoadFont (STARCON_FONT_HD);
		}
		if (pMS->CurFrame == 0)
			pMS->CurFrame = CaptureDrawable (
					LoadGraphic (RESTART_PMAP_ANIM_HD));
	}

	s.frame = pMS->CurFrame;
	GetFrameRect (s.frame, &r);
	s.origin.x = (SCREEN_WIDTH - r.extent.width) >> 1;
	s.origin.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
	
	SetContextBackGroundColor (BLACK_COLOR);
	BatchGraphics ();
	ClearDrawable ();
	FlushColorXForms ();
	DrawStamp (&s);

	// Put the version number in the bottom right corner.
	SetContextFont (TinyFont);
	t.pStr = buf;
	t.baseline.x = SCREEN_WIDTH - RES_SCALE (2);
	t.baseline.y = SCREEN_HEIGHT - RES_SCALE (2);
	t.align = ALIGN_RIGHT;
	t.CharCount = (COUNT)~0;
	sprintf (buf, "v%d.%d.%g %s",
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION,
			UQM_EXTRA_VERSION);
	SetContextForeGroundColor (WHITE_COLOR);
	font_DrawText (&t);

	// Put the main menu music credit in the bottom left corner.
	if (optMainMenuMusic)
	{
		memset (&buf[0], 0, sizeof (buf));
		t.baseline.x = RES_SCALE (2);
		t.baseline.y = SCREEN_HEIGHT - RES_SCALE (2);
		t.align = ALIGN_LEFT;
		sprintf (buf, "%s %s", GAME_STRING (MAINMENU_STRING_BASE + 61),
				GAME_STRING (MAINMENU_STRING_BASE + 62 + Rando));
		font_DrawText (&t);
	}

	UnbatchGraphics ();
}

static void
DrawRestartMenu (MENU_STATE *pMS, BYTE NewState, FRAME f)
{
	POINT origin;
	origin.x = 0;
	origin.y = 0;
	Flash_setOverlay (pMS->flashContext, &origin, SetAbsFrameIndex (f, NewState + 1), FALSE);
}

static BOOLEAN
RestartMessage (MENU_STATE *pMS, TimeCount TimeIn)
{	
	if (optRequiresRestart)
	{
		SetFlashRect (NULL, FALSE);
		DoPopupWindow (GAME_STRING (MAINMENU_STRING_BASE + 35));
		// Got to restart -message
		SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);	
		SetTransitionSource (NULL);
		SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
		GLOBAL (CurrentActivity) = CHECK_ABORT;
		restartGame = TRUE;
		return TRUE;
	} 
	else if (!PacksInstalled ())
	{
		Flash_pause (pMS->flashContext);
		DoPopupWindow (GAME_STRING (MAINMENU_STRING_BASE + 35 + RESOLUTION_FACTOR));
		// Could not find graphics pack - message
		SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);
		SetTransitionSource (NULL);
		Flash_continue (pMS->flashContext);
		SleepThreadUntil (TimeIn + ONE_SECOND / 30);
		return TRUE;
	} else 
		return FALSE;
}

static void
InitFlash (MENU_STATE *pMS)
{
	pMS->flashContext = Flash_createOverlay (ScreenContext,
		NULL, NULL);
	Flash_setMergeFactors (pMS->flashContext, -3, 3, 16);
	Flash_setSpeed (pMS->flashContext, (6 * ONE_SECOND) / 14, 0,
		(6 * ONE_SECOND) / 14, 0);
	Flash_setFrameTime (pMS->flashContext, ONE_SECOND / 16);
	Flash_setState (pMS->flashContext, FlashState_fadeIn,
		(3 * ONE_SECOND) / 16);
	Flash_setPulseBox (pMS->flashContext, FALSE);

	DrawRestartMenu (pMS, pMS->CurState, pMS->CurFrame);
	Flash_start (pMS->flashContext);
}

static BOOLEAN
DoRestart (MENU_STATE *pMS)
{
	static TimeCount LastInputTime;
	static TimeCount InactTimeOut;
	TimeCount TimeIn = GetTimeCounter ();

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;
	
	if (optSuperMelee && !optLoadGame && PacksInstalled ())
	{
		pMS->CurState = PLAY_SUPER_MELEE;
		PulsedInputState.menu[KEY_MENU_SELECT] = 65535;
	}
	else if (optLoadGame && !optSuperMelee && PacksInstalled ())
	{
		pMS->CurState = LOAD_SAVED_GAME;
		PulsedInputState.menu[KEY_MENU_SELECT] = 65535;
	}

	if (pMS->Initialized)
		Flash_process (pMS->flashContext);

	if (!pMS->Initialized)
	{
		if (pMS->hMusic && !comingFromInit)
		{
			StopMusic ();
			DestroyMusic (pMS->hMusic);
			pMS->hMusic = 0;
		}
		
		pMS->hMusic = loadMainMenuMusic (Rando);
		InactTimeOut = (optMainMenuMusic ? 60 : 20) * ONE_SECOND;

		InitFlash (pMS);
		LastInputTime = GetTimeCounter ();
		pMS->Initialized = TRUE;

		SleepThreadUntil (FadeScreen (FadeAllToColor, ONE_SECOND / 2));
		if (!comingFromInit){
			FadeMusic(0,0);
			PlayMusic (pMS->hMusic, TRUE, 1);
		
			if (optMainMenuMusic)
				FadeMusic (NORMAL_VOLUME+70, ONE_SECOND * 3);
		}
	}
	else if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		return FALSE;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		//BYTE fade_buf[1];
		COUNT oldresfactor;

		switch (pMS->CurState)
		{
			case LOAD_SAVED_GAME:
				if (!RestartMessage (pMS, TimeIn))
				{
					LastActivity = CHECK_LOAD;
					GLOBAL (CurrentActivity) = IN_INTERPLANETARY;
					optLoadGame = FALSE;
				}
				else
					return TRUE;
				break;
			case START_NEW_GAME:
				if (optCustomSeed == 404)
				{
					SetFlashRect (NULL, FALSE);
					DoPopupWindow (
							GAME_STRING (MAINMENU_STRING_BASE + 65));
					// Got to restart -message
					SetMenuSounds (
							MENU_SOUND_UP | MENU_SOUND_DOWN,
							MENU_SOUND_SELECT);
					SetTransitionSource (NULL);
					SleepThreadUntil (
							FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
					GLOBAL (CurrentActivity) = CHECK_ABORT;
					restartGame = TRUE;
				}
				else if (!RestartMessage (pMS, TimeIn))
				{
					if (optDifficulty == OPTVAL_IMPO)
					{
						Flash_terminate (pMS->flashContext); //so it won't visibly stuck
						if (!DoDiffChooser (SetRelFrameIndex(pMS->CurFrame, 6)))
						{
							LastInputTime = GetTimeCounter ();// if we timed out - don't start second credit roll
							InitFlash (pMS);// reinit flash
							return TRUE;
						}
						InitFlash(pMS);// reinit flash
					}
					LastActivity = CHECK_LOAD | CHECK_RESTART;
					GLOBAL (CurrentActivity) = IN_INTERPLANETARY;
				}
				else
					return TRUE;
				break;
			case PLAY_SUPER_MELEE:
				if (!RestartMessage (pMS, TimeIn)) 
				{
					GLOBAL (CurrentActivity) = SUPER_MELEE;
					optSuperMelee = FALSE;
				} 
				else
					return TRUE;
				break;
			case SETUP_GAME:
				oldresfactor = resolutionFactor;

				Flash_terminate (pMS->flashContext);
				SetupMenu ();
				SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN,
						MENU_SOUND_SELECT);

				InactTimeOut = (optMainMenuMusic ? 60 : 20) * ONE_SECOND;

				LastInputTime = GetTimeCounter ();
				SetTransitionSource (NULL);
				BatchGraphics ();
				DrawRestartMenuGraphic (pMS);
				ScreenTransition (3, NULL);
				
				InitFlash (pMS);
				UnbatchGraphics ();
				return TRUE;
			case QUIT_GAME:
				SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
				GLOBAL (CurrentActivity) = CHECK_ABORT;
				break;
		}

		Flash_pause (pMS->flashContext);

		return FALSE;
	}
	else if (PulsedInputState.menu[KEY_MENU_UP] ||
			PulsedInputState.menu[KEY_MENU_DOWN])
	{
		BYTE NewState;

		NewState = pMS->CurState;
		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (NewState == START_NEW_GAME)
				NewState = QUIT_GAME;
			else
				--NewState;
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			if (NewState == QUIT_GAME)
				NewState = START_NEW_GAME;
			else
				++NewState;
		}
		if (NewState != pMS->CurState)
		{
			BatchGraphics ();
			DrawRestartMenu (pMS, NewState, pMS->CurFrame);
			UnbatchGraphics ();
			pMS->CurState = NewState;
		}

		LastInputTime = GetTimeCounter ();
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT] ||
			PulsedInputState.menu[KEY_MENU_RIGHT])
	{	// Does nothing, but counts as input for timeout purposes
		LastInputTime = GetTimeCounter ();
	}
	else if (MouseButtonDown)
	{
		Flash_pause(pMS->flashContext);
		DoPopupWindow (GAME_STRING (MAINMENU_STRING_BASE + 54));
				// Mouse not supported message
		SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);	
		SetTransitionSource (NULL);
		BatchGraphics ();
		DrawRestartMenuGraphic (pMS);
		DrawRestartMenu (pMS, pMS->CurState, pMS->CurFrame);
		ScreenTransition (3, NULL);
		UnbatchGraphics ();
		Flash_continue (pMS->flashContext);

		LastInputTime = GetTimeCounter ();
	}
	else
	{	// No input received, check if timed out
		// JMS: After changing resolution mode, prevent displaying credits
		// (until the next time the game is restarted). This is to prevent
		// showing the credits with the wrong resolution mode's font&background.
		if (GetTimeCounter () - LastInputTime > InactTimeOut
			&& !optRequiresRestart && PacksInstalled ())
		{
			SleepThreadUntil (FadeMusic (0, ONE_SECOND/2));
			StopMusic ();
			FadeMusic (NORMAL_VOLUME, 0);

			GLOBAL (CurrentActivity) = (ACTIVITY)~0;
			return FALSE;
		}
	}
	comingFromInit = FALSE;
	SleepThreadUntil (TimeIn + ONE_SECOND / 30);

	return TRUE;
}

static BOOLEAN
RestartMenu (MENU_STATE *pMS)
{
	TimeCount TimeOut;

	ReinitQueue (&race_q[0]);
	ReinitQueue (&race_q[1]);

	SetContext (ScreenContext);

	GLOBAL (CurrentActivity) |= CHECK_ABORT;
	if (GLOBAL_SIS (CrewEnlisted) == (COUNT)~0
			&& GET_GAME_STATE (UTWIG_BOMB_ON_SHIP)
			&& !GET_GAME_STATE (UTWIG_BOMB)
			&& DeathBySuicide)
	{	// player blew himself up with Utwig bomb
		SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, 0);

		SleepThreadUntil (FadeScreen (FadeAllToWhite, ONE_SECOND / 8)
				+ ONE_SECOND / 60);
		SetContextBackGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));

		ClearDrawable ();
		FlushColorXForms ();
		TimeOut = ONE_SECOND / 8;

		GLOBAL(CurrentActivity) = IN_ENCOUNTER;

		if (optGameOver)
			GameOver (SUICIDE);

		DeathBySuicide = FALSE;

		FreeGameData();
		GLOBAL(CurrentActivity) = CHECK_ABORT;
	}
	else 
	{
		TimeOut = ONE_SECOND / 2;

		if (GLOBAL_SIS (CrewEnlisted) == (COUNT)~0) 
		{
			GLOBAL(CurrentActivity) = IN_ENCOUNTER;

			if (DeathByMelee && GLOBAL_SIS (CrewEnlisted) == (COUNT)~0) {
				if (optGameOver)
					GameOver (DIED_IN_BATTLE);
				DeathByMelee = FALSE;
			}

			if (DeathBySurrender && GLOBAL_SIS (CrewEnlisted) == (COUNT)~0) {
				if (optGameOver)
					GameOver (SURRENDERED);
				DeathBySurrender = FALSE;
			}
		}

		if (LOBYTE (LastActivity) == WON_LAST_BATTLE)
		{
			GLOBAL (CurrentActivity) = WON_LAST_BATTLE;
			Victory ();
			Credits (TRUE);
		}

		FreeGameData ();
		GLOBAL (CurrentActivity) = CHECK_ABORT;
	}

	LastActivity = 0;
	NextActivity = 0;

	// TODO: This fade is not always necessary, especially after a splash
	//   screen. It only makes a user wait.
	SleepThreadUntil (FadeScreen (FadeAllToBlack, TimeOut));
	if (TimeOut == ONE_SECOND / 8)
		SleepThread (ONE_SECOND * 3);

	DrawRestartMenuGraphic (pMS);
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay ();
	DoInput (pMS, TRUE);
	
	if (optMainMenuMusic)
		SleepThreadUntil (FadeMusic (0, ONE_SECOND));

	StopMusic ();
	if (pMS->hMusic)
	{
		DestroyMusic (pMS->hMusic);
		pMS->hMusic = 0;

		if (optMainMenuMusic)
			FadeMusic (NORMAL_VOLUME, 0);
	}

	Flash_terminate (pMS->flashContext);
	pMS->flashContext = 0;
	DestroyDrawable (ReleaseDrawable (pMS->CurFrame));
	pMS->CurFrame = 0;

	if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
		return (FALSE); // timed out

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE); // quit

	TimeOut = FadeScreen (FadeAllToBlack, ONE_SECOND / 2);
	
	SleepThreadUntil (TimeOut);
	FlushColorXForms ();

	SeedRandomNumbers ();

	return (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE);
}

static BOOLEAN
TryStartGame (void)
{
	MENU_STATE MenuState;

	LastActivity = GLOBAL (CurrentActivity);
	GLOBAL (CurrentActivity) = 0;

	memset (&MenuState, 0, sizeof (MenuState));
	MenuState.InputFunc = DoRestart;

	while (!RestartMenu (&MenuState))
	{	// spin until a game is started or loaded
		if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE &&
				!(GLOBAL (CurrentActivity) & CHECK_ABORT))
		{
			FreeGameData ();
			Melee ();
			MenuState.Initialized = FALSE;
		}
		else if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
		{	// timed out
			SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
			return (FALSE);
		}
		else if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		{	// quit
			return (FALSE);
		}
	}

	return TRUE;
}

BOOLEAN
StartGame (void)
{
	do
	{
		while (!TryStartGame ())
		{
			if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
			{	// timed out
				GLOBAL (CurrentActivity) = 0;
				SplashScreen (0);
				if (optWhichIntro == OPT_3DO)
					Drumall ();
				Credits (FALSE);
			}

			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				return (FALSE); // quit
		}

		if (LastActivity & CHECK_RESTART)
		{	// starting a new game
			FadeMusic (NORMAL_VOLUME, 0);
			if (!optSkipIntro)
				Introduction ();
		}
	
	} while (GLOBAL (CurrentActivity) & CHECK_ABORT);

	{
		extern STAR_DESC starmap_array[];
		extern const BYTE element_array[];
		extern const PlanetFrame planet_array[];
		extern POINT constell_array[];

		star_array = starmap_array;
		Elements = element_array;
		PlanData = planet_array;
		constel_array = constell_array;
	}

	PlayerControl[0] = HUMAN_CONTROL | STANDARD_RATING;
	PlayerControl[1] = COMPUTER_CONTROL | AWESOME_RATING;

	return (TRUE);
}

