//   ___________		     _________		      _____  __
//   \_	  _____/______	 ____	____ \_	  ___ \____________ _/ ____\/  |_
//    |	   __) \_  __ \_/ __ \_/ __ \/	  \  \/\_  __ \__  \\	__\\   __\ 
//    |	    \	|  | \/\  ___/\	 ___/\	   \____|  | \// __ \|	|   |  |
//    \___  /	|__|	\___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________			     ______________________
//			  T H E	  W A R	  B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name X11.c		-	XWindows support. */
//
//	(c) Copyright 1998-2002 by Lutz Sammer and Valery Shchedrin
//
//	FreeCraft is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	FreeCraft is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

//@{

#ifdef USE_X11

// FIXME: move this and clean up to new_X11.
// FIXME: move this and clean up to new_X11.
// FIXME: move this and clean up to new_X11.
// FIXME: move this and clean up to new_X11.
// FIXME: move this and clean up to new_X11.
// FIXME: move this and clean up to new_X11.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <errno.h>

#include "freecraft.h"
#include "video.h"
#include "tileset.h"
#include "sound_id.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "map.h"
#include "minimap.h"
#include "font.h"
#include "sound_server.h"
#include "missile.h"
#include "sound.h"
#include "cursor.h"
#include "interface.h"
#include "network.h"
#include "ui.h"

local Display* TheDisplay;		/// My X11 display
local int TheScreen;			/// My X11 screen
local Window TheMainWindow;		/// My X11 window
local Pixmap TheMainDrawable;		/// My X11 drawlable
local GC GcLine;			/// My drawing context

local Atom WmDeleteWindowAtom;		/// Atom for WM_DELETE_WINDOW

local struct timeval X11TicksStart;	/// My counter start

/*----------------------------------------------------------------------------
--	Sync
----------------------------------------------------------------------------*/

#define noUSE_ITIMER			/// Use the old ITIMER code (obsolete)

#ifdef USE_ITIMER

/*
**	The timer resolution is 10ms, which make the timer useless for us.
*/

/**
**	Called from SIGALRM.
*/
local void VideoSyncHandler(int unused __attribute__((unused)))
{
    DebugLevel3("Interrupt\n");
    ++VideoInterrupts;
}

/**
**	Initialise video sync.
*/
global void SetVideoSync(void)
{
    struct sigaction sa;
    struct itimerval itv;

    if( !VideoSyncSpeed ) {
	return;
    }

    sa.sa_handler=VideoSyncHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=SA_RESTART;
    if( sigaction(SIGALRM,&sa,NULL) ) {
	fprintf(stderr,"Can't set signal handler\n");
    }

    itv.it_interval.tv_sec=itv.it_value.tv_sec=
	(100/CYCLES_PER_SECOND)/VideoSyncSpeed;
    itv.it_interval.tv_usec=itv.it_value.tv_usec=
	(100000000/CYCLES_PER_SECOND)/VideoSyncSpeed-
	itv.it_value.tv_sec*100000;
    if( setitimer(ITIMER_REAL,&itv,NULL) ) {
	fprintf(stderr,"Can't set itimer\n");
    }

    DebugLevel3("Timer installed %ld,%ld\n" _C_
	itv.it_interval.tv_sec _C_ itv.it_interval.tv_usec);
}

#else

local int FrameTicks;			/// Frame length in ms
local int FrameRemainder;		/// Frame remainder 0.1 ms
local int FrameFraction;		/// Frame fractional term
local int SkipFrames;			/// Skip this frames

/**
**	Initialise video sync.
**	Calculate the length of video frame and any simulation skips.
**
**	@see VideoSyncSpeed @see SkipFrames @see FrameTicks @see FrameRemainder
*/
global void SetVideoSync(void)
{
    int ms;

    if( VideoSyncSpeed ) {
	ms = (1000 * 1000 / CYCLES_PER_SECOND) / VideoSyncSpeed;
    } else {
	ms = INT_MAX;
    }
    SkipFrames = ms / 400;
    while (SkipFrames && ms / SkipFrames < 200) {
	--SkipFrames;
    }
    ms /= SkipFrames + 1;

    FrameTicks = ms / 10;
    FrameRemainder = ms % 10;
    DebugLevel0Fn("frames %d - %d.%dms\n" _C_ SkipFrames _C_ ms / 10 _C_ ms % 10);
}

#endif

#if 0
/**
**	Watch opening/closing of X11 connections. (not needed)
*/
local void MyConnectionWatch
	(Display* display,XPointer client,int fd,Bool flag,XPointer* data)
{
    DebugLevel0Fn(": fildes %d flag %d\n" _C_ fd _C_ flag);
    if( flag ) {			// file handle opened
    } else {				// file handle closed
    }
}
#endif

/**
**	X11 get ticks in ms.
**
**	@return		Game ticks in ms.
**
**	@todo	Use if supported RDTSC.
*/
global unsigned long X11GetTicks(void)
{
    struct timeval now;
    unsigned long ticks;

    gettimeofday(&now,NULL);

    ticks=(now.tv_sec-X11TicksStart.tv_sec)*1000
	    +(now.tv_usec-X11TicksStart.tv_usec)/1000;

    return ticks;
}

/**
**	Converts a hardware independend 256 color palette to a hardware
**	dependent palette. Letting a system color as index in the result,
**	correspond with the right RGB value.
**
**	@param palette	Hardware independent 256 color palette.
**	@param syspalette	Hardware dependent 256 color palette, to be
**				filled.
**	@param syspalette_defined	Array denoting which entries in above
**			palette are defined by this function.
*/
local void AllocPalette8(Palette * palette, Palette * syspalette,
    unsigned long syspalette_defined[8])
{
    XWindowAttributes xwa;
    XColor color;
    int average;
    int i;
    int warning_given;

    XGetWindowAttributes(TheDisplay, TheMainWindow, &xwa);
    average = 0;
    color.pad = 0;
    color.flags = DoRed | DoGreen | DoBlue;
    warning_given = 0;

    for (i = 0; i <= 7; i++) {
	syspalette_defined[i] = 0;
    }

    for (i = 0; i <= 255; i++, palette++) {
	unsigned int r, g, b;

	// -> Video
	color.red = (r = palette->r) << 8;
	color.green = (g = palette->g) << 8;
	color.blue = (b = palette->b) << 8;
	if (XAllocColor(TheDisplay, xwa.colormap, &color)) {
	    unsigned long bit;
	    int j;

	    if (color.pixel > 255) {	// DEBUG: should not happen?
		fprintf(stderr, "System 8bit color above unsupported 255\n");
		ExitFatal(-1);
	    }
	    // Fill palette, to get from system to RGB
	    j = color.pixel >> 5;
	    bit = 1 << (color.pixel & 0x1F);
	    if (syspalette_defined[j] & bit) {
		// multiple RGB matches for one sytem color, average RGB values
		// Note: happens when a palette with duplicate RGB values is
		//	 used, but might also happen for another reason?
		r += syspalette[color.pixel].r + 1;
		r >>= 1;
		g += syspalette[color.pixel].g + 1;
		g >>= 1;
		b += syspalette[color.pixel].b + 1;
		b >>= 1;
		average++;
	    } else {
		syspalette_defined[j] |= bit;
	    }

	    syspalette[color.pixel].r = r;
	    syspalette[color.pixel].g = g;
	    syspalette[color.pixel].b = b;
	} else if (!warning_given) {	// Note: this may also happen when
					// more then 256 colors are tried..
	    //	    Use VideoFreePalette to unallocate..
	    warning_given = 1;
	    fprintf(stderr,
		"Cannot allocate 8pp color\n"
		"Probably another application has taken some colors..\n");
	}
    }

// Denote missing colors
    if (average) {
	fprintf(stderr, "Only %d unique colors available\n", 256 - average);
    }
}

/**
**	X11 initialize.
*/
global void GameInitDisplay(void)
{
    int i;
    Window window;
    XGCValues gcvalue;
    XSizeHints hints;
    XWMHints wmhints;
    XClassHint classhint;
    XSetWindowAttributes attributes;
    int shm_major,shm_minor;
    Bool pixmap_support;
    XShmSegmentInfo shminfo;
    XVisualInfo xvi;
    XPixmapFormatValues *xpfv;

    if( !(TheDisplay=XOpenDisplay(NULL)) ) {
	fprintf(stderr,"Cannot connect to X-Server.\n");
	ExitFatal(-1);
    }

    gettimeofday(&X11TicksStart,NULL);

    TheScreen=DefaultScreen(TheDisplay);

    //	I need shared memory pixmap extension.

    if( !XShmQueryVersion(TheDisplay,&shm_major,&shm_minor,&pixmap_support) ) {
	fprintf(stderr,"SHM-Extensions required.\n");
	ExitFatal(-1);
    }
    if( !pixmap_support ) {
	fprintf(stderr,"SHM-Extensions with pixmap supported required.\n");
	ExitFatal(-1);
    }

    //	Look for a nice visual

    if( VideoDepth && XMatchVisualInfo(TheDisplay,
	    TheScreen,VideoDepth,TrueColor,&xvi) ) {
	goto foundvisual;
    }
    if(XMatchVisualInfo(TheDisplay, TheScreen, 16, TrueColor, &xvi)) {
	goto foundvisual;
    }
    if(XMatchVisualInfo(TheDisplay, TheScreen, 15, TrueColor, &xvi)) {
	goto foundvisual;
    }
    if(XMatchVisualInfo(TheDisplay, TheScreen, 24, TrueColor, &xvi)) {
	goto foundvisual;
    }
    if(XMatchVisualInfo(TheDisplay, TheScreen, 24, TrueColor, &xvi)) {
	goto foundvisual;
    }
    if(XMatchVisualInfo(TheDisplay, TheScreen, 8, PseudoColor, &xvi)) {
	goto foundvisual;
    }
    if(XMatchVisualInfo(TheDisplay, TheScreen, 8, TrueColor, &xvi)) {
	goto foundvisual;
    }
    fprintf(stderr,"Sorry, I couldn't find an 8, 15, 16, 24 or 32 bit visual.\n");
    ExitFatal(-1);

foundvisual:

    xpfv=XListPixmapFormats(TheDisplay, &i);
    for( i--; i>=0; i-- )  {
	DebugLevel3("pixmap %d\n" _C_ xpfv[i].depth);
	if( xpfv[i].depth==xvi.depth ) {
	    break;
	}
    }
    if(i<0)  {
	fprintf(stderr,"No Pixmap format for visual depth?\n");
	ExitFatal(-1);
    }
    if( !VideoDepth ) {
	VideoDepth=xvi.depth;
    }
    VideoBpp=xpfv[i].bits_per_pixel;
    printf( "Video X11, %d color, %d bpp\n", VideoDepth, VideoBpp );

    if( !VideoWidth ) {
	VideoWidth = DEFAULT_VIDEO_WIDTH;
	VideoHeight = DEFAULT_VIDEO_HEIGHT;
    }

    shminfo.shmid=shmget(IPC_PRIVATE,
	    (VideoWidth*xpfv[i].bits_per_pixel+xpfv[i].scanline_pad-1) /
	    xpfv[i].scanline_pad * xpfv[i].scanline_pad * VideoHeight / 8,
	    IPC_CREAT|0777);

    XFree(xpfv);

    if( !shminfo.shmid==-1 ) {
	fprintf(stderr,"shmget failed.\n");
	ExitFatal(-1);
    }
    VideoMemory=(void*)shminfo.shmaddr=shmat(shminfo.shmid,0,0);
    if( shminfo.shmaddr==(void*)-1 ) {
	shmctl(shminfo.shmid,IPC_RMID,0);
	fprintf(stderr,"shmat failed.\n");
	ExitFatal(-1);
    }
    shminfo.readOnly=False;

    if( !XShmAttach(TheDisplay,&shminfo) ) {
	shmctl(shminfo.shmid,IPC_RMID,0);
	fprintf(stderr,"XShmAttach failed.\n");
	ExitFatal(-1);
    }
    // Mark segment as deleted as soon as both us and the X server have
    // attached to it.	The POSIX spec says that a segment marked as deleted
    // can no longer have addition processes attach to it, but Linux will let
    // them anyway.
#if defined(linux)
    shmctl(shminfo.shmid,IPC_RMID,0);
#endif /* linux */

    TheMainDrawable=attributes.background_pixmap=
	    XShmCreatePixmap(TheDisplay,DefaultRootWindow(TheDisplay)
		,shminfo.shmaddr,&shminfo
		,VideoWidth,VideoHeight
		,xvi.depth);
    attributes.cursor = XCreateFontCursor(TheDisplay,XC_tcross-1);
    attributes.backing_store = NotUseful;
    attributes.save_under = False;
    attributes.event_mask = KeyPressMask|KeyReleaseMask|/*ExposureMask|*/
	FocusChangeMask|ButtonPressMask|PointerMotionMask|ButtonReleaseMask;
    i = CWBackPixmap|CWBackingStore|CWSaveUnder|CWEventMask|CWCursor;

    if(xvi.class==PseudoColor)	{
	i|=CWColormap;
	attributes.colormap =
		XCreateColormap( TheDisplay, DefaultRootWindow(TheDisplay),
		    xvi.visual, AllocNone);
	// FIXME:  Really should fill in the colormap right now
    }
    window=XCreateWindow(TheDisplay,DefaultRootWindow(TheDisplay)
	    ,0,0,VideoWidth,VideoHeight,3
	    ,xvi.depth,InputOutput,xvi.visual,i,&attributes);
    TheMainWindow=window;

    gcvalue.graphics_exposures=False;
    GcLine=XCreateGC(TheDisplay,window,GCGraphicsExposures,&gcvalue);

    //
    //	Clear initial window.
    //
    XSetForeground(TheDisplay,GcLine,BlackPixel(TheDisplay,TheScreen));
    XFillRectangle(TheDisplay,TheMainDrawable,GcLine,0,0
	    ,VideoWidth,VideoHeight);

    WmDeleteWindowAtom=XInternAtom(TheDisplay,"WM_DELETE_WINDOW",False);

    //
    //	Set some usefull min/max sizes as well as a 1.3 aspect
    //
#if 0
    if( geometry ) {
	hints.flags=0;
	f=XParseGeometry(geometry
		,&hints.x,&hints.y,&hints.width,&hints.height);

	if( f&XValue ) {
	    if( f&XNegative ) {
		hints.x+=DisplayWidth-hints.width;
	    }
	    hints.flags|=USPosition;
	    // FIXME: win gravity
	}
	if( f&YValue ) {
	    if( f&YNegative ) {
		hints.y+=DisplayHeight-hints.height;
	    }
	    hints.flags|=USPosition;
	    // FIXME: win gravity
	}
	if( f&WidthValue ) {
	    hints.flags|=USSize;
	}
	if( f&HeightValue ) {
	    hints.flags|=USSize;
	}
    } else {
#endif
	hints.width=VideoWidth;
	hints.height=VideoHeight;
	hints.flags=PSize;
#if 0
    }
#endif
    hints.min_width=VideoWidth;
    hints.min_height=VideoHeight;
    hints.max_width=VideoWidth;
    hints.max_height=VideoHeight;
    hints.min_aspect.x=4;
    hints.min_aspect.y=3;

    hints.max_aspect.x=4;
    hints.max_aspect.y=3;
    hints.width_inc=4;
    hints.height_inc=3;

    hints.flags|=PMinSize|PMaxSize|PAspect|PResizeInc;

    wmhints.input=True;
    wmhints.initial_state=NormalState;
    wmhints.window_group=window;
    wmhints.flags=InputHint|StateHint|WindowGroupHint;

    classhint.res_name="freecraft";
    classhint.res_class="FreeCraft";

    XSetStandardProperties(TheDisplay,window
	,"FreeCraft (formerly known as ALE Clone)"
	,"FreeCraft",None,(char**)0,0,&hints);
    XSetClassHint(TheDisplay,window,&classhint);
    XSetWMHints(TheDisplay,window,&wmhints);

    XSetWMProtocols(TheDisplay,window,&WmDeleteWindowAtom,1);

    XMapWindow(TheDisplay,window);

    //
    //	Input handling.
    //
    //XAddConnectionWatch(TheDisplay,MyConnectionWatch,NULL);

    XFlush(TheDisplay);

    //
    //	Let hardware independent palette be converted.
    //
    VideoAllocPalette8=AllocPalette8;
}

/**
**	Change video mode to new width.
*/
global int SetVideoMode(int width)
{
    if (width == 640) return 1;
    return 0;
}

/**
**	Invalidate some area
**
**	@param x	screen pixel X position.
**	@param y	screen pixel Y position.
**	@param w	width of rectangle in pixels.
**	@param h	height of rectangle in pixels.
*/
global void InvalidateArea(int x,int y,int w,int h)
{
    // FIXME: This checks should be done at hight level
    if( x<0 ) {
	w+=x;
	x=0;
    }
    if( y<0 ) {
	h+=y;
	y=0;
    }
    if( !w<=0 && !h<=0 ) {
	DebugLevel3("X %d,%d -> %d,%d\n" _C_ x _C_ y _C_ w _C_ h);
	XClearArea(TheDisplay,TheMainWindow,x,y,w,h,False);
    }
}

/**
**	Invalidate whole window
*/
global void Invalidate(void)
{
    XClearWindow(TheDisplay,TheMainWindow);
}

/**
**	Handle keyboard modifiers
*/
local void X11HandleModifiers(XKeyEvent* keyevent)
{
    int mod=keyevent->state;

    // Here we use an ideous hack to avoid X keysyms mapping.
    // What we need is to know that the player hit key 'x' with
    // the control modifier; we don't care if he typed `key mapped
    // on Ctrl-x` with control modifier...
    // Note that we don't use this hack for "shift", because shifted
    // keys can be useful (to get numbers on my french keybord
    // for exemple :)).
    if( mod&ShiftMask ) {
	    /* Do Nothing */;
    }
    if( mod&ControlMask ) {
	keyevent->state&=~ControlMask;	// Hack Attack!
    }
    if( mod&Mod1Mask ) {
	keyevent->state&=~Mod1Mask;	// Hack Attack!
    }
}

/**
**	Convert X11 keysym into internal keycode.
**
**	@param code	X11 keysym structure pointer.
**
**	@return		ASCII code or internal keycode.
*/
local unsigned X112InternalKeycode(const KeySym code)
{
    int icode;

    /*
    **	Convert X11 keycodes into internal keycodes.
    */
    // FIXME: Combine X11 keysym mapping to internal in up and down.
    switch( (icode=code) ) {
	case XK_Escape:
	    icode='\e';
	    break;
	case XK_Return:
	    icode='\r';
	    break;
	case XK_BackSpace:
	    icode='\b';
	    break;
	case XK_Tab:
	    icode='\t';
	    break;
	case XK_Up:
	    icode=KeyCodeUp;
	    break;
	case XK_Down:
	    icode=KeyCodeDown;
	    break;
	case XK_Left:
	    icode=KeyCodeLeft;
	    break;
	case XK_Right:
	    icode=KeyCodeRight;
	    break;
	case XK_Pause:
	    icode=KeyCodePause;
	    break;
	case XK_F1:
	    icode=KeyCodeF1;
	    break;
	case XK_F2:
	    icode=KeyCodeF2;
	    break;
	case XK_F3:
	    icode=KeyCodeF3;
	    break;
	case XK_F4:
	    icode=KeyCodeF4;
	    break;
	case XK_F5:
	    icode=KeyCodeF5;
	    break;
	case XK_F6:
	    icode=KeyCodeF6;
	    break;
	case XK_F7:
	    icode=KeyCodeF7;
	    break;
	case XK_F8:
	    icode=KeyCodeF8;
	    break;
	case XK_F9:
	    icode=KeyCodeF9;
	    break;
	case XK_F10:
	    icode=KeyCodeF10;
	    break;
	case XK_F11:
	    icode=KeyCodeF11;
	    break;
	case XK_F12:
	    icode=KeyCodeF12;
	    break;

	case XK_KP_0:
	    icode=KeyCodeKP0;
	    break;
	case XK_KP_1:
	    icode=KeyCodeKP1;
	    break;
	case XK_KP_2:
	    icode=KeyCodeKP2;
	    break;
	case XK_KP_3:
	    icode=KeyCodeKP3;
	    break;
	case XK_KP_4:
	    icode=KeyCodeKP4;
	    break;
	case XK_KP_5:
	    icode=KeyCodeKP5;
	    break;
	case XK_KP_6:
	    icode=KeyCodeKP6;
	    break;
	case XK_KP_7:
	    icode=KeyCodeKP7;
	    break;
	case XK_KP_8:
	    icode=KeyCodeKP8;
	    break;
	case XK_KP_9:
	    icode=KeyCodeKP9;
	    break;
	case XK_KP_Add:
	    icode=KeyCodeKPPlus;
	    break;
	case XK_KP_Subtract:
	    icode=KeyCodeKPMinus;
	    break;
	case XK_Print:
	    icode=KeyCodePrint;
	    break;
	case XK_Delete:
	    icode=KeyCodeDelete;
	    break;

	case XK_dead_circumflex:
	    icode='^';
	    break;

	// We need these because if you only hit a modifier key,
	// X doesn't set its state (modifiers) field in the keyevent.
	case XK_Shift_L:
	case XK_Shift_R:
	    icode = KeyCodeShift;
	    break;
	case XK_Control_L:
	case XK_Control_R:
	    icode = KeyCodeControl;
	    break;
	case XK_Alt_L:
	case XK_Alt_R:
	case XK_Meta_L:
	case XK_Meta_R:
	    icode = KeyCodeAlt;
	    break;
	case XK_Super_L:
	case XK_Super_R:
	    icode = KeyCodeSuper;
	    break;
	case XK_Hyper_L:
	case XK_Hyper_R:
	    icode = KeyCodeHyper;
	    break;
	default:
	    break;
    }

    return icode;
}

/**
**	Handle keyboard! (pressed)
**
**	@param callbacks	Call backs that handle the events.
**	@param keycode		X11 key symbol.
**	@param keychar		Keyboard character
*/
local void X11HandleKeyPress(const EventCallback* callbacks,KeySym keycode,
	unsigned keychar)
{
    int icode;

    icode=X112InternalKeycode(keycode);
    InputKeyButtonPress(callbacks, X11GetTicks(), icode, keychar);
}

/**
**	Handle keyboard! (release)
**
**	@param callbacks	Call backs that handle the events.
**	@param keycode		X11 key symbol.
**	@param keychar		Keyboard character
*/
local void X11HandleKeyRelease(const EventCallback* callbacks,KeySym keycode,
	unsigned keychar)
{
    int icode;

    icode=X112InternalKeycode(keycode);
    InputKeyButtonRelease(callbacks, X11GetTicks(), icode, keychar);
}

/**
**	Handle interactive input event.
*/
local void X11DoEvent(const EventCallback* callbacks)
{
    XEvent event;
    int xw, yw;

    XNextEvent(TheDisplay,&event);

    switch( event.type ) {
	case ButtonPress:
	    DebugLevel3("\tbutton press %d\n" _C_ event.xbutton.button);
	    InputMouseButtonPress(callbacks, X11GetTicks(),
		    event.xbutton.button);
	    break;

	case ButtonRelease:
	    DebugLevel3("\tbutton release %d\n" _C_ event.xbutton.button);
	    InputMouseButtonRelease(callbacks, X11GetTicks(),
		    event.xbutton.button);
	    break;

	case Expose:
	    DebugLevel1("\texpose\n");
	    MustRedraw=-1;
	    break;

	case MotionNotify:
	    DebugLevel3("\tmotion notify %d,%d\n"
		 _C_ event.xbutton.x _C_ event.xbutton.y);
	    InputMouseMove(callbacks, X11GetTicks(),
		    event.xbutton.x,event.xbutton.y);
	    if ( (TheUI.WarpX != -1 || TheUI.WarpY != -1)
		    && (event.xbutton.x!=TheUI.WarpX
			 || event.xbutton.y!=TheUI.WarpY)
		    ) {
		xw = TheUI.WarpX;
		yw = TheUI.WarpY;
		TheUI.WarpX = -1;
		TheUI.WarpY = -1;

		XWarpPointer(TheDisplay,TheMainWindow,TheMainWindow,0,0
			,0,0,xw,yw);
	    }
	    MustRedraw|=RedrawCursor;
	    break;

	case FocusIn:
	    DebugLevel3("\tfocus in\n");
	    break;

	case FocusOut:
	    DebugLevel3("\tfocus out\n");
	    InputMouseExit(callbacks, X11GetTicks());
	    break;

	case ClientMessage:
	    DebugLevel3("\tclient message\n");
	    if (event.xclient.format == 32) {
		if ((Atom)event.xclient.data.l[0] == WmDeleteWindowAtom) {
		    Exit(0);
		}
	    }
	    break;

	case KeyPress:
	    DebugLevel3("\tKey press\n");
	{
	    char buf[128];
	    int num;
	    KeySym keysym;
	    KeySym key;

	    X11HandleModifiers((XKeyEvent*)&event);
	    num=XLookupString((XKeyEvent*)&event,buf,sizeof(buf),&keysym,0);
	    key=XLookupKeysym((XKeyEvent*)&event,0);
	    DebugLevel3("\tKeyv %lx %lx `%*.*s'\n" _C_ key _C_ keysym _C_ num _C_ num _C_ buf);
	    if( num==1 ) {
		X11HandleKeyPress(callbacks,key,*buf);
	    } else {
		X11HandleKeyPress(callbacks,key,keysym);
	    }
	}
	    break;

	case KeyRelease:
	    DebugLevel3("\tKey release\n");
	{
	    char buf[128];
	    int num;
	    KeySym keysym;
	    KeySym key;

	    X11HandleModifiers((XKeyEvent*)&event);
	    num=XLookupString((XKeyEvent*)&event,buf,sizeof(buf),&keysym,0);
	    key=XLookupKeysym((XKeyEvent*)&event,0);
	    DebugLevel3("\tKey^ %lx %lx `%*.*s'\n" _C_ key _C_ keysym _C_ num _C_ num _C_ buf);
	    if( num==1 ) {
		X11HandleKeyRelease(callbacks,key,*buf);
	    } else {
		X11HandleKeyRelease(callbacks,key,keysym);
	    }
	}
	    break;

	case ConfigureNotify:	// IGNORE, not useful for us yet -
				// we may be able to limit hidden map draw here later
	    break;

	default:
	    DebugLevel0("\tX11: Unknown event type: %d\n" _C_  event.type);
	    break;

    }
}

/**
**	Wait for interactive input event for one frame.
**
**	Handles system events, joystick, keyboard, mouse.
**	Handles the network messages.
**	Handles the sound queue.
**
**	All events available are fetched. Sound and network only if available.
**	Returns if the time for one frame is over.
**
**	@param callbacks	Call backs that handle the events.
**
**	@todo FIXME:	the initialition could be moved out of the loop
*/
global void WaitEventsOneFrame(const EventCallback* callbacks)
{
    struct timeval tv;
    fd_set rfds;
    fd_set wfds;
    int maxfd;
    int* xfd;
    int n;
    int i;
    int morex;
    int connection;
    unsigned long ticks;

    connection=ConnectionNumber(TheDisplay);
#ifdef WITH_SOUND
    // FIXME: ugly hack, move into sound part!!!
    if( SoundFildes==-1 ) {
	SoundOff=1;
    }
#endif
    if( !++FrameCounter ) {
	// FIXME: tests with frame counter now fails :(
	// FIXME: Should happen in 68 years :)
	fprintf(stderr,"FIXME: *** round robin ***\n");
	fprintf(stderr,"FIXME: *** round robin ***\n");
	fprintf(stderr,"FIXME: *** round robin ***\n");
	fprintf(stderr,"FIXME: *** round robin ***\n");
    }

    ticks=X11GetTicks();
#ifndef USE_ITIMER
    if( ticks>NextFrameTicks ) {	// We are too slow :(
	IfDebug(
	    if (InterfaceState == IfaceStateNormal) {
		VideoDrawText(TheUI.MapArea.X+10,TheUI.MapArea.Y+10,GameFont,
		    "SLOW FRAME!!");
		XClearArea(TheDisplay,TheMainWindow
		    ,TheUI.MapArea.X+10,TheUI.MapArea.Y+10,13*13,13
		    ,False);
	    }
	);
	++SlowFrameCounter;
    }
#endif

    InputMouseTimeout(callbacks,ticks);
    InputKeyTimeout(callbacks,ticks);
    CursorAnimate(ticks);

    for( ;; ) {
#ifdef SLOW_INPUT
	while( XPending(TheDisplay) ) {
	   X11DoEvent(callbacks);
	}
#endif
	//
	//	Prepare select
	//
	maxfd=0;
	tv.tv_sec=tv.tv_usec=0;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

#ifndef USE_ITIMER
	//
	//	Time of frame over? This makes the CPU happy. :(
	//
	ticks=X11GetTicks();
	if( !VideoInterrupts && ticks+11<NextFrameTicks ) {
	    tv.tv_usec=(NextFrameTicks-ticks)*1000;
	}
	while( ticks>=NextFrameTicks ) {
	    ++VideoInterrupts;
	    FrameFraction+=FrameRemainder;
	    if( FrameFraction>10 ) {
		FrameFraction-=10;
		++NextFrameTicks;
	    }
	    NextFrameTicks+=FrameTicks;
	}
#endif
	//
	//	X11 how many events already in queue
	//
	xfd=NULL;
	morex=QLength(TheDisplay);
	if( !morex ) {
	    //
	    //	X11 connections number
	    //
	    maxfd=connection;
	    FD_SET(connection,&rfds);

	    //
	    //	Get all X11 internal connections
	    //
	    if( !XInternalConnectionNumbers(TheDisplay,&xfd,&n) ) {
		DebugLevel0Fn(": out of memory\n");
		abort();
	    }
	    for( i=n; i--; ) {
		FD_SET(xfd[i],&rfds);
		if( xfd[i]>maxfd ) {
		    maxfd=xfd[i];
		}
	    }
	}

	//
	//	Sound
	//
	if( !SoundOff && !SoundThreadRunning ) {
	    if( SoundFildes>maxfd ) {
		maxfd=SoundFildes;
	    }
	    FD_SET(SoundFildes,&wfds);
	}

	//
	//	Network
	//
	if( NetworkFildes!=-1 ) {
	    if( NetworkFildes>maxfd ) {
		maxfd=NetworkFildes;
	    }
	    FD_SET(NetworkFildes,&rfds);
	}

#ifdef USE_ITIMER
	maxfd=select(maxfd+1,&rfds,&wfds,NULL
		,(morex|VideoInterrupts) ? &tv : NULL);
#else
	maxfd=select(maxfd+1,&rfds,&wfds,NULL,&tv);
#endif

	DebugLevel3Fn("%d, %d\n" _C_ morex|VideoInterrupts _C_ maxfd);

	//
	//	X11
	//
	if( maxfd>0 ) {
	    if( !morex ) {		// look if new events
		if (xfd) {
		    for( i=n; i--; ) {
			if( FD_ISSET(xfd[i],&rfds) ) {
			    XProcessInternalConnection(TheDisplay,xfd[i]);
			}
		    }
		}
		if( FD_ISSET(connection,&rfds) ) {
		    morex=XEventsQueued(TheDisplay,QueuedAfterReading);
		} else {
		    morex=QLength(TheDisplay);
		}
	    }
	}
	if( xfd) {
	    XFree(xfd);
	}

	for( i=morex; i > 0 && i--; ) {		// handle new + *OLD* x11 events
	   X11DoEvent(callbacks);
	}

	if( maxfd>0 ) {
	    //
	    //	Sound
	    //
	    if( !SoundOff && !SoundThreadRunning
		    && FD_ISSET(SoundFildes,&wfds) ) {
		callbacks->SoundReady();
	    }

	    //
	    //	Network
	    //
	    if( NetworkFildes!=-1 && FD_ISSET(NetworkFildes,&rfds) ) {
		callbacks->NetworkEvent();
	    }

	}

	//
	//	Not more input and time for frame over: return
	//
	if( !morex && maxfd<=0 && VideoInterrupts ) {
	    break;
	}
    }

    //
    //	Prepare return, time for one frame is over.
    //
    VideoInterrupts=0;

#ifndef USE_ITIMER
    if( !SkipGameCycle-- ) {
	SkipGameCycle=SkipFrames;
    }
#endif
}

#if 0
/**
**	Free a hardware dependend palette.
**
**	@param palette	Hardware dependend palette.
**
**	@todo FIXME: XFreeColors planes can be used to free an entire range
**		of colors; couldn't get it working though..
**		Function not used.
*/
local void VideoFreePallette(void* pixels)
{
    XWindowAttributes xwa;
    int i;
    char* vp;
    unsigned long oldpal[256];

    if (!TheDisplay || !TheMainWindow) {
	return;
    }

    XGetWindowAttributes(TheDisplay, TheMainWindow, &xwa);

    if (pixels) {
	switch (VideoBpp) {
	    case 8:
		for (i = 0; i < 256; i++) {
		    oldpal[i] = ((VMemType8 *) pixels)[i];
		}
		break;
	    case 15:
	    case 16:
		for (i = 0; i < 256; i++) {
		    oldpal[i] = ((VMemType16 *) pixels)[i];
		}
		break;
	    case 24:
		for (i = 0; i < 256; i++) {
		    vp = (char *)(oldpal + i);
		    vp[0] = ((VMemType24 *) pixels)[i].a;
		    vp[1] = ((VMemType24 *) pixels)[i].b;
		    vp[2] = ((VMemType24 *) pixels)[i].c;
		}
		break;
	    case 32:
		for (i = 0; i < 256; i++) {
		    oldpal[i] = ((VMemType32 *) pixels)[i];
		}
		break;
	    default:
		DebugLevel0Fn(": Unknown depth\n");
		return;
	}
	XFreeColors(TheDisplay, xwa.colormap, oldpal, 256, 0);
    }
}
#endif

/**
**	Allocate a new hardware dependend palette palette.
**
**	@param palette	Hardware independend palette.
**
**	@return		A hardware dependend pixel table.
**
**	@todo FIXME: VideoFreePallette should be used to free unused colors
*/
global VMemType* VideoCreateNewPalette(const Palette *palette)
{
    XColor color;
    XWindowAttributes xwa;
    int i;
    void* pixels;

    if( !TheDisplay || !TheMainWindow ) {	// no init
	return NULL;
    }

    switch( VideoBpp ) {
    case 8:
	if ( colorcube8 ) {
	// Shortcut: get palette from already allocated common palette.
	// FIXME: shortcut should be placed in video.c, for all video support.
	    return (VMemType*)VideoFindNewPalette8( colorcube8, palette );
	}
	pixels=malloc(256*sizeof(VMemType8));
	break;
    case 15:
    case 16:
	pixels=malloc(256*sizeof(VMemType16));
	break;
    case 24:
	pixels=malloc(256*sizeof(VMemType24));
	break;
    case 32:
	pixels=malloc(256*sizeof(VMemType32));
	break;
    default:
	DebugLevel0Fn(": Unknown depth\n");
	return NULL;
    }

    XGetWindowAttributes(TheDisplay,TheMainWindow,&xwa);

    //
    //	Convert each palette entry into hardware format.
    //
    for( i=0; i<256; ++i ) {
	int r;
	int g;
	int b;
	int v;
	char *vp;

	r=(palette[i].r)&0xFF;
	g=(palette[i].g)&0xFF;
	b=(palette[i].b)&0xFF;
	v=r+g+b;

	// Apply global saturation,contrast and brightness
	r= ((((r*3-v)*TheUI.Saturation + v*100)
	    *TheUI.Contrast)
	    +TheUI.Brightness*25600*3)/30000;
	g= ((((g*3-v)*TheUI.Saturation + v*100)
	    *TheUI.Contrast)
	    +TheUI.Brightness*25600*3)/30000;
	b= ((((b*3-v)*TheUI.Saturation + v*100)
	    *TheUI.Contrast)
	    +TheUI.Brightness*25600*3)/30000;

	// Boundings
	r= r<0 ? 0 : r>255 ? 255 : r;
	g= g<0 ? 0 : g>255 ? 255 : g;
	b= b<0 ? 0 : b>255 ? 255 : b;

	// -> Video
	color.red=r<<8;
	color.green=g<<8;
	color.blue=b<<8;
	color.flags=DoRed|DoGreen|DoBlue;
	if( !XAllocColor(TheDisplay,xwa.colormap,&color) ) {
	    fprintf(stderr,"Cannot allocate color\n");
	    // FIXME: Must find the nearest matching color
	    //ExitFatal(-1);
	}

	switch( VideoBpp ) {
	case 8:
	    ((VMemType8*)pixels)[i]=color.pixel;
	    break;
	case 15:
	case 16:
	    ((VMemType16*)pixels)[i]=color.pixel;
	    break;
	case 24:
	    // Disliked by gcc 2.95.2, maybe due to size mismatch
	    // ((VMemType24*)pixels)[i]=color.pixel;
	    // ARI: Let's hope XAllocColor did correct RGB/BGR DAC mapping
	    // into color.pixel
	    // The following brute force hack then should be endian safe, well
	    // maybe except for vaxen..
	    // Now just tell users to stay away from strict-aliasing..
	    vp = (char *)(&color.pixel);
	    ((VMemType24*)pixels)[i].a=vp[0];
	    ((VMemType24*)pixels)[i].b=vp[1];
	    ((VMemType24*)pixels)[i].c=vp[2];
	    break;
	case 32:
	    ((VMemType32*)pixels)[i]=color.pixel;
	    break;
	}
    }

    return pixels;
}

/**
**	Check video interrupt.
**
**	Display and count too slow frames.
*/
global void CheckVideoInterrupts(void)
{
#ifdef USE_ITIMER
    if( VideoInterrupts ) {
	//DebugLevel1("Slow frame\n");
	IfDebug(
	    if (InterfaceState == IfaceStateNormal) {
		VideoDrawText(TheUI.MapX+10,TheUI.MapY+10,GameFont,"SLOW FRAME!!");
		XClearArea(TheDisplay,TheMainWindow
		    ,TheUI.MapX+10,TheUI.MapX+10,13*13,13
		    ,False);
	    }
	);
	++SlowFrameCounter;
    }
#endif
}

/**
**	Realize video memory.
*/
global void RealizeVideoMemory(void)
{
    // in X11 it does flushing the output queue
    //XFlush(TheDisplay);
    // in X11 wait for all commands done.
    XSync(TheDisplay,False);
}

/**
**	Toggle grab mouse.
**
**	@param mode	Wanted mode, 1 grab, -1 not grab, 0 toggle.
*/
global void ToggleGrabMouse(int mode)
{
    static int grabbed;

    if( mode<=0 && grabbed ) {
	XUngrabPointer(TheDisplay,CurrentTime);
	grabbed=0;
    } else if( mode>=0 && !grabbed ) {
	if( XGrabPointer(TheDisplay,TheMainWindow,True,0
		,GrabModeAsync,GrabModeAsync
		,TheMainWindow, None, CurrentTime)==GrabSuccess ) {
	    grabbed=1;
	}
    }
}

/**
**	Toggle full screen mode.
**
**	@todo FIXME: not written.
*/
global void ToggleFullScreen(void)
{
}

#endif	// USE_X11

//@}
