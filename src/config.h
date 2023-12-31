#include <X11/XF86keysym.h>
#include "movestack.c"

/* macros */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
    { MODKEY,                       KEY, view,       {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,           KEY, toggleview, {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY, tag,        {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask, KEY, toggletag,  {.ui = 1 << TAG} },

// helper for spawning shell commands
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* appearance */
static const unsigned int borderpx       = 3;  // border pixel of windows
static const unsigned int snap           = 32; // snap pixel
static const unsigned int systraypinning = 0;  // 0: sloppy systray follows selected monitor, >0: pin systray to monitor X
static const unsigned int systrayonleft  = 0;  // 0: systray in the right corner, >0: systray on left of status text
static const unsigned int systrayspacing = 2;  // systray spacing
static const int systraypinningfailfirst = 1;  // 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor
static const int showsystray             = 1;  // 0 means no systray
static const unsigned int gappih         = 8;  // horiz inner gap between windows
static const unsigned int gappiv         = 8;  // vert inner gap between windows
static const unsigned int gappoh         = 8;  // horiz outer gap between windows and screen edge
static const unsigned int gappov         = 8;  // vert outer gap between windows and screen edge
static const int smartgaps               = 0;  // 1 means no outer gap when there is only one window
static const int showbar                 = 1;  // 0 means no bar
static const int topbar                  = 1;  // 0 means bottom bar

/*
    Display modes of the tab bar: never shown, always shown, shown only in
    monocle mode in the presence of several windows.
    Modes after showtab_nmodes are disabled.
*/
enum showtab_modes { showtab_never, showtab_auto, showtab_nmodes, showtab_always };
static const int showtab = showtab_auto;  // default tab bar show mode
static const int toptab = False;          // False means bottom tab bar

static const char *fonts[]    = { "Fira Sans:size=11", "monospace:size=11" };
static const char dmenufont[] = "Fira Sans:size=11";

/*
static const char col_gray1[]  = "#222222";
static const char col_gray2[]  = "#444444";
static const char col_gray3[]  = "#bbbbbb";
static const char col_gray4[]  = "#eeeeee";
static const char col_cyan[]   = "#005577";
*/

static const char col_gray1[]  = "#151515";
static const char col_gray2[]  = "#444444";
static const char col_gray3[]  = "#bbbbbb";
static const char col_gray4[]  = "#eeeeee";
static const char col_cyan[]   = "#54457f";

static const char *colors[][3] = {
    /*               fg         bg         border   */
    [SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
    [SchemeSel]  = { col_gray4, col_cyan,  col_cyan  },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
    /* xprop(1):
     *	WM_CLASS(STRING) = instance, class
     *	WM_NAME(STRING) = title
     */
    /* class      instance    title       tags mask     isfloating   monitor */
    { "Gimp",     NULL,       NULL,       0,            1,           -1 },
};

/* layout(s) */
static const float mfact        = 0.55; // factor of master area size [0.05..0.95]
static const int nmaster        = 1;    // number of clients in master area
static const int resizehints    = 1;    // 1 means respect size hints in tiled resizals
static const int attachbelow    = 1;    // 1 means attach after the currently active window
static const int lockfullscreen = 1;    // 1 will force focus on the fullscreen window

static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[]=",      tile },    // first entry is default
    { "><>",      NULL },    // no layout function means floating behavior
    { "[M]",      monocle },
};

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
static const char *termcmd[]  = { "/home/manuelgcsousa/.cargo/bin/alacritty", NULL };

static const Key keys[] = {
    /* modifier                     key                        function         argument */
    { MODKEY,                       XK_Return,                 spawn,           {.v = termcmd } },
    { MODKEY,                       XK_b,                      togglebar,       {0} },
    { MODKEY,                       XK_w,                      tabmode,         {-1} },
    { MODKEY,                       XK_Down,                   focusstack,      {.i = +1 } },
    { MODKEY,                       XK_Up,                     focusstack,      {.i = -1 } },
    { MODKEY|ShiftMask,             XK_Left,                   setmfact,        {.f = -0.05} },
    { MODKEY|ShiftMask,             XK_Right,                  setmfact,        {.f = +0.05} },
    { MODKEY|ShiftMask,             XK_Down,                   movestack,       {.i = +1 } },
    { MODKEY|ShiftMask,             XK_Up,                     movestack,       {.i = -1 } },
    { MODKEY|ShiftMask,             XK_plus,                   incrgaps,        {.i = +1 } },
    { MODKEY|ShiftMask,             XK_minus,                  incrgaps,        {.i = -1 } },
    { MODKEY|ShiftMask,             XK_0,                      defaultgaps,     {0} },
    { MODKEY|ShiftMask,             XK_q,                      killclient,      {0} },
    { MODKEY,                       XK_t,                      setlayout,       {.v = &layouts[0]} },
    { MODKEY,                       XK_f,                      setlayout,       {.v = &layouts[1]} },
    { MODKEY,                       XK_m,                      setlayout,       {.v = &layouts[2]} },
    { MODKEY|ShiftMask,             XK_space,                  togglefloating,  {0} },
    { MODKEY,                       XK_comma,                  focusmon,        {.i = -1 } },
    { MODKEY,                       XK_period,                 focusmon,        {.i = +1 } },
    { MODKEY|ShiftMask,             XK_comma,                  tagmon,          {.i = -1 } },
    { MODKEY|ShiftMask,             XK_period,                 tagmon,          {.i = +1 } },
    { MODKEY|ShiftMask,             XK_d,                      spawn,           SHCMD("$HOME/.config/dwm/resources/scripts/displays") },
    { MODKEY|ShiftMask,             XK_e,                      spawn,           SHCMD("$HOME/.config/dwm/resources/scripts/powermenu") },
    { MODKEY|ShiftMask,             XK_p,                      spawn,           SHCMD("$HOME/.config/dwm/resources/scripts/screenshot") },
    { MODKEY,                       XK_space,                  spawn,           SHCMD("rofi -show run -theme $HOME/.config/dwm/resources/rofi/config.rasi") },
    { 0,                            XF86XK_AudioRaiseVolume,   spawn,           SHCMD("amixer set 'Master' 5%+ 1>/dev/null; kill -34 $(cat $HOME/.cache/pidofbar)") },
    { 0,                            XF86XK_AudioLowerVolume,   spawn,           SHCMD("amixer set 'Master' 5%- 1>/dev/null; kill -34 $(cat $HOME/.cache/pidofbar)") },
    { 0,                            XF86XK_AudioMute,          spawn,           SHCMD("amixer -D pulse set 'Master' 1+ toggle 1>/dev/null; kill -34 $(cat $HOME/.cache/pidofbar)") },
    { 0,                            XF86XK_MonBrightnessUp,    spawn,           SHCMD("brillo -q -A 2") },
    { 0,                            XF86XK_MonBrightnessDown,  spawn,           SHCMD("brillo -q -U 2") },
    { 0,                            XK_Page_Up,                NULL,            {0} },
    { 0,                            XK_Page_Down,              NULL,            {0} },
    TAGKEYS(                        XK_1,                                       0)
    TAGKEYS(                        XK_2,                                       1)
    TAGKEYS(                        XK_3,                                       2)
    TAGKEYS(                        XK_4,                                       3)
    TAGKEYS(                        XK_5,                                       4)
    TAGKEYS(                        XK_6,                                       5)
    TAGKEYS(                        XK_7,                                       6)
    TAGKEYS(                        XK_8,                                       7)
    TAGKEYS(                        XK_9,                                       8)
};

/* button definitions */
// click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin
static const Button buttons[] = {
    /* click                event mask      button          function        argument */
    { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
    { ClkWinTitle,          0,              Button2,        zoom,           {0} },
    { ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
    { ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
    { ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
    { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
    { ClkTagBar,            0,              Button1,        view,           {0} },
    { ClkTabBar,            0,              Button1,        focuswin,       {0} },
};

