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

#include "controls.h"
#include "util.h"
#include "setup.h"
#include "units.h"
#include "settings.h"
#include "libs/inplib.h"
#include "libs/sound/trackplayer.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include "hyper.h"
#include "colors.h"
#include "sis.h"
#include "menustat.h"
#include "gamestr.h"
#include "shipcont.h"

void
DrawStarConBox (RECT *pRect, SIZE BorderWidth, Color TopLeftColor,
		Color BottomRightColor, BOOLEAN FillInterior, Color InteriorColor,
		BOOLEAN CreateCorners, Color CornerColor)
{
	RECT locRect;
	Color oldcolor;

	BatchGraphics ();

	if (FillInterior)
	{
		oldcolor = SetContextForeGroundColor (InteriorColor);
		DrawFilledRectangle (pRect);
		SetContextForeGroundColor (TopLeftColor);
	}
	else
		oldcolor = SetContextForeGroundColor (TopLeftColor);

	if (BorderWidth == 0)
		BorderWidth = RES_SCALE (2);

	locRect.corner = pRect->corner;
	locRect.extent.width = pRect->extent.width;
	locRect.extent.height = RES_SCALE (1);
	DrawFilledRectangle (&locRect);

	if (BorderWidth == RES_SCALE (2))
	{
		locRect.corner.x += RES_SCALE (1);
		locRect.corner.y += RES_SCALE (1);
		locRect.extent.width -= RES_SCALE (2);
		DrawFilledRectangle (&locRect);
	}

	locRect.corner = pRect->corner;
	locRect.extent.width = RES_SCALE (1);
	locRect.extent.height = pRect->extent.height;
	DrawFilledRectangle (&locRect);

	if (BorderWidth == RES_SCALE (2))
	{
		locRect.corner.x += RES_SCALE (1);
		locRect.corner.y += RES_SCALE (1);
		locRect.extent.height -= RES_SCALE (2);
		DrawFilledRectangle (&locRect);
	}

	SetContextForeGroundColor (BottomRightColor);
	locRect.corner.x = pRect->corner.x + pRect->extent.width - RES_SCALE (1);
	locRect.corner.y = pRect->corner.y + RES_SCALE (1);
	locRect.extent.height = pRect->extent.height - RES_SCALE (1);
	DrawFilledRectangle (&locRect);

	if (BorderWidth == RES_SCALE (2))
	{
		locRect.corner.x -= RES_SCALE (1);
		locRect.corner.y += RES_SCALE (1);
		locRect.extent.height -= RES_SCALE (2);
		DrawFilledRectangle (&locRect);
	}

	locRect.corner.x = pRect->corner.x;
	locRect.extent.width = pRect->extent.width;
	locRect.corner.y = pRect->corner.y + pRect->extent.height - RES_SCALE (1);
	locRect.extent.height = RES_SCALE (1);
	DrawFilledRectangle (&locRect);

	if (BorderWidth == RES_SCALE (2))
	{
		locRect.corner.x += RES_SCALE (1);
		locRect.corner.y -= RES_SCALE (1);
		locRect.extent.width -= RES_SCALE (2);
		DrawFilledRectangle (&locRect);
	}

	if (CreateCorners)
	{
		if (sameColor (TRANSPARENT, CornerColor)
				&& AreTheyShades (TopLeftColor, BottomRightColor))
		{
			SetContextForeGroundColor (
					CreateAvgShade (TopLeftColor, BottomRightColor));
		}
		else
			SetContextForeGroundColor (CornerColor);

		locRect.corner.x = pRect->corner.x;
		locRect.corner.y = pRect->corner.y + pRect->extent.height
			- RES_SCALE (1);
		locRect.extent.width = RES_SCALE (1);
		locRect.extent.height = RES_SCALE (1);
		DrawFilledRectangle (&locRect);
		locRect.corner.x = pRect->corner.x + pRect->extent.width
			- RES_SCALE (1);
		locRect.corner.y = pRect->corner.y;
		DrawFilledRectangle (&locRect);

		if (BorderWidth == RES_SCALE (2))
		{
			locRect.corner.x -= RES_SCALE (1);
			locRect.corner.y += RES_SCALE (1);
			DrawFilledRectangle (&locRect);
			locRect.corner.x = pRect->corner.x + RES_SCALE (1);
			locRect.corner.y = pRect->corner.y + pRect->extent.height
				- RES_SCALE (2);
			DrawFilledRectangle (&locRect);
		}
	}

	SetContextForeGroundColor (oldcolor);

	UnbatchGraphics ();
}

void
DrawRenderedBox (RECT *r, BOOLEAN filled, Color fill_color, SCB_TYPE type)
{
	int i;
	STAMP stamp;
	COORD columns = r->extent.width;
	COORD rows = r->extent.height;

	if (!r->extent.width || !r->extent.height)
		return;

	BatchGraphics ();

	if (filled)
	{
		Color OldColor = SetContextForeGroundColor (fill_color);

		if (type == SPECIAL_BEVEL)
		{
			r->corner.x += RES_SCALE (4);
			r->corner.y += RES_SCALE (4);
			r->extent.width -= RES_SCALE (8);
			r->extent.height -= RES_SCALE (8);
		}

		DrawFilledRectangle (r);

		if (type == SPECIAL_BEVEL)
		{
			r->corner.x -= RES_SCALE (4);
			r->corner.y -= RES_SCALE (4);
			r->extent.width += RES_SCALE (8);
			r->extent.height += RES_SCALE (8);
		}

		SetContextForeGroundColor (OldColor);
	}

	stamp.frame = optCustomBorder && LOBYTE (GLOBAL (CurrentActivity))
			!= SUPER_MELEE ? BorderFrame : HDBorderFrame;

	if (type == SPECIAL_BEVEL)
	{
		columns -= RES_SCALE (8);
		rows -= RES_SCALE (8);
	}

	for (i = 0; i < columns; i++)
	{	// Draw top and bottom borders
		stamp.frame = SetAbsFrameIndex (stamp.frame, type + 4);
		stamp.origin.x = r->corner.x + i;
		stamp.origin.y = r->corner.y;
		DrawStamp (&stamp);
		stamp.frame = SetAbsFrameIndex (stamp.frame, type + 6);
		stamp.origin.y = r->corner.y + r->extent.height;
		DrawStamp (&stamp);
	}

	for (i = 0; i < rows; i++)
	{	// Draw right and left borders
		stamp.frame = SetAbsFrameIndex (stamp.frame, type + 5);
		stamp.origin.x = r->corner.x + r->extent.width;
		stamp.origin.y = r->corner.y + i;
		DrawStamp (&stamp);
		stamp.frame = SetAbsFrameIndex (stamp.frame, type + 7);
		stamp.origin.x = r->corner.x;
		DrawStamp (&stamp);
	}

	// Draw corners clockwise from the top-left
	stamp.frame = SetAbsFrameIndex (stamp.frame, type);
	stamp.origin.x = r->corner.x;
	stamp.origin.y = r->corner.y;
	DrawStamp (&stamp);

	stamp.frame = SetAbsFrameIndex (stamp.frame, type + 1);
	stamp.origin.x = r->corner.x + r->extent.width;
	DrawStamp (&stamp);

	stamp.frame = SetAbsFrameIndex (stamp.frame, type + 2);
	stamp.origin.y = r->corner.y + r->extent.height;
	DrawStamp (&stamp);

	stamp.frame = SetAbsFrameIndex (stamp.frame, type + 3);
	stamp.origin.x = r->corner.x;
	DrawStamp (&stamp);

	UnbatchGraphics ();
}

void
DrawBorderPadding (DWORD videoWidth)
{
	RECT r;
	CONTEXT OldContext;
	UWORD safe_x =
			(videoWidth && videoWidth < 280 ? SAFE_NEG (4) * 2 : SAFE_X);

	if (!safe_x)
		return;

	OldContext = SetContext (ScreenContext);

	if (videoWidth)
		SetContextForeGroundColor (BUILD_SHADE_RGBA (0x0C));
	else
		SetContextForeGroundColor (BLACK_COLOR);

	// Top bar
	r.corner = MAKE_POINT (0, 0);
	r.extent.width = ScreenWidth;
	r.extent.height = SAFE_Y;
	DrawFilledRectangle (&r);

	// Right bar
	r.corner.x = r.extent.width - safe_x;
	r.extent.width = safe_x;
	r.extent.height = ScreenHeight;
	DrawFilledRectangle (&r);

	// Bottom bar
	r.corner.x = 0;
	r.corner.y = ScreenHeight - SAFE_Y;
	r.extent.width = ScreenWidth;
	r.extent.height = SAFE_Y;
	DrawFilledRectangle (&r);

	// Left bar
	r.corner = MAKE_POINT (0, 0);
	r.extent.width = safe_x;
	r.extent.height = ScreenHeight;
	DrawFilledRectangle (&r);

	SetContext (OldContext);
}

void
DrawRadarBorder (void)
{
	RECT r;
	CONTEXT OldContext;

	if (IS_PAD)
		return;

	OldContext = SetContext (StatusContext);

	r.corner.x = RES_SCALE (4) - RES_SCALE (1);
	r.corner.y = RADAR_Y - RES_SCALE (1);
	r.extent.width = RADAR_WIDTH + RES_SCALE (2);
	r.extent.height = RADAR_HEIGHT + RES_SCALE (2);

	if (IS_HD || optCustomBorder)
	{
		DrawRenderedBox (&r, FALSE, NULL_COLOR, THIN_INNER_BEVEL);
	}
	else
	{
		DrawStarConBox (&r, RES_SCALE (1), ALT_SHADOWBOX_TOP_LEFT,
				ALT_SHADOWBOX_BOTTOM_RIGHT, FALSE, TRANSPARENT, FALSE,
				TRANSPARENT);
	}

	SetContext (OldContext);
}

DWORD
SeedRandomNumbers (void)
{
	DWORD cur_time;

	cur_time = GetTimeCounter ();
	TFB_SeedRandom (cur_time);

	return (cur_time);
}

STAMP
SaveContextFrame (const RECT *saveRect)
{
	STAMP s;

	if (saveRect)
	{	// a portion of the context
		s.origin = saveRect->corner;
	}
	else
	{	// the entire context
		s.origin.x = 0;
		s.origin.y = 0;
	}

	s.frame = CaptureDrawable (CopyContextRect (saveRect));

	return s;
}

BOOLEAN
PauseGame (void)
{
	RECT r;
	STAMP s;
	CONTEXT OldContext;
	STAMP saveStamp;
	RECT ctxRect;
	POINT oldOrigin;
	RECT OldRect;

	if (ActivityFrame == 0
			|| (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_PAUSE))
			|| (LastActivity & (CHECK_LOAD | CHECK_RESTART)))
		return (FALSE);
		
	GLOBAL (CurrentActivity) |= CHECK_PAUSE;

	if (PlayingTrack ())
		PauseTrack ();

	OldContext = SetContext (ScreenContext);
	oldOrigin = SetContextOrigin (MAKE_POINT (0, 0));
	GetContextClipRect (&OldRect);
	SetContextClipRect (NULL);

	GetContextClipRect (&ctxRect);
	GetFrameRect (ActivityFrame, &r);
	r.corner.x = (ctxRect.extent.width - r.extent.width) >> 1;
	r.corner.y = (ctxRect.extent.height - r.extent.height) >> 1;
	saveStamp = SaveContextFrame (&r);

	// TODO: This should draw a localizable text message instead
	s.origin = r.corner;
	s.frame = ActivityFrame;
	SetSystemRect (&r);
	DrawStamp (&s);

	FlushGraphics ();

	while (ImmediateInputState.menu[KEY_PAUSE] && GamePaused)
	{
		BeginInputFrame ();
		TaskSwitch ();
	}

	while (!ImmediateInputState.menu[KEY_PAUSE] && GamePaused)
	{
		BeginInputFrame ();
		TaskSwitch ();
	}

	while (ImmediateInputState.menu[KEY_PAUSE] && GamePaused)
	{
		BeginInputFrame ();
		TaskSwitch ();
	}

	GamePaused = FALSE;

	DrawStamp (&saveStamp);
	DestroyDrawable (ReleaseDrawable (saveStamp.frame));
	ClearSystemRect ();

	SetContextClipRect (&OldRect);
	SetContextOrigin (oldOrigin);
	SetContext (OldContext);

	WaitForNoInput (ONE_SECOND / 4, TRUE);

	if (PlayingTrack ())
		ResumeTrack ();


	TaskSwitch ();
	GLOBAL (CurrentActivity) &= ~CHECK_PAUSE;
	return (TRUE);
}

// Waits for a button to be pressed
// Returns TRUE if the wait succeeded (found input)
//    FALSE if timed out or game aborted
BOOLEAN
WaitForAnyButtonUntil (BOOLEAN newButton, TimeCount timeOut,
		BOOLEAN resetInput)
{
	BOOLEAN buttonPressed;

	if (newButton && !WaitForNoInputUntil (timeOut, FALSE))
		return FALSE;

	buttonPressed = AnyButtonPress (TRUE);
	while (!buttonPressed
			&& (timeOut == WAIT_INFINITE || GetTimeCounter () < timeOut)
			&& !(GLOBAL (CurrentActivity) & CHECK_ABORT)
			&& !QuitPosted)
	{
		SleepThread (ONE_SECOND / 40);
		buttonPressed = AnyButtonPress (TRUE);
	} 

	if (resetInput)
		FlushInput ();

	return buttonPressed;
}

// Waits for action button to be pressed
// Returns TRUE if the wait succeeded (found input)
//    FALSE if timed out or game aborted
BOOLEAN
WaitForActButtonUntil (BOOLEAN newButton, TimeCount timeOut,
		BOOLEAN resetInput)
{
	BOOLEAN buttonPressed;

	if (newButton && !WaitForNoInputUntil (timeOut, FALSE))
		return FALSE;

	buttonPressed = ActKeysPress ();
	while (!buttonPressed
			&& (timeOut == WAIT_INFINITE || GetTimeCounter () < timeOut)
			&& !(GLOBAL (CurrentActivity) & CHECK_ABORT)
			&& !QuitPosted)
	{
		SleepThread (ONE_SECOND / 40);
		buttonPressed = ActKeysPress ();
	} 

	if (resetInput)
		FlushInput ();

	return buttonPressed;
}

BOOLEAN
WaitForAnyButton (BOOLEAN newButton, TimePeriod duration, BOOLEAN resetInput)
{
	TimeCount timeOut = duration;
	if (duration != WAIT_INFINITE)
		timeOut += GetTimeCounter ();
	return WaitForAnyButtonUntil (newButton, timeOut, resetInput);
}

BOOLEAN
WaitForActButton (BOOLEAN newButton, TimePeriod duration, BOOLEAN resetInput)
{
	TimeCount timeOut = duration;
	if (duration != WAIT_INFINITE)
		timeOut += GetTimeCounter ();
	return WaitForActButtonUntil (newButton, timeOut, resetInput);
}

// Returns TRUE if the wait succeeded (found no input)
//    FALSE if timed out or game aborted
BOOLEAN
WaitForNoInputUntil (TimeCount timeOut, BOOLEAN resetInput)
{
	BOOLEAN buttonPressed;

	buttonPressed = AnyButtonPress (TRUE);
	while (buttonPressed
			&& (timeOut == WAIT_INFINITE || GetTimeCounter () < timeOut)
			&& !(GLOBAL (CurrentActivity) & CHECK_ABORT)
			&& !QuitPosted)
	{
		SleepThread (ONE_SECOND / 40);
		buttonPressed = AnyButtonPress (TRUE);
	} 

	if (resetInput)
		FlushInput ();

	return !buttonPressed;
}

BOOLEAN
WaitForNoInput (TimePeriod duration, BOOLEAN resetInput)
{
	TimeCount timeOut = duration;
	if (duration != WAIT_INFINITE)
		timeOut += GetTimeCounter ();
	return WaitForNoInputUntil (timeOut, resetInput);
}

// Stops game clock and music thread and minimizes interrupts/cycles
//  based on value of global GameActive variable
// See similar sleep state for main thread in uqm.c:main()
void
SleepGame (void)
{
	if (QuitPosted)
		return; // Do not sleep the game when already asked to quit

	log_add (log_Debug, "Game is going to sleep");

	if (PlayingTrack ())
		PauseTrack ();
	PauseMusic ();


	while (!GameActive && !QuitPosted)
		SleepThread (ONE_SECOND / 2);

	log_add (log_Debug, "Game is waking up");

	WaitForNoInput (ONE_SECOND / 10, TRUE);

	ResumeMusic ();

	if (PlayingTrack ())
		ResumeTrack ();


	TaskSwitch ();
}

/* Returns the fuel requirement to get to Sol (in fuel units * 100)
 */
DWORD
get_fuel_to_sol (void)
{
	POINT pt;
	DWORD f;

	pt.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
	pt.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	
	pt.x -= SOL_X;
	pt.y -= SOL_Y;

	f = (DWORD)((long)pt.x * pt.x + (long)pt.y * pt.y);
	if (f == 0 || GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1)
		return 0;
	else
		return (square_root (f) + (FUEL_TANK_SCALE / 20));
}

void
DrawFlagStatDisplay (UNICODE *str)
{
	TEXT t;
	RECT r;

	r.corner.x = RES_SCALE (2);
	r.corner.y = RES_SCALE (20);
	r.extent.width = FIELD_WIDTH + RES_SCALE (1);
	r.extent.height = (RES_SCALE (129) - r.corner.y);

	if (!optCustomBorder && !IS_HD)
	{
		DrawStarConBox (&r, RES_SCALE (1),
			SHADOWBOX_MEDIUM_COLOR, SHADOWBOX_DARK_COLOR,
			TRUE, MODULE_BACK_COLOR, FALSE, TRANSPARENT);
	}
	else
		DrawBorder (13);

	// print the "str" title
	SetContextFont (StarConFont);
	t.baseline.x = (STATUS_WIDTH >> 1) - RES_SCALE (1);
	t.baseline.y = r.corner.y + RES_SCALE (7);
	t.align = ALIGN_CENTER;
	t.pStr = str;
	t.CharCount = (COUNT)~0;
	SetContextForeGroundColor (MODULE_SELECTED_COLOR);
	font_DrawText (&t);
}

UNICODE *
WholeFuelValue (void)
{
	static UNICODE buf[7];
	DWORD CoarseFuel = GLOBAL_SIS (FuelOnBoard);

	double dblFuelOnBoard = (double)CoarseFuel / FUEL_TANK_SCALE;

	if (!optInfiniteFuel)
	{
		if (!optWholeFuel)
			snprintf (buf, sizeof buf, "%u", CoarseFuel);
		else if (dblFuelOnBoard > 999.99)
			snprintf (buf, sizeof buf, "%.1f", dblFuelOnBoard);
		else
			snprintf (buf, sizeof buf, "%.2f", dblFuelOnBoard);
	}
	else
		snprintf (buf, sizeof buf, "%s", STR_INFINITY_SIGN);

	return buf;
}

void
DrawStarmapHelper (void)
{

	CONTEXT OldContext;
	STAMP s;
	TEXT t;
	RECT r;
	SIZE leading;
	int frame_index;
#define GAMEPAD(a) (RES_SCALE (optControllerType ? (a) : 0))

	OldContext = SetContext (StatusContext);

	BatchGraphics ();

	DrawFlagStatDisplay (GAME_STRING (STATUS_STRING_BASE + 7));

	SetContextFont (TinyFont);
	GetContextFontLeading (&leading);

	r.corner.x = RES_SCALE (4);
	r.corner.y = RES_SCALE (34);

	frame_index = !optControllerType ? 0 : 5 * optControllerType;

	// :Maps
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index);
	s.origin = r.corner;
	DrawStamp (&s);

	SetContextForeGroundColor (MODULE_NAME_COLOR);
	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE (12) - GAMEPAD (2);
	t.baseline.y = s.origin.y + leading;
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 12);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// :Add->O
	s.origin.y += RES_SCALE (10);
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index + 1);
	DrawStamp (&s);

	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE (16) - GAMEPAD (6);
	t.baseline.y = s.origin.y + leading;
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 13);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	s.origin.x = t.baseline.x + RES_SCALE (28);
	s.origin.y += RES_SCALE (4);
	s.frame = SetAbsFrameIndex (MiscDataFrame, 109);
	DrawStamp (&s);

	// Cursor Speed
	s.origin.x = r.corner.x;
	s.origin.y += RES_SCALE (6);
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index + 2);
	DrawStamp (&s);

	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE (18) - GAMEPAD (8);
	t.baseline.y = s.origin.y + leading;
	t.pStr = ":";
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	s.origin.x = t.baseline.x + RES_SCALE (7);
	s.origin.y += RES_SCALE (4);
	s.frame = SetAbsFrameIndex (MiscDataFrame, 48);
	DrawStamp (&s);

	t.align = ALIGN_LEFT;
	t.baseline.x += RES_SCALE (13);
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 14);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// :Search
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index + 3);
	s.origin.x = r.corner.x;
	s.origin.y += RES_SCALE (7) - GAMEPAD (1);
	DrawStamp (&s);

	t.baseline.x = s.origin.x + RES_SCALE (12) + GAMEPAD (11);
	t.baseline.y = s.origin.y + leading + RES_SCALE (5) - GAMEPAD (5);
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 15);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// :Zoom
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index + 4);
	s.origin.y += RES_SCALE (21) - GAMEPAD (11);
	DrawStamp (&s);

	t.baseline.x = s.origin.x + RES_SCALE (30) - GAMEPAD (18);
	t.baseline.y = s.origin.y + leading + RES_SCALE (5);
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 16);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// Separator
	r.corner.y = RES_SCALE (118);
	r.extent.width = FIELD_WIDTH - RES_SCALE (3);
	r.extent.height = RES_SCALE (1);
	SetContextForeGroundColor (CARGO_SELECTED_BACK_COLOR);
	DrawFilledRectangle (&r);

	// FUEL:
	SetContextForeGroundColor (MODULE_NAME_COLOR);
	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE (2);
	t.baseline.y = r.corner.y + leading + RES_SCALE (1);
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 8); // FUEL:
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// Current amount of fuel
	SetContextForeGroundColor (MODULE_PRICE_COLOR);
	t.align = ALIGN_RIGHT;
	t.baseline.x = r.extent.width + RES_SCALE (2);
	t.baseline.y = r.corner.y + leading + RES_SCALE (1);
	t.pStr = WholeFuelValue ();
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	UnbatchGraphics ();

	SetContext (OldContext);
}
