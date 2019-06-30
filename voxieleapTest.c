#if 0
voxieleapTest.exe:  voxieleapTest.c voxiebox.h Leap_Voxon.h Leap_Voxon.cpp;
cl  /TP voxieleapTest.c Leap_Voxon.cpp /MT /link user32.lib

!if 0
#endif

// command: cl  /TP voxieleapTest.c Leap_Voxon.cpp /MT /link user32.lib
//Voxiebox example for C. Requires voxiebox.dll to be in path.
//NOTE:voxiebox.dll is compiled as 64-bit mode. Be sure to compile for 64-bit target or else voxie_load() will fail.
//To compile, type "nmake voxiesimp.c" at a VC command prompt, or set up in environment using hints from makefile above.
    
// Edit by Yi-Ting, Hsieh
// The code is derived from voxiedemo.c.
// Introduce leap motion SDK4.0(implement a wrapper API as Leap_Voxon) and implement a new menu system, which is the world's first gesture menu system.
// Project video: https://www.youtube.com/watch?v=zyG7rsY_ZL8
// Project website: https://gesturelab.blogspot.com/
#include "voxiebox.h"
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <intrin.h>
/* In order to use Leap Motion */
#include <process.h>
#include <malloc.h>
#define USELEAP 1
#define USEMAG6D 0

#define WIN32_LEAN_AND_MEAN
#define PI 3.14159265358979323

#if (USEMAG6D)
#define MAG6D_MAIN
#include "mag6dll.h"
static int gusemag6d;
#endif

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b) (((a)<(b))?(a):(b))
#endif

static int gxres, gyres;
static voxie_wind_t vw;
static voxie_frame_t vf;
static double tim = 0.0, otim, dtim, avgdtim = 0.0, cycletim = 0.0;

enum
{
    RENDMODE_TITLE=0,  RENDMODE_PHASESYNC,
    RENDMODE_KEYSTONECAL,
    RENDMODE_HEIGHTMAP,
    RENDMODE_MODELANIM,
    RENDMODE_GAMEFOLDER,
    RENDMODE_CHESS,
    RENDMODE_DOTMUNCH,
    RENDMODE_SNAKETRON,
    RENDMODE_FLYSTOMP,
    RENDMODE_END
};
static const char *iconnam[] =
{
    "title.kv6", "mode0.kv6",
    "mode1.kv6",
    "mode3.kv6",
    "mode13.kv6",
    "mode7.kv6",
    "mode7.kv6",
    "mode10.kv6",
    "mode11.kv6",
    "mode12.kv6",
};
static char *iconst[] =
{
    "Select App", "Phase sync",
    "Keystone Cal",
    "Height Map",
    "Model / Anim",
    "Game Apps",
    "Chess",
    "Dot Munch",
    "Snake Tron",
    "FlyStomp",
};

//SUB-folder for Game
//testing: drawing elements in circle:   25.09.18 YI-Ting, Hsieh
static int MENUGAMESTART = 6;
static int GAMENUMS = 5;
enum
{
    GAMEMODE_CHESS,     GAMEMODE_DOTMUNCH,
    GAMEMODE_SNAKETRON, GAMEMODE_FLYSTOMP,
    GAMEMODE_BACK
};

static const char *iconGamenam[] =
{
    "mode7.kv6",  "mode10.kv6",
    "mode11.kv6", "mode12.kv6",
    "title.kv6"
};
static char *iconGamest[] =
{
    "Chess",     "Dot Munch",
    "Snake Tron","FlyStomp",    "Menu"
};

// End Here
enum
{
    //Generic names
    MENU_RESET, MENU_PAUSE, MENU_PREV, MENU_NEXT, MENU_ENTER, MENU_LOAD, MENU_SAVE,
    MENU_LEFT, MENU_RIGHT, MENU_UP, MENU_DOWN, MENU_CEILING, MENU_FLOOR,
    MENU_WHITE, MENU_RED, MENU_GREEN, MENU_BLUE, MENU_CYAN, MENU_MAGENTA, MENU_YELLOW,

    //2 state switches
    MENU_AUTOMOVEOFF, MENU_AUTOMOVEON,
    MENU_AUTOCYCLEOFF, MENU_AUTOCYCLEON,
    MENU_TEXEL_NEAREST, MENU_TEXEL_BILINEAR,
    MENU_SLICEDITHEROFF, MENU_SLICEDITHERON,
    MENU_TEXTUREOFF, MENU_TEXTUREON,
    MENU_SLICEOFF, MENU_SLICEON,
    MENU_DRAWSTATSOFF, MENU_DRAWSTATSON,

    //Other
    MENU_AUTOROTATEOFF, MENU_AUTOROTATEX, MENU_AUTOROTATEY, MENU_AUTOROTATEZ, MENU_AUTOROTATESPD,
    MENU_FRAME_PREV, MENU_FRAME_NEXT,
    MENU_SOL0, MENU_SOL1, MENU_SOL2, MENU_SOL3,
    MENU_HINT, MENU_DIFFICULTY, MENU_SPEED,
    MENU_ZIGZAG, MENU_SINE, MENU_HUMP_LEVEL,
    MENU_DISP_CUR, MENU_DISP_END=MENU_DISP_CUR+MAXDISP-1,
};

static char *gmediadir = 0;
static int ghitkey = 0; //generic simulated keypress keyboard scancode for menu hacks
static int gdrawstats = 0, gautocycle = 0;
//static int gautorotateax = -1, gautorotatespd = 5;
static int gautorotatespd[3] = {0,0,0};

//Title globals
static double gtimbore = 0.0;
static int menu_title_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_UP:    ghitkey = 0xc8; break;
    case MENU_LEFT:  ghitkey = 0xcb; break;
    case MENU_RIGHT: ghitkey = 0xcd; break;
    case MENU_DOWN:  ghitkey = 0xd0; break;
    case MENU_ENTER: ghitkey = 0x1c; break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Phasesync globals
static int gphasesync_curmode = 0, gphasesync_cursc = 4;
static int menu_phasesync_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_ZIGZAG:     gphasesync_curmode = 0; break;
    case MENU_SINE:       gphasesync_curmode = 1; break;
    case MENU_HUMP_LEVEL: gphasesync_cursc = 16>>(((int)v)-1); break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Keystonecal globals
typedef struct { float curx, cury, curz; } keystone_t;
static keystone_t keystone = {0};
static int menu_keystonecal_update (int id, double v, int how, void *userdata)
{
    keystone_t *keystone = (keystone_t *)userdata;
    switch(id)
    {
    case MENU_CEILING: keystone->curz = -vw.aspz; break;
    case MENU_FLOOR:   keystone->curz = +vw.aspz; break;
    case MENU_DISP_CUR: case MENU_DISP_CUR+1: case MENU_DISP_CUR+2:
        vw.dispcur = id-MENU_DISP_CUR;
        voxie_init(&vw);
        break;
    }
    return(1);
}

//--------------------------------------------------------------------------------------------------
//Wavyplane globals
//--------------------------------------------------------------------------------------------------
//Heightmap globals
typedef struct { char file[MAX_PATH*2+2]; point3d sp, sr, sd, sf, p, r, d, f; int colorkey, usesidebyside, mapzen; } dem_t;
#define GDEMMAX 256
static dem_t gdem[GDEMMAX];
static int gdemi = 0, gdemn = 0, gheightmap_flags = 7;
static int menu_heightmap_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_PREV:           ghitkey = 0xc9; break;
    case MENU_NEXT:           ghitkey = 0xd1; break;
    case MENU_AUTOCYCLEOFF:   gautocycle = 0;         break;
    case MENU_AUTOCYCLEON:    gautocycle = 1;         break;
    case MENU_TEXEL_NEAREST:  gheightmap_flags &= ~2; break;
    case MENU_TEXEL_BILINEAR: gheightmap_flags |=  2; break;
    case MENU_SLICEDITHEROFF: gheightmap_flags &= ~1; break;
    case MENU_SLICEDITHERON:  gheightmap_flags |=  1; break;
    case MENU_TEXTUREOFF:     gheightmap_flags &= ~4; break;
    case MENU_TEXTUREON:      gheightmap_flags |=  4; break;
    case MENU_RESET:          ghitkey = 0x0e; break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Voxieplayer globals
typedef struct { char file[MAX_PATH]; int mode, rep; } rec_t;
#define GRECMAX 256
static rec_t grec[GRECMAX];
static int greci = 0, grecn = 0, gautocycleall = 1, gvoxieplayer_ispaused = 0;
static int menu_voxieplayer_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_PREV:         ghitkey = 0xc9; break;
    case MENU_NEXT:         ghitkey = 0xd1; break;
    case MENU_DRAWSTATSOFF: gdrawstats = 0; break;
    case MENU_DRAWSTATSON:  gdrawstats = 1; break;
    case MENU_AUTOCYCLEOFF: gautocycleall = 0; break;
    case MENU_AUTOCYCLEON:  gautocycleall = 1; break;
    case MENU_PAUSE:        gvoxieplayer_ispaused = !gvoxieplayer_ispaused; break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Platonicsolids globals
static int platonic_col = 7, platonic_solmode = 2, platonic_ispaused = 0;
static int menu_platonic_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_SOL0:    platonic_solmode = 0; break;
    case MENU_SOL1:    platonic_solmode = 1; break;
    case MENU_SOL2:    platonic_solmode = 2; break;
    case MENU_SOL3:    platonic_solmode = 3; break;
    case MENU_WHITE:   platonic_col = 7; break;
    case MENU_RED:     platonic_col = 4; break;
    case MENU_GREEN:   platonic_col = 2; break;
    case MENU_BLUE:    platonic_col = 1; break;
    case MENU_CYAN:    platonic_col = 3; break;
    case MENU_MAGENTA: platonic_col = 5; break;
    case MENU_YELLOW:  platonic_col = 6; break;
    case MENU_PAUSE:   platonic_ispaused = !platonic_ispaused; break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Flyingstuff globals
static char gmyst[20+1] = {"Voxiebox by VOXON :)"};
//--------------------------------------------------------------------------------------------------
//Chess globals
static int gchesscol[2] = {0x808080,0x104010};
static int gchessailev[2] = {0,5};
static int gchess_automove = 0, gchess_hint = 0;
static float gchesstime = 1.0;
static int menu_chess_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_HINT: gchess_hint = 1; break;
    case MENU_AUTOMOVEOFF: gchess_automove = 0; break;
    case MENU_AUTOMOVEON: gchess_automove = 1; break;
    case MENU_DIFFICULTY: gchessailev[1] = (int)v; break;
    case MENU_RESET: ghitkey = 0x0e; break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Packer globals
static int gpakrendmode = 4, gpakxdim = 6, gpakydim = 3;
static int menu_packer_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_SOL0:  gpakrendmode = 1; break;
    case MENU_SOL1:  gpakrendmode = 2; break;
    case MENU_SOL2:  gpakrendmode = 4; break;
    case MENU_SOL3:  gpakrendmode = 6; break;
    case MENU_UP:    ghitkey = 0xc8; break;
    case MENU_LEFT:  ghitkey = 0xcb; break;
    case MENU_RIGHT: ghitkey = 0xcd; break;
    case MENU_DOWN:  ghitkey = 0xd0; break;
    case MENU_RESET: ghitkey = 0x0e; break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Paratrooper globals
static int menu_paratrooper_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_RESET: ghitkey = 0x0e; break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Dotmunch globals
typedef struct { int xsiz, ysiz, zsiz, xwrap, ywrap, zwrap; char *board; } mun_t;
#define MUNMAX 16
static int munlev = 0, munn = 2;
static mun_t gmun[MUNMAX] =
{
    21,19,1, 1,0,0,
    ".Mxxxxxxxx.xxxxxxxxM."\
    ".P...x...x.x...x...P."\
    ".xxxxxxxxxxxxxxxxxxx."\
    ".x...x.x.....x.x...x."\
    ".Mxxxx.xxx.xxx.xxxxM."\
    ".....x...x.x...x....."\
    ".....x.xxxxxxx.x....."\
    ".....x.x.....x.x....."\
    "xxxxxxxx.....xxxxxxxx"\
    ".....x.x.....x.x....."\
    ".....x.xxxxxxx.x....."\
    ".....x.x.....x.x....."\
    ".xxxxxxxxx.xxxxxxxxx."\

    ".P...x...x.x...x...P."\
    ".xxx.xxxxxSxxxxx.xxx."\
    "...x.x.x.....x.x.x..."\
    ".xxxxx.xxx.xxx.xxxxx."\
    ".x.......x.x.......x."\
    ".xxxxxxxxxxxxxxxxxxx.",


    11,11,5, 0,0,0,
    "..........."\
    "....xxxxxP."\
    "....x....x."\
    "x...xxxM.x."\
    "x......x.x."\
    "xxxxxP.x.x."\
    "x....x.xxx."\
    "x....x....."\
    "x....x....."\
    "x....x....."\
    "Sxxxxx....."\

    "..........."\
    "..........."\
    "..........."\
    "x......x..."\
    "..........."\
    "x....x....."\
    "..........."\
    "..........."\
    "..........."\
    "..........."\
    "x....x....."\

    ".......xxxx"\
    ".......x..x"\
    ".......x..x"\
    "x......x..x"\
    "..........x"\
    "x....x....x"\
    "..........x"\
    "..........x"\
    "..........x"\
    "..........x"\
    "x....x.xxxx"\

    ".......x..."\
    "..........."\
    "..........."\
    "x.........x"\
    "..........."\
    "x....x....."\
    "..........."\
    "..........."\
    "..........."\
    "..........."\
    "x....x.x..."\

    "....xxxMxxx"\
    "....x.....x"\
    "....x.....x"\
    "xxxxx.....M"\
    "..........."\
    "xxxxxx....."\
    "x....x....."\
    "x....x....."\
    "x....x....."\
    "x....x....."\
    "Sxxxxxxx...",
};
static mun_t *cmun = &gmun[munlev];
static int menu_dotmunch_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_PREV:  ghitkey = 0xc9; break;
    case MENU_NEXT:  ghitkey = 0xd1; break;
    case MENU_RESET: ghitkey = 0x0e; break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Snaketron globals
static int gminplays = 2, gdualnav = 0;
static int gsnaketime = 0, gsnakenumpels = 2, gsnakenumtetras = 2;
static float gsnakepelspeed = 0.25f;
static int menu_snaketron_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_SPEED:
        switch((int)v)
        {
        case 1: gsnakepelspeed = 0.f; break;
        case 2: gsnakepelspeed = 0.0625f; break;
        case 3: gsnakepelspeed = 0.125f; break;
        case 4: gsnakepelspeed = 0.25f; break;
        case 5: gsnakepelspeed = 0.5f; break;
        case 6: gsnakepelspeed = 1.f; break;
        case 7: gsnakepelspeed = 2.f; break;
        }
        ghitkey = 0x27;
        break;
    case MENU_RESET: ghitkey = 0x0e; break;
    }
    return(1);
}
//--------------------------------------------------------------------------------------------------
//Flystomp globals
static int menu_flystomp_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_RESET: ghitkey = 0x0e; break;
    }
    return(1);
}

//--------------------------------------------------------------------------------------------------
static point3d plat_vert[4+8+6+20+12];
static int plat_vertoffs[5+1], plat_mesh[5][80], plat_meshn[5];
static void platonic_init (void)
{
    static const int tetr_facei[] = {0,1,2, 0,3,1, 0,2,3, 1,3,2};
    static const int cube_facei[] = {0,1,3,2, 5,4,6,7, 1,0,4,5, 2,3,7,6, 0,2,6,4, 3,1,5,7};
    static const int octa_facei[] = {0,2,4, 2,1,4, 1,3,4, 3,0,4, 0,3,5, 3,1,5, 1,2,5, 2,0,5};
    static const int dode_facei[] = {14,13,12,11,10, 10,11,1,8,0, 11,12,2,9,1, 12,13,3,5,2, 13,14,4,6,3, 14,10,0,7,4, 15,16,17,18,19, 15,19,9,2,5, 16,15,5,3,6, 17,16,6,4,7, 18,17,7,0,8, 19,18,8,1,9};
    static const int icos_facei[] = {6,11,9, 6,1,11, 6,4,1, 6,0,4, 6,9,0, 11,7,9, 7,11,3, 1,3,11, 3,1,10, 4,10,1, 10,4,8, 0,8,4, 8,0,2, 9,2,0, 2,9,7, 5,3,10, 5,10,8, 5,7,3, 5,2,7, 5,8,2};
    static const int *facei;
    float phi, fx, fy, fz, a, b;
    int i, j, k, m, nv, nfacei, nvert, vertperface;

    nv = 0; plat_vertoffs[0] = nv;
    for(m=0;m<5;m++)
    {
        switch(m)
        {
        case 0: //Tetra (based on RIGIDLINE3D.KC)
            a = 1.0/sqrt(3.0);
            for(i=0;i<4;i++) //corners of cube
            {
                plat_vert[nv+i].x = (float)(1-((i+1)&2))*a;
                plat_vert[nv+i].y = (float)(1-(i&1)*2)*a;
                plat_vert[nv+i].z = (float)(1-(i&2))*a;
            }
            nvert = 4; vertperface = 3; facei = tetr_facei; nfacei = sizeof(tetr_facei)/sizeof(int); break;
        case 1: //Cube
            a = 1.0/sqrt(3.0);
            for(i=0;i<8;i++)
            {
                plat_vert[nv+i].x = (float)(((i>>0)&1)*2-1)*a;
                plat_vert[nv+i].y = (float)(((i>>1)&1)*2-1)*a;
                plat_vert[nv+i].z = (float)(((i>>2)&1)*2-1)*a;
            }
            nvert = 8; vertperface = 4; facei = cube_facei; nfacei = sizeof(cube_facei)/sizeof(int); break;
        case 2: //Octa (based on RIGIDLINE3D.KC)
            for(i=0;i<2;i++)
            {
                plat_vert[nv+i  ].x = i*2-1; plat_vert[nv+i  ].y =     0; plat_vert[nv+i  ].z =     0;
                plat_vert[nv+i+2].x =     0; plat_vert[nv+i+2].y = i*2-1; plat_vert[nv+i+2].z =     0;
                plat_vert[nv+i+4].x =     0; plat_vert[nv+i+4].y =     0; plat_vert[nv+i+4].z = i*2-1;
            }
            nvert = 6; vertperface = 3; facei = octa_facei; nfacei = sizeof(octa_facei)/sizeof(int); break;
        case 3: //Dodec (based on DODEC2.KC)
            phi = (sqrt(5.f)+1.f)*.5f;
            for(j=0,k=0;j<4;j++)
                for(i=0;i<5;i++,k++)
                {
                    fx = cos(i*(PI*.4f));
                    fy = sin(i*(PI*.4f));
                    if (j < 2) fz = 1.f-phi*.5f; else { fx *= phi-1.f; fy *= phi-1.f; fz = phi*.5f; }
                    if (j&1) { fx = -fx; fy = -fy; fz = -fz; }
                    plat_vert[nv+k].x = fx; plat_vert[nv+k].y = fy; plat_vert[nv+k].z = fz;
                }
            nvert = 20; vertperface = 5; facei = dode_facei; nfacei = sizeof(dode_facei)/sizeof(int); break;
        case 4: //Icos (based on RIGIDLINE3D.KC)
            phi = (sqrt(5.f)+1.f)*.5f; a = phi*.5f; b = .5f;
            for(fx=-1.f,k=0;fx<=1.f;fx+=2.f)
                for(fy=-1.f;fy<=1.f;fy+=2.f,k++)
                {
                    plat_vert[nv+k  ].x =  0.f; plat_vert[nv+k  ].y = fx*b; plat_vert[nv+k  ].z = fy*a;
                    plat_vert[nv+k+4].x = fx*b; plat_vert[nv+k+4].y = fy*a; plat_vert[nv+k+4].z =  0.f;
                    plat_vert[nv+k+8].x = fy*a; plat_vert[nv+k+8].y =  0.f; plat_vert[nv+k+8].z = fx*b;
                }
            nvert = 12; vertperface = 3; facei = icos_facei; nfacei = sizeof(icos_facei)/sizeof(int); break;
        }

        plat_meshn[m] = 0;
        for(i=0;i<nfacei;i+=vertperface)
        {
            memcpy(&plat_mesh[m][plat_meshn[m]],&facei[i],vertperface*sizeof(int)); plat_meshn[m] += vertperface;
            plat_mesh[m][plat_meshn[m]] = -1; plat_meshn[m]++;
        }
        nv += nvert; plat_vertoffs[m+1] = nv;
    }
}

static void draw_platonic (int ind, float posx, float posy, float posz, float rad, float rot, int fillmode, int col)
{
    poltex_t *vt;
    point3d p0, p1;
    float c, s;
    int i, vo, nv;

    vo = plat_vertoffs[ind]; nv = plat_vertoffs[ind+1]-vo;
    vt = (poltex_t *)_alloca(nv*sizeof(vt[0]));
    c = cos(rot); s = sin(rot);
    for(i=nv-1;i>=0;i--)
    {
        p0 = plat_vert[i+vo];
        p1.x = p0.x*c - p0.y*s; p1.y = p0.y*c + p0.x*s; p1.z = p0.z;
        p0.x = p1.x*c - p1.z*s; p0.z = p1.z*c + p1.x*s; p0.y = p1.y;
        p1.y = p0.y*c - p0.z*s; p1.z = p0.z*c + p0.y*s; p1.x = p0.x;
        vt[i].x = p1.x*rad + posx;
        vt[i].y = p1.y*rad + posy;
        vt[i].z = p1.z*rad + posz;
        vt[i].u = (float)(i > 0);
        vt[i].v = (float)(i > 1);
        vt[i].col = col;
    }
    voxie_drawmeshtex(&vf,0,vt,nv,plat_mesh[ind],plat_meshn[ind],fillmode,col);
}
/* adding for menu: 31.08.18 */
//--------------------------------------------------------------------------------------------------
//Modelanim globals
typedef struct
{
    char file[MAX_PATH], snd[MAX_PATH];
    tiletype *tt;
    point3d p, p_init;
    float ang[3], sc, ang_init[3];
    float fps, defscale;
    int filetyp, mode, n, mal, cnt, loaddone; //filetyp:/*0=PNG,*/ 1=KV6,etc..
} anim_t;
#define GANIMMAX 256
static anim_t ganim[GANIMMAX];
static int ganimi = 0, ganimn = 0;
static int gmodelanim_pauseit = 0, gslicemode = 0;
static int grotatex = 0, grotatey = 0, grotatez = 5;
static point3d gslicep, gslicer, gsliced, gslicef;
static int menu_modelanim_update (int id, double v, int how, void *userdata)
{
    switch(id)
    {
    case MENU_PREV:          ghitkey = 0xc9; break;
    case MENU_NEXT:          ghitkey = 0xd1; break;
    case MENU_FRAME_PREV:    ghitkey = 0xc8; break;
    case MENU_FRAME_NEXT:    ghitkey = 0xd0; break;
    case MENU_DRAWSTATSOFF:  gdrawstats = 0; break;
    case MENU_DRAWSTATSON:   gdrawstats = 1; break;
    case MENU_AUTOCYCLEOFF:  gautocycle = 0; break;
    case MENU_AUTOCYCLEON:   gautocycle = 1; break;
    case MENU_SLICEOFF:      gslicemode = 0; break;
    case MENU_SLICEON:       gslicemode = 1;
        gslicep.x = 0.f; gslicer.x = .05f; gsliced.x = 0.f; gslicef.x = 0.f;
        gslicep.y = 0.f; gslicer.y = 0.f; gsliced.y = .05f; gslicef.y = 0.f;
        gslicep.z = 0.f; gslicer.z = 0.f; gsliced.z = 0.f; gslicef.z = .05f; break;
    case MENU_RESET:         ghitkey = 0x0e; break;
    case MENU_PAUSE:         gmodelanim_pauseit = !gmodelanim_pauseit; break;
    case MENU_LOAD:          ghitkey = 0x26; break;
    case MENU_SAVE:          ghitkey = 0x1f; break;

#if 0
    case MENU_AUTOROTATEOFF: gautorotateax =-1; break;
    case MENU_AUTOROTATEX:   gautorotateax = 0; break;
    case MENU_AUTOROTATEY:   gautorotateax = 1; break;
    case MENU_AUTOROTATEZ:   gautorotateax = 2; break;
    case MENU_AUTOROTATESPD: gautorotatespd = (int)v; break;
#else
    case MENU_AUTOROTATEX:   gautorotatespd[0] = (int)v; break;
    case MENU_AUTOROTATEY:   gautorotatespd[1] = (int)v; break;
    case MENU_AUTOROTATEZ:   gautorotatespd[2] = (int)v; break;
#endif
    }
    return(1);
}

typedef struct { int rendmode; char file[MAX_PATH]; } mus_t;
#define GMUSMAX 32
static mus_t gmus[GRECMAX];
static int gmusn = 0;

static int keymenu[2] = {0x29,0x39}; //menu key is: ` or tilda w/no shift by default
static int keyremap[8][7] =
{ //Lef  Rig  Up  Dow  LBut RBut  Mid
  0xcb,0xcd,0xc8,0xd0,0x9d,0x36,0x1c, //P1
  0x4b,0x4d,0x48,0x50,0xb5,0x37,0x00, //Q1
  0x24,0x26,0x17,0x25,0x2c,0x2d,0x00, //P2
  0x00,0x00,0x00,0x00,0x00,0x00,0x00, //Q2
  0x20,0x22,0x13,0x21,0x1e,0x1f,0x00, //P3
  0x00,0x00,0x00,0x00,0x00,0x00,0x00, //Q3
  0x2f,0x16,0x15,0x31,0x14,0x23,0x00, //P4
  0x00,0x00,0x00,0x00,0x00,0x00,0x00, //Q4
};

#define MAXFRAMESPERVOL 128
#define MAXPLANES (MAXFRAMESPERVOL*24)
static int clutmid[MAXPLANES], grendmode = RENDMODE_TITLE, gcurselmode = 0, gshowborder = 1, gmenutimeout = 60;
static int div3[MAXPLANES], mod3[MAXPLANES], div24[MAXPLANES], mod24[MAXPLANES], oneupmod24[MAXPLANES];



//Rotate vectors a & b around their common plane, by ang
static void rotate_vex (float ang, point3d *a, point3d *b)
{
    float f, c, s;
    int i;

    c = cos(ang); s = sin(ang);
    f = a->x; a->x = f*c + b->x*s; b->x = b->x*c - f*s;
    f = a->y; a->y = f*c + b->y*s; b->y = b->y*c - f*s;
    f = a->z; a->z = f*c + b->z*s; b->z = b->z*c - f*s;
}

//NOTE:assumes axis is unit length!
static void rotate_ax (point3d *p, point3d *ax, float w) //10/26/2011:optimized algo :)
{
    point3d op;
    float f, c, s;

    c = cos(w); s = sin(w);

    //P = cross(AX,P)*s + dot(AX,P)*(1-c)*AX + P*c;
    op = (*p);
    f = (op.x*ax->x + op.y*ax->y + op.z*ax->z)*(1.f-c);
    p->x = (ax->y*op.z - ax->z*op.y)*s + ax->x*f + op.x*c;
    p->y = (ax->z*op.x - ax->x*op.z)*s + ax->y*f + op.y*c;
    p->z = (ax->x*op.y - ax->y*op.x)*s + ax->z*f + op.z*c;
}





//Find shortest path between 2 line segments
//Input: 2 line segments: a0-a1, b0-b1
//Output: 2 intersection points: ai on segment a0-a1, bi on segment b0-b1
//Returns: distance between ai&bi
static void roundcylminpath (point3d *a0, point3d *a1, point3d *b0, point3d *b1, point3d *ai, point3d *bi)
{
    point3d av, bv, ab;
    float k0, k1, k2, k3, k4, det, t, u;

    av.x = a1->x-a0->x; bv.x = b1->x-b0->x; ab.x = b0->x-a0->x;
    av.y = a1->y-a0->y; bv.y = b1->y-b0->y; ab.y = b0->y-a0->y;
    av.z = a1->z-a0->z; bv.z = b1->z-b0->z; ab.z = b0->z-a0->z;
    k0 = av.x*av.x + av.y*av.y + av.z*av.z;
    k1 = bv.x*bv.x + bv.y*bv.y + bv.z*bv.z;
    k2 = av.x*ab.x + av.y*ab.y + av.z*ab.z;
    k3 = bv.x*ab.x + bv.y*ab.y + bv.z*ab.z;
    k4 = av.x*bv.x + av.y*bv.y + av.z*bv.z;
    // k0*t - k4*u = k2
    //-k4*t + k1*u =-k3
    det = k0*k1 - k4*k4;
    if (det != 0.0)
    {
        det = 1.0/det;
        t = (k1*k2 - k3*k4)*det;
        u = (k2*k4 - k0*k3)*det;
    } else { t = 0.0; u = -k2/k3; }
    t = min(max(t,0.0),1.0);
    u = min(max(u,0.0),1.0);
    ai->x = av.x*t + a0->x; bi->x = bv.x*u + b0->x;
    ai->y = av.y*t + a0->y; bi->y = bv.y*u + b0->y;
    ai->z = av.z*t + a0->z; bi->z = bv.z*u + b0->z;
    //return(sqrt((bi->x-ai->x)*(bi->x-ai->x) + (bi->y-ai->y)*(bi->y-ai->y) + (bi->z-ai->z)*(bi->z-ai->z)));
}

static void vecrand (point3d *a, float mag) //UNIFORM spherical randomization (see spherand.c)
{
    float f;

    a->z = (float)rand()/(float)RAND_MAX*2.0-1.0;
    f = (float)rand()/(float)RAND_MAX*(PI*2.0); a->x = cos(f); a->y = sin(f);
    f = sqrt(1.0 - a->z*a->z)*mag; a->x *= f; a->y *= f; a->z *= mag;
}



#define MAXDEP 8
static int rank[16] = {0,1,3,3,5,15,1024,0,0,0,-1024,-15,-5,-3,-3,-1};
static int gmove[16384], gmoven = 0;
static int moves[MAXDEP][320], csc[MAXDEP][320];
static int guseprune;


static int ksgn (int i) { if (i < 0) return(-1); if (i > 0) return(+1); return(0); }

//Assumes (kingx,kingy) is turn's king (even if it's not)
static int ischeck (int board[8][8], int kingx, int kingy, int turn)
{
    int i, b, t, x, y, x0, y0, x1, y1;

    t = 1-turn*2;

    //check king
    x0 = max(kingx-1,0); x1 = min(kingx+1,7);
    y0 = max(kingy-1,0); y1 = min(kingy+1,7);
    for(y=y0;y<=y1;y++) for(x=x0;x<=x1;x++) if (board[y][x] == t*-6) return(1);

    //check pawn
    if (kingy != (1-turn)*7)
    {
        if ((kingx > 0) && (board[kingy+t][kingx-1] == -t)) return(1);
        if ((kingx < 7) && (board[kingy+t][kingx+1] == -t)) return(1);
    }

    //check knight
    static const int knix[8] = {+1,+2,+2,+1,-1,-2,-2,-1};
    static const int kniy[8] = {+2,+1,-1,-2,-2,-1,+1,+2};
    for(i=8-1;i>=0;i--)
    {
        x = knix[i]+kingx; if ((x < 0) || (x >= 8)) continue;
        y = kniy[i]+kingy; if ((y < 0) || (y >= 8)) continue;
        if (board[y][x] == t*-2) return(1);
    }

    //check others
    static const int dirx[8] = {-1,+1, 0, 0,-1,+1,-1,+1};
    static const int diry[8] = { 0, 0,-1,+1,-1,-1,+1,+1};
    static const int dirt[8] = {-4,-4,-4,-4,-3,-3,-3,-3};
    for(i=8-1;i>=0;i--)
    {
        x = kingx; y = kingy;
        do
        {
            x += dirx[i]; if ((x < 0) || (x >= 8)) break;
            y += diry[i]; if ((y < 0) || (y >= 8)) break;
            b = board[y][x]; if (!b) continue;
            b *= t; if ((b == -5) || (b == dirt[i])) return(1); //opp: queen=t*-5, bishop=t*-3, rook=t*-4
            break;
        } while (1);
    }

    return(0);
}

static int domove (int board[8][8], int *caststat, int *prevmove, int x0, int y0, int x1, int y1, int doit)
{
    int sc, o, n, t, kx, ky;

    sc = 0;
    o = board[y0][x0]; t = ksgn(o);
    n = board[y1][x1];

    (*prevmove) = y1*512 + x1*64 + y0*8 + x0;

    if (labs(o) == 1) //pawn
    {
        //promote to queen
        if ((o > 0) && (y1 == 7)) { o = 5; sc += rank[5]-rank[1]; }
        else if ((o < 0) && (y1 == 0)) { o =-5; sc -= rank[5]-rank[1]; }

        //en passant
        if ((n == 0) && (x0 != x1))
        {
            if (doit) { voxie_playsound("getstuff.flac",-1,100,100,1.f); }
            if (board[y0][x1]) sc -= rank[board[y0][x1]&15];
            board[y0][x1] = 0;
        }
    }

    if (labs(o) == 6) //castle
    {
        //moving king kills castle opportunity
        if (o > 0) { (*caststat) +=  3-((*caststat)& 3)  ; }
        else { (*caststat) += 12-((*caststat)>>2)*4; }
        if (labs(x1-x0) == 2)
        {
            board[y1][(x1>4)*2+3] = t*4;
            board[y1][(x1>4)*7] = 0;
        }
    }

    if (labs(o) == 4) //rook: moving it kills castle opportunity
    {
        if (o > 0)
        {
            if (x0 == 0) { if (!((*caststat)&1)) (*caststat) += 1; }
            else if (x0 == 7) { if (!((*caststat)&2)) (*caststat) += 2; }
        }
        else
        {
            if (x0 == 0) { if (!((*caststat)&4)) (*caststat) += 4; }
            else if (x0 == 7) { if (!((*caststat)&8)) (*caststat) += 8; }
        }
    }

    board[y0][x0] = 0;
    if (board[y1][x1])
    {
        if (y1 == 0)
        {
            if (x1 == 0) { if (!((*caststat)&1)) (*caststat) += 1; }
            else if (x1 == 7) { if (!((*caststat)&2)) (*caststat) += 2; }
        }
        else
        {
            if (x1 == 0) { if (!((*caststat)&4)) (*caststat) += 4; }
            else if (x1 == 7) { if (!((*caststat)&8)) (*caststat) += 8; }
        }
        sc -= rank[n&15];
    }
    board[y1][x1] = o;

    if (doit)
    {
        gmove[gmoven] = (*prevmove); gmoven++;
        if (n)
        {
            if (labs(n) == 1) voxie_playsound("getstuff.flac",-1,100,100,1.f);
            if (labs(n) == 2) voxie_playsound("blowup.flac",-1,100,100,1.f);
            if (labs(n) == 3) voxie_playsound("blowup.flac",-1,100,100,1.f);
            if (labs(n) == 4) voxie_playsound("blowup.flac",-1,100,100,1.f);
            if (labs(n) == 5) voxie_playsound("death.flac",-1,100,100,1.f);
            if (labs(n) == 6) voxie_playsound("closdoor.flac",-1,100,100,1.f);
        }

        for(ky=0;ky<8;ky++)
            for(kx=0;kx<8;kx++)
            {
                if (board[ky][kx] != t*-6) continue;
                if (ischeck(board,kx,ky,1-(o<0))) voxie_playsound("alarm.flac",-1,100,100,1.f);
                goto break2;
            }
break2:;
    }

    return(sc*t);
}

static int domove_ret_sc_only (int board[8][8], int x0, int y0, int x1, int y1)
{
    int sc, o, n;

    sc = 0;
    o = board[y0][x0];
    n = board[y1][x1];

    if (labs(o) == 1)
    {
        //promote to queen
        if ((o > 0) && (y1 == 7)) { sc = rank[5]-rank[1]; }
        else if ((o < 0) && (y1 == 0)) { sc = rank[1]-rank[5]; }

        //en passant
        if ((n == 0) && (x0 != x1) && (board[y0][x1])) sc = -rank[board[y0][x1]&15];
    }

    if (n) sc -= rank[n&15];
    return(ksgn(o)*sc);
}

static void undomove (int board[8][8], int x0, int y0, int x1, int y1, int patch0, int patch1)
{
    if ((labs(board[y1][x1]) == 1) && (x0 != x1) && (!patch1)) //undo en passant
    { board[y0][x1] = -board[y1][x1]; }
    board[y0][x0] = patch0;
    board[y1][x1] = patch1;
    if ((labs(patch0) == 6) && (labs(x1-x0) == 2)) //undo castle
    {
        board[y1][(x1>4)*2+3] = 0;
        board[y1][(x1>4)*7] = ksgn(patch0)*4;
    }
}

static void move2xys (int k, int *x0, int *y0, int *x1, int *y1)
{
    (*x0) = ((k   )&7); (*y0) = ((k>>3)&7);
    (*x1) = ((k>>6)&7); (*y1) = ((k>>9)&7);
}

static int getvalmoves (int board[8][8], int caststat, int prevmove, int turn, int dep)
{
    int i, j, m, n, p, t, x, y, x0, y0, x1, y1, ox0, oy0, ox1, oy1, b, kx, ky, ocaststat, oprevmove, patch0, patch1;

    n = 0; t = 1-turn*2;
    for(y0=0;y0<8;y0++)
        for(x0=0;x0<8;x0++)
        {
            p = board[y0][x0]; if (ksgn(p) != t) continue;
            p = labs(p); m = (y0*8+x0)*65;

            if (p == 1) //pawn
            {
                if (!board[y0+t][x0])
                {
                    moves[dep][n] = m + t*512; n++;
                    if ((y0 == turn*5+1) && (!board[y0+t*2][x0]))
                    { moves[dep][n] = m + t*1024; n++; }
                }
                for(x=-1;x<=1;x+=2)
                {
                    if ((x+x0 < 0) || (x+x0 >= 8)) continue;
                    if (ksgn(board[y0+t][x+x0]) == -t) { moves[dep][n] = m + t*512 + x*64; n++; }
                }
                if (y0 == 4-turn) //capture en passant
                {
                    move2xys(prevmove,&ox0,&oy0,&ox1,&oy1);
                    if ((board[oy1][ox1] == -t) && (labs(oy1-oy0) == 2) && (labs(ox0-x0) == 1))
                    { moves[dep][n] = m + t*512 + (ox1-x0)*64; n++; }
                }
                continue;
            }
            if (p == 2) //knight
            {
                static const int knix[8] = {+1,+2,+2,+1,-1,-2,-2,-1};
                static const int kniy[8] = {+2,+1,-1,-2,-2,-1,+1,+2};
                static const int knia[8] = {1024+64,512+128,-512+128,-1024+64,-1024-64,-512-128,512-128,1024-64};
                for(i=8-1;i>=0;i--)
                {
                    x = knix[i]+x0; if ((x < 0) || (x >= 8)) continue;
                    y = kniy[i]+y0; if ((y < 0) || (y >= 8)) continue;
                    if (ksgn(board[y][x]) == t) continue;
                    moves[dep][n] = knia[i] + m; n++;
                }
                continue;
            }
            if (p == 6) //king
            {
                static const int kinx[8] = {-1, 0,+1,-1,+1,-1, 0,+1};
                static const int kiny[8] = {-1,-1,-1, 0, 0,+1,+1,+1};
                static const int kina[8] = {-9*64,-8*64,-7*64,-1*64,+1*64,+7*64,+8*64,+9*64};
                for(i=8-1;i>=0;i--)
                {
                    x = kinx[i]+x0; if ((x < 0) || (x >= 8)) continue;
                    y = kiny[i]+y0; if ((y < 0) || (y >= 8)) continue;
                    if (ksgn(board[y][x]) == t) continue;
                    moves[dep][n] = kina[i] + m; n++;
                }
                if ((x0 == 4) && (y0 == turn*7)) //castle
                {
                    //long castle
                    if ((board[y0][3] == 0) && (board[y0][2] == 0) && (board[y0][1] == 0) &&
                            (board[y0][0] == t*4) && ((caststat%(turn* 6+2)) < turn*3+1))
                    {
                        if ((!ischeck(board,4,y0,turn)) &&
                                (!ischeck(board,3,y0,turn)) &&
                                (!ischeck(board,2,y0,turn))) { moves[dep][n] = m - 128; n++; }
                    }

                    //short castle
                    if ((board[y0][5] == 0) && (board[y0][6] == 0) &&
                            (board[y0][7] == t*4) && ((caststat%(turn*12+4)) < turn*6+2))
                    {
                        if ((!ischeck(board,4,y0,turn)) &&
                                (!ischeck(board,5,y0,turn)) &&
                                (!ischeck(board,6,y0,turn))) { moves[dep][n] = m + 128; n++; }
                    }
                }
                continue;
            }
            if (p != 3) //horiz/vert (rook&queen)
            {
                for(i=1,j=x0;i<=j;i++)
                {
                    b = board[y0][x0-i]; if (ksgn(b) == t) break;
                    moves[dep][n] = i*(-64) + m; n++; if (b) break;
                }
                for(i=1,j=7-x0;i<=j;i++)
                {
                    b = board[y0][x0+i]; if (ksgn(b) == t) break;
                    moves[dep][n] = i*(+64) + m; n++; if (b) break;
                }
                for(i=1,j=y0;i<=j;i++)
                {
                    b = board[y0-i][x0]; if (ksgn(b) == t) break;
                    moves[dep][n] = i*(-512) + m; n++; if (b) break;
                }
                for(i=1,j=7-y0;i<=j;i++)
                {
                    b = board[y0+i][x0]; if (ksgn(b) == t) break;
                    moves[dep][n] = i*(+512) + m; n++; if (b) break;
                }
            }
            if (p != 4) //diag (bishop&queen)
            {
                for(i=1,j=min(x0,y0);i<=j;i++)
                {
                    b = board[y0-i][x0-i]; if (ksgn(b) == t) break;
                    moves[dep][n] = i*(-64-512) + m; n++; if (b) break;
                }
                for(i=1,j=min(7-x0,7-y0);i<=j;i++)
                {
                    b = board[y0+i][x0+i]; if (ksgn(b) == t) break;
                    moves[dep][n] = i*(+64+512) + m; n++; if (b) break;
                }
                for(i=1,j=min(x0,7-y0);i<=j;i++)
                {
                    b = board[y0+i][x0-i]; if (ksgn(b) == t) break;
                    moves[dep][n] = i*(-64+512) + m; n++; if (b) break;
                }
                for(i=1,j=min(7-x0,y0);i<=j;i++)
                {
                    b = board[y0-i][x0+i]; if (ksgn(b) == t) break;
                    moves[dep][n] = i*(+64-512) + m; n++; if (b) break;
                }
            }
        }

    if (dep == 0) //delete moves that would put king in check
    {
        for(y=0;y<8;y++)
            for(x=0;x<8;x++)
                if (board[y][x] == t*6) { kx = x; ky = y; goto foundking2; }
foundking2:;
        for(i=n-1;i>=0;i--)
        {
            move2xys(moves[dep][i],&x0,&y0,&x1,&y1);

            ocaststat = caststat; oprevmove = prevmove; patch0 = board[y0][x0]; patch1 = board[y1][x1];
            domove(board,&caststat,&prevmove,x0,y0,x1,y1,0);

            for(y=max(ky-1,0);y<=min(ky+1,7);y++)
                for(x=max(kx-2,0);x<=min(kx+2,7);x++)
                    if (board[y][x] == t*6)
                        if (ischeck(board,x,y,turn))
                        { n--; moves[0][i] = moves[0][n]; } //delete move - would put king in check

            undomove(board,x0,y0,x1,y1,patch0,patch1); caststat = ocaststat; prevmove = oprevmove;
        }
    }

    return(n);
}

static int isvalmove (int board[8][8], int caststat, int prevmove, int x0, int y0, int x1, int y1)
{
    int i, m, n, p;

    p = board[y0][x0]; if (p == 0) return(0);
    m = y1*512 + x1*64 + y0*8 + x0;
    n = getvalmoves(board,caststat,prevmove,p<0,0);
    for(i=n-1;i>=0;i--) if (moves[0][i] == m) return(1);
    return(0);
}

static int getcompmove_rec (int board[8][8], int *caststat, int *prevmove, int turn, int *bx0, int *by0, int *bx1, int *by1, int dep, int depmax, int obestsc)
{
    int i, j, k, sc, x0, y0, x1, y1, nx0, ny0, nx1, ny1, ocaststat, oprevmove, patch0, patch1, movesn, bsc;

    movesn = getvalmoves(board,*caststat,*prevmove,turn,dep); bsc = 0x80000001;

    for(i=movesn-1;i>=0;i--)
    {
        j = (rand()%(i+1)); k = moves[dep][j]; moves[dep][j] = moves[dep][i]; moves[dep][i] = k; //Shuffle
        move2xys(k,&x0,&y0,&x1,&y1);

        ocaststat = (*caststat); oprevmove = (*prevmove); patch0 = board[y0][x0]; patch1 = board[y1][x1];
        sc = domove(board,caststat,prevmove,x0,y0,x1,y1,0);
        if (dep < depmax)
            sc -= getcompmove_rec(board,caststat,prevmove,1-turn,&nx0,&ny0,&nx1,&ny1,dep+1,depmax,bsc-sc);
        undomove(board,x0,y0,x1,y1,patch0,patch1); (*caststat) = ocaststat; (*prevmove) = oprevmove;

        if (sc > bsc)
        {
            bsc = sc; if (!dep) { (*bx0) = x0; (*by0) = y0; (*bx1) = x1; (*by1) = y1; }
            if ((guseprune) && (sc >= -obestsc)) break; //alpha-beta prune
        }
    }
    return(bsc);
}

static int getcompmove (int board[8][8], int *caststat, int *prevmove, int turn, int *bx0, int *by0, int *bx1, int *by1, int depmax)
{
    int bsc;

    guseprune = (depmax >= 0); depmax = labs(depmax);
    bsc = getcompmove_rec(board,caststat,prevmove,turn,bx0,by0,bx1,by1,0,depmax-1,0x80000001);
    return(bsc != 0x80000001);
}

//--------------------------------------------------------------------------------------------------

static void drawchopper (float fx, float fy, float fz, float sc, float ha, float tim)
{
    static const char *ptroopnam[] = {"chopper.kv6","chopper_rotor.kv6","chopper_tail_rotor.kv6"};
    point3d pp, rr, dd, ff;
    float c, s, f, g;
    int i;

    c = cos(ha); s = sin(ha);
    for(i=0;i<3;i++)
    {
        if (i == 0)
        {
            f = sc*0.5f;
            rr.x =   f; rr.y = 0.f; rr.z = 0.f;
            dd.x = 0.f; dd.y =   f; dd.z = 0.f;
            ff.x = 0.f; ff.y = 0.f; ff.z =   f;
            pp.x = 0.f; pp.y = sc*.02f; pp.z = sc*.07f;
        }
        if (i == 1)
        {
            f = sc*0.5f; g = tim*32.f;
            rr.x = cos(g)*f; rr.y = sin(g)*f; rr.z = 0.f;
            dd.x =-sin(g)*f; dd.y = cos(g)*f; dd.z = 0.f;
            ff.x =      0.f; ff.y =      0.f; ff.z =   f;
            pp.x = 0.f; pp.y = sc*-.015f; pp.z = sc*.07f;
            f = 0.08f; pp.x += dd.x*f; pp.y += dd.y*f; pp.z += dd.z*f;
        }
        if (i == 2)
        {
            f = sc*0.125f; g = tim*16.f;
            rr.x =   f; rr.y =      0.f; rr.z =     0.f;
            dd.x = 0.f; dd.y = cos(g)*f; dd.z = sin(g)*f;
            ff.x = 0.f; ff.y =-sin(g)*f; ff.z = cos(g)*f;
            pp.x = 0.f; pp.y = sc*.32f; pp.z = sc*-.03f;
            f = 4.10f; pp.x -= dd.x*f; pp.y -= dd.y*f; pp.z -= dd.z*f;
            f = -1.54f; pp.x -= ff.x*f; pp.y -= ff.y*f; pp.z -= ff.z*f;
        }

        f = rr.x*c - rr.y*s; rr.y = rr.y*c + rr.x*s; rr.x = f;
        f = dd.x*c - dd.y*s; dd.y = dd.y*c + dd.x*s; dd.x = f;
        f = ff.x*c - ff.y*s; ff.y = ff.y*c + ff.x*s; ff.x = f;
        f = pp.x*c - pp.y*s; pp.y = pp.y*c + pp.x*s; pp.x = f;
        pp.x += fx; pp.y += fy; pp.z += fz;

        voxie_drawspr(&vf,ptroopnam[i],&pp,&rr,&dd,&ff,0xffffff);
    }
}

static void drawman (float fx, float fy, float fz, float sc, float ha, int ischute)
{
#define PTMAX 256
    poltex_t pt[PTMAX];
    float fz2, f, g;
    int i, n;

    voxie_drawsph(&vf,fx,fy,fz-sc*.02f,sc*.012,1,0xffffff); //Head
    for(f=-1.f;f<=1.f;f+=0.5f) //Arms
    {
        voxie_drawsph(&vf,fx+cos(ha)*sc*f*.018f,fy+sin(ha)*sc*f*.018f,fz,sc*.008f,1,0x00ffff);
    }
    voxie_drawcone(&vf,fx,fy,fz-sc*.02f,sc*.010f, //Torso
                   fx,fy,fz+sc*.02f,sc*.010f,1,0x00ffff);
    for(f=-1.f;f<=1.f;f+=2.f)
    {
        voxie_drawcone(&vf,fx                   ,fy                   ,fz+sc*.02f,sc*.008f, //Legs
                       fx+cos(ha)*sc*.018f*f,fy+sin(ha)*sc*.018f*f,fz+sc*.04f,sc*.008f,1,0x00ffff);
    }
    if (ischute)
    {
        voxie_drawcone(&vf,fx-sc*.018f,fy,fz-sc*.02f,sc*.008f,
                       fx-sc*.040f,fy,fz-sc*.08f,sc*.008f,1,0xff00ff);
        voxie_drawcone(&vf,fx+sc*.018f,fy,fz-sc*.02f,sc*.008f,
                       fx+sc*.040f,fy,fz-sc*.08f,sc*.008f,1,0xff00ff);

        n = 4096; g = 2.f/(float)n; fz2 = g*.5f-1.f;
        n = ((n*6)>>4);
        for(i=0;i<n;i++,fz2+=g)
        {
            f = sqrt(1.f-fz2*fz2);
            pt[i&(PTMAX-1)].x = cos((double)i*(sqrt(5.0)-1.0)*PI)*f*sc*.07f + fx;
            pt[i&(PTMAX-1)].y = sin((double)i*(sqrt(5.0)-1.0)*PI)*f*sc*.07f + fy;
            pt[i&(PTMAX-1)].z = fz2*sc*.07f - .02f + fz;
            pt[i&(PTMAX-1)].col = 0xffffff;
            if ((i&(PTMAX-1)) == PTMAX-1) voxie_drawmeshtex(&vf,0,&pt[0],PTMAX,0,0,0,0xffffff);
        }
        if (i&(PTMAX-1)) voxie_drawmeshtex(&vf,0,&pt[0],i&(PTMAX-1),0,0,0,0xffffff);

    }
}

//--------------------------------------------------------------------------------------------------
static char mungetboard (mun_t *cmun, int x, int y, int z, char defchar)
{
    if (!cmun->xwrap) { if ((unsigned)x >= (unsigned)cmun->xsiz) return(defchar); } else { x %= cmun->xsiz; if (x < 0) x += cmun->xsiz; }
    if (!cmun->ywrap) { if ((unsigned)y >= (unsigned)cmun->ysiz) return(defchar); } else { y %= cmun->ysiz; if (y < 0) y += cmun->ysiz; }
    if (!cmun->zwrap) { if ((unsigned)z >= (unsigned)cmun->zsiz) return(defchar); } else { z %= cmun->zsiz; if (z < 0) z += cmun->zsiz; }
    return(cmun->board[(z*cmun->ysiz + y)*cmun->xsiz + x]);
}

static void drawlin (float x, float y, float z, float x0, float y0, float z0, float x1, float y1, float z1, int dir, int col)
{
    float t;
    int i;

    if (dir&1) { x0 *= -1.f; x1 *= -1.f; }
    if (dir&2) { y0 *= -1.f; y1 *= -1.f; }
    if (dir&4) { z0 *= -1.f; z1 *= -1.f; }
    i = (dir>>3);
    if (i == 1) { t = x0; x0 = y0; y0 = t; t = x1; x1 = y1; y1 = t; }
    if (i == 2) { t = x0; x0 = z0; z0 = t; t = x1; x1 = z1; z1 = t; }
    if (i == 3) { t = y0; y0 = z0; z0 = t; t = y1; y1 = z1; z1 = t; }
    if (i == 4) { t = x0; x0 = y0; y0 = z0; z0 = t; t = x1; x1 = y1; y1 = z1; z1 = t; }
    if (i == 5) { t = x0; x0 = z0; z0 = y0; y0 = t; t = x1; x1 = z1; z1 = y1; y1 = t; }
    voxie_drawlin(&vf,x0+x,y0+y,z0+z,x1+x,y1+y,z1+z,col);
}

static void drawcyl (float x, float y, float z, float r, int col, int vis, int n2, int nlin /*should be multiple of 4*/)
{
    static const char dirlut[64] =
    {
        -1, 0, 1, 0, 8,22,18,28,
        9,20,16,24, 8,44,40,24,
        16,14,13, 7,30, 0, 1, 4,
        26, 2, 3, 7,12,12,14,24,
        17,10, 8, 0, 2, 4, 5, 0,
        0, 6, 7, 2, 8, 8,10,26,
        16,20,16, 7,36,20,16, 0,
        32,22,18, 2, 8, 8,10,-1
    };
    static const char dirlut2[8][3] =
    {
        7,15,31, 6,13,30, 5,14,27, 4,12,26,
        3,11,29, 2, 9,28, 1,10,25, 0, 8,24,
    };
    float f, g, c, s, c0, s0, c1, s1;
    int i, j, k, m, m0, m1, n, dir;

    n = 0; for(i=0;i<6;i++) { if (vis&(1<<i)) n++; }

    //64 cases
    dir = dirlut[vis];
    if (n == 0) { } //1 null
    else if (n == 1) //6 dead ends
    {
        for(i=0;i<nlin;i++)
        {
            c = cos((float)i*PI*2.0/(float)nlin)*r;
            s = sin((float)i*PI*2.0/(float)nlin)*r;
            for(j=0;j<n2*6/6;j++)
            {
                c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/(float)n2);
                s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/(float)n2);
                drawlin(x,y,z,r*s0,c*c0,s*c0,r*s1,c*c1,s*c1,dir,col);
            }
            drawlin(x,y,z,0,c,s,-r,c,s,dir,col);
        }
    }
    else if (n == 2)
    {
        if ((vis == 3) || (vis == 12) || (vis == 48)) //3 axes
        {
            for(i=0;i<nlin;i++)
            {
                c = cos((float)i*PI*2.0/(float)nlin)*r;
                s = sin((float)i*PI*2.0/(float)nlin)*r;
                drawlin(x,y,z,-r,c,s,r,c,s,dir,col);
            }
        }
        else //12 L-shaped
        {
            for(i=0;i<nlin;i++)
            {
                c = cos((float)i*PI*2.0/(float)nlin)*r;
                s = sin((float)i*PI*2.0/(float)nlin)*r; s += r;
                for(j=0;j<n2;j++)
                {
                    c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/(float)n2);
                    s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/(float)n2);
                    drawlin(x,y,z,c,r-s*c0,r-s*s0,
                            c,r-s*c1,r-s*s1,dir,col);
                }
            }
        }
    }
    else if (n == 3)
    {
        //8 corners
        if ((vis == 21) || (vis == 22) || (vis == 25) || (vis == 26) ||
                (vis == 37) || (vis == 38) || (vis == 41) || (vis == 42))
        {
            m = dirlut[vis];
            for(i=(int)(nlin*3/8);i<=nlin*7/8;i++)
            {
                c = cos((float)i*PI*2.0/(float)nlin)*r; c += r;
                s = sin((float)i*PI*2.0/(float)nlin)*r;
                for(j=0;j<n2;j++)
                {
                    c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/(float)n2);
                    s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/(float)n2);
                    for(k=3-1;k>=0;k--)
                        drawlin(x,y,z,r-c*c0,s,r-c*s0,
                                r-c*c1,s,r-c*s1,dirlut2[m][k],col);
                }
            }
        }
        else //12 half plusses
        {
            for(i=nlin/2+1;i<nlin;i++)
            {
                c = cos((float)i*PI*2.0/(float)nlin)*r;
                s = sin((float)i*PI*2.0/(float)nlin)*r;
                drawlin(x,y,z,-r,c,s,r,c,s,dir,col);
            }
            for(i=nlin/4;i<=nlin*3/4;i++)
            {
                c = cos((float)i*PI*2.0/(float)nlin)*r; c += r;
                s = sin((float)i*PI*2.0/(float)nlin)*r;
                for(j=0;j<n2;j++)
                {
                    c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/n2);
                    s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/n2);
                    for(f=-1;f<=1;f+=2)
                        drawlin(x,y,z,(r-c*c0)*f,s,r-c*s0,
                                (r-c*c1)*f,s,r-c*s1,dir,col);
                }
            }
        }
    }
    else if (n == 4)
    {
        if ((vis == 15) || (vis == 51) || (vis == 60)) //3 plusses
        {
            for(i=nlin/4;i<=nlin*3/4;i++)
            {
                c = cos((float)i*PI*2.0/(float)nlin)*r; c += r;
                s = sin((float)i*PI*2.0/(float)nlin)*r;
                for(j=0;j<n2;j++)
                {
                    c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/(float)n2);
                    s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/(float)n2);
                    for(g=-1.f;g<=1.f;g+=2.f)
                        for(f=-1.f;f<=1.f;f+=2.f)
                            drawlin(x,y,z,(r-c*c0)*f,s,(r-c*s0)*g,
                                    (r-c*c1)*f,s,(r-c*s1)*g,dir,col);
                }
            }
        }
        else //12 corners with 1 extra
        {
            for(i=((nlin*3)>>2);i<nlin;i++)
            {
                c = cos((float)i*PI*2.0/(float)nlin)*r;
                s = sin((float)i*PI*2.0/(float)nlin)*r;
                drawlin(x,y,z,-r,c,s,r,c,s,dir,col);
            }
            for(i=nlin/4;i<=nlin*2/4;i++)
            {
                c = cos((float)i*PI*2.0/(float)nlin)*r; c += r;
                s = sin((float)i*PI*2.0/(float)nlin)*r;
                for(j=0;j<n2;j++)
                {
                    c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/(float)n2);
                    s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/(float)n2);
                    for(f=-1.f;f<=1.f;f+=2.f)
                    {
                        drawlin(x,y,z,(r-c*c0)*f,s,r-c*s0,
                                (r-c*c1)*f,s,r-c*s1,dir,col);
                        drawlin(x,y,z,(r-c*c0)*f,-(r-c*s0),-s,
                                (r-c*c1)*f,-(r-c*s1),-s,dir,col);
                    }
                }
            }
            if (vis == 23) { m0 = 0; m1 = 1; }
            if (vis == 27) { m0 = 3; m1 = 2; }
            if (vis == 29) { m0 = 0; m1 = 2; }
            if (vis == 30) { m0 = 3; m1 = 1; }
            if (vis == 39) { m0 = 4; m1 = 5; }
            if (vis == 43) { m0 = 6; m1 = 7; }
            if (vis == 45) { m0 = 4; m1 = 6; }
            if (vis == 46) { m0 = 5; m1 = 7; }
            if (vis == 53) { m0 = 0; m1 = 4; }
            if (vis == 54) { m0 = 1; m1 = 5; }
            if (vis == 57) { m0 = 2; m1 = 6; }
            if (vis == 58) { m0 = 3; m1 = 7; }
            for(i=nlin*4/8;i>=nlin*3/8;i--)
            {
                c = cos((float)i*PI*2.0/(float)nlin)*r; c += r;
                s = sin((float)i*PI*2.0/(float)nlin)*r;
                for(j=0;j<n2;j++)
                {
                    c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/(float)n2);
                    s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/(float)n2);
                    for(m=0;m<2;m++)
                        for(k=3-1;k>=0;k--)
                            drawlin(x,y,z,r-c*c0,s,r-c*s0,
                                    r-c*c1,s,r-c*s1,dirlut2[(m1-m0)*m+m0][k],col);
                }
            }
        }
    }
    else if (n == 5) //6 half planes
    {
        for(i=nlin/4;i<=nlin*2/4;i++)
        {
            c = cos((float)i*PI*2.0/(float)nlin)*r; c += r;
            s = sin((float)i*PI*2.0/(float)nlin)*r;
            for(j=0;j<n2;j++)
            {
                c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/(float)n2);
                s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/(float)n2);
                for(g=-1.f;g<=1.f;g+=2.f)
                    for(f=-1.f;f<=1.f;f+=2.f)
                        drawlin(x,y,z,(r-c*c0)*f,s,(r-c*s0)*g,
                                (r-c*c1)*f,s,(r-c*s1)*g,dir,col);
            }
        }
        static int mv[4];
        if (vis == 31) { mv[0] = 0; mv[1] = 1; mv[2] = 2; mv[3] = 3; }
        if (vis == 47) { mv[0] = 4; mv[1] = 5; mv[2] = 6; mv[3] = 7; }
        if (vis == 55) { mv[0] = 0; mv[1] = 1; mv[2] = 4; mv[3] = 5; }
        if (vis == 59) { mv[0] = 2; mv[1] = 3; mv[2] = 6; mv[3] = 7; }
        if (vis == 61) { mv[0] = 0; mv[1] = 2; mv[2] = 4; mv[3] = 6; }
        if (vis == 62) { mv[0] = 1; mv[1] = 3; mv[2] = 5; mv[3] = 7; }
        for(i=nlin*4/8;i>=nlin*3/8;i--)
        {
            c = cos((float)i*PI*2.0/(float)nlin)*r; c += r;
            s = sin((float)i*PI*2.0/(float)nlin)*r;
            for(j=0;j<n2;j++)
            {
                c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/(float)n2);
                s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/(float)n2);
                for(m=0;m<4;m++)
                    for(k=3-1;k>=0;k--)
                        drawlin(x,y,z,r-c*c0,s,r-c*s0,
                                r-c*c1,s,r-c*s1,dirlut2[mv[m]][k],col);
            }
        }
    }
    else if (n == 6) //1 full axes
    {
        for(i=nlin*4/8;i>=nlin*3/8;i--)
        {
            c = cos((float)i*PI*2.0/(float)nlin)*r; c += r;
            s = sin((float)i*PI*2.0/(float)nlin)*r;
            for(j=0;j<n2;j++)
            {
                c0 = cos((float)j*PI/2.0/(float)n2); c1 = cos((float)(j+1)*PI/2.0/(float)n2);
                s0 = sin((float)j*PI/2.0/(float)n2); s1 = sin((float)(j+1)*PI/2.0/(float)n2);
                for(m=0;m<8;m++)
                    for(k=3-1;k>=0;k--)
                        drawlin(x,y,z,r-c*c0,s,r-c*s0,
                                r-c*c1,s,r-c*s1,dirlut2[m][k],col);
            }
        }
    }
}

static void drawbird (float x, float y, float z, float hang, float vang, float tang, float sc, int col)
{
    point3d pp, rr, dd, ff;
    float g;
    int i;

    for(i=-2;i<=2;i++)
    {
        pp.x = x;
        pp.y = y;
        pp.z = z;
        rr.x =-sin(hang); ff.x = cos(hang)*cos(vang);
        rr.y = cos(hang); ff.y = sin(hang)*cos(vang);
        rr.z = 0;         ff.z =           sin(vang);
        dd.x = ff.y*rr.z - ff.z*rr.y;
        dd.y = ff.z*rr.x - ff.x*rr.z;
        dd.z = ff.x*rr.y - ff.y*rr.x;

        rotate_vex(tang*(float)i,&rr,&dd);

        switch(labs(i))
        {
        case 0: g = .80; break;
        case 1: g = .18; break;
        case 2: pp.z += tang*-.40*sc;
            g = fabs(sin(tang))*.02*sc;
            pp.x += rr.x*g*(float)i;
            pp.y += rr.y*g*(float)i;
            pp.z += rr.z*g*(float)i;
            g = .30; break;
        }

        g *= sc;

        if (i > 0) { rr.x = -rr.x; rr.y = -rr.y; rr.z = -rr.z; }
        rr.x *= g; rr.y *= g; rr.z *= g;
        dd.x *= g; dd.y *= g; dd.z *= g;
        ff.x *= g; ff.y *= g; ff.z *= g;

        switch(labs(i))
        {
        case 0: voxie_drawspr(&vf, "torso.kv6",&pp,&rr,&dd,&ff,col); break;
        case 1: voxie_drawspr(&vf,"wingl1.kv6",&pp,&rr,&dd,&ff,col); break;
        case 2: voxie_drawspr(&vf,"wingl2.kv6",&pp,&rr,&dd,&ff,col); break;
        }
    }
}

static float dist2plat2 (float px, float py, float pz, float x, float y, float z, float hx, float hy)
{
    float d, f, xx, yy, zz, hz;

    hz = 0.02f;
    xx = min(max(px,x-hx),x+hx);
    yy = min(max(py,y-hy),y+hy);
    zz = min(max(pz,z-hz),z+hz);
    d = (px-xx)*(px-xx) + (py-yy)*(py-yy) + (pz-zz)*(pz-zz);
    if (pz > z)
    {
        f = max(sqrt((px-x)*(px-x) + (py-y)*(py-y))-.05f,0.f);
        d = min(d,f*f);
    }
    return(d);
}



static void drawcube_thickwire (voxie_frame_t *vf, float x0, float y0, float z0, float x1, float y1, float z1, float rad, int col)
{
    int x, y;

    for(y=0;y<=1;y++)
        for(x=0;x<=1;x++)
        {
            voxie_drawcone(vf,(x1-x0)*x+x0,(y1-y0)*y+y0,z0,rad,
                           (x1-x0)*x+x0,(y1-y0)*y+y0,z1,rad,1,col);
            voxie_drawcone(vf,(x1-x0)*x+x0,y0,(z1-z0)*y+z0,rad,
                           (x1-x0)*x+x0,y1,(z1-z0)*y+z0,rad,1,col);
            voxie_drawcone(vf,x0,(y1-y0)*x+y0,(z1-z0)*y+z0,rad,
                           x1,(y1-y0)*x+y0,(z1-z0)*y+z0,rad,1,col);
        }
}

//--------------------------------------------------------------------------------------------\/
#if (USELEAP != 0)
//Based on leap3/leaptest.c (12/01/2017)
#pragma comment(lib,"c:/LeapSDK/lib/x64/LeapC.lib")
#include "LeapC.h"
#include "Leap_Voxon.h"
#include "VLInteract.h"
#define PI 3.14159265358979323

// Import function from LeapC.dll
static LEAP_CONNECTION leap_con;
static LEAP_TRACKING_EVENT leap_frame[2] = {0,0}; //FIFO for thread safety
static int leap_framecnt = 0, leap_iscon = 0, leap_isrun = 0;
static void leap_thread (void *_)
{
    LEAP_CONNECTION_MESSAGE msg;
    eLeapRS r;
    while (leap_isrun)
    {
        r = LeapPollConnection(leap_con,1000,&msg); if (r != eLeapRS_Success) continue;
        switch (msg.type)
        {
        case eLeapEventType_Tracking: memcpy(&leap_frame[leap_framecnt&1],msg.tracking_event,sizeof(LEAP_TRACKING_EVENT)); leap_framecnt++; break;
        case eLeapEventType_Connection:     leap_iscon = 1; break;
        case eLeapEventType_ConnectionLost: leap_iscon = 0; break;
        default: break;
        }
    }
}
void leap_uninit (void) { LeapDestroyConnection(leap_con); leap_isrun = 0; }
LEAP_CONNECTION *leap_init (void)
{
    eLeapRS r;
    r = LeapCreateConnection(0,&leap_con); if (r != eLeapRS_Success) return(&leap_con);
    r = LeapOpenConnection(leap_con);      if (r != eLeapRS_Success) return(&leap_con);
    leap_isrun = 1; _beginthread(leap_thread,0,0);
    while (!leap_iscon) Sleep(15);
    return(&leap_con);
}
LEAP_TRACKING_EVENT *leap_getframe (void) { return(&leap_frame[leap_framecnt&1]); }


#endif
//--------------------------------------------------------------------------------------------------


static int gcnti[2], gbstat = 0;
static void mymix (int *ibuf, int nsamps)
{
    static int cnt[2]; int i, c;
    for(i=0;i<nsamps;i++,ibuf+=vw.playnchans)
        for(c=min(vw.playnchans,2)-1;c>=0;c--)
        { ibuf[c] = ((cnt[c]&(1<<20))-(1<<19))&gbstat; cnt[c] += gcnti[c]; }
}

//Rotate vectors a & b around their common plane, by ang
static void rotvex (float ang, point3d *a, point3d *b)
{
    float f, c, s;
    int i;

    c = cos(ang); s = sin(ang);
    f = a->x; a->x = f*c + b->x*s; b->x = b->x*c - f*s;
    f = a->y; a->y = f*c + b->y*s; b->y = b->y*c - f*s;
    f = a->z; a->z = f*c + b->z*s; b->z = b->z*c - f*s;
}


/************************************************/
/* adding Leap motion device, by Yi-Ting, Hsieh */
// Count grabbing time
static float countGrab = 0.0;
// For trigger gslicemode
static bool HandPinched = false;
static bool callExit = false;
// For Draw hands
static void drawHands(LEAP_HAND *hand, float LeapScale, int rendmode)
{
    float f;
    int j, k;
    LEAP_VECTOR *vec, *vec1;
    LEAP_PALM *palm;
    LEAP_VECTOR *normPalmVec;
    LEAP_DIGIT *digit;
    point3d pp, rr, dd, ff, pos = {0.0,0.0,0.0}, palmPos;
    point3d fing[5], ofing[5];
    point3d prefProximal[5];
    f = .03f;


    /* detect hands and draw fingers */
    // Draw palm
    palm = &hand->palm;
    vec = &palm->position;
    normPalmVec = &palm->normal;

    // Based on the rendmode to draw hands
    if (rendmode==0)
    {
        palmPos.x = LeapScale*vec->x*+.01f;		palmPos.x = min(max(palmPos.x,-vw.aspx),vw.aspx);
        palmPos.y = LeapScale*vec->z*+.01f;		palmPos.y = min(max(palmPos.y,-vw.aspy),vw.aspy);
        palmPos.z = LeapScale*vec->y*-.01f+1.5f;	palmPos.z = min(max(palmPos.z,-vw.aspz),vw.aspz);

        voxie_drawsph(&vf,palmPos.x,palmPos.y,palmPos.z,f,0,0xffffff);

        // Draw fingers: thumb, index, middle, ring and pinky
        for(j=0;j<5;j++)
        {
            digit = &hand->digits[j];
            //if(!digit->is_extended) continue;

            // Walk through the 4 bones and draw it: Metacarpal, Proximal, Intermediate and Distal
            for(k=0;k<4;k++)
            {
                vec = &digit->bones[k].next_joint;
                //vec = &digit->stabilized_tip_position;
                fing[j].x = LeapScale*vec->x*+.01f;	fing[j].x = min(max(fing[j].x,-vw.aspx),vw.aspx);
                fing[j].y = LeapScale*vec->z*+.01f;	fing[j].y = min(max(fing[j].y,-vw.aspy),vw.aspy);
                fing[j].z = LeapScale*vec->y*-.01f+1.5f;fing[j].z = min(max(fing[j].z,-vw.aspz),vw.aspz);

                voxie_drawsph(&vf,fing[j].x,fing[j].y,fing[j].z,f,0,0xffffff);
            }
        }
    }

    // Use line to draw hands
    else if (rendmode == 1)
    {
        /* detect hands and draw fingers */
        // Draw palm
        f = .03f;
        palmPos.x = LeapScale*vec->x*+.01f;		palmPos.x = min(max(palmPos.x,-vw.aspx),vw.aspx);
        palmPos.y = LeapScale*vec->z*+.01f;		palmPos.y = min(max(palmPos.y,-vw.aspy),vw.aspy);
        palmPos.z = LeapScale*vec->y*-.01f+1.5f;	palmPos.z = min(max(palmPos.z,-vw.aspz),vw.aspz);
        voxie_drawsph(&vf,palmPos.x,palmPos.y,palmPos.z,f,0,0xffffff);
        //voxie_drawcone(&vf, palmPos.x,palmPos.y,palmPos.z,.1f,palmPos.x,palmPos.y,palmPos.z,.1f,0,0xffffff);
        double angle, x1, y1, z1;
        float itr;
        float plane;
        float projAngle;

        plane = normPalmVec->x * palmPos.x + normPalmVec->y * palmPos.y + normPalmVec->z * palmPos.z;
        projAngle = (normPalmVec->x * 0.2 + normPalmVec->y * 0.2 + normPalmVec->z * 0) /
                (sqrtf((normPalmVec->x)*(normPalmVec->x) + (normPalmVec->y)*(normPalmVec->y) + (normPalmVec->z)*(normPalmVec->z))*sqrtf(0.2*0.2+0.2*0.2+0*0));
        for (itr = 0; itr < 360; itr += 0.1) {
            angle = itr;
            x1 = 0.2 * cos(angle * PI / 180);
            y1 = 0.2 * sin(angle * PI / 180);
            z1 = 0;
            voxie_drawsph(&vf,palmPos.x+x1,palmPos.y+y1, palmPos.z+z1, .01f, 0, 0xffffff);
        }
        // for (i = 0; i < 360; i += 0.1) {
        // 	angle = i;
        // 	x1 = palmPos.x  + (0.2 * cos(angle * PI / 180));		x1 = x1 * cos(projAngle);
        // 	y1 = palmPos.y + (0.2 * sin(angle * PI / 180));			y1 = y1 * cos(projAngle);
        // 	z1 = plane - x1 - y1;
        // 	voxie_drawsph(&vf,x1,y1,z1, .01f, 0, 0xffffff);
        // }


        // Draw fingers
        bool checkExits[5] = { false, false, false, false, false };
        for(j=0;j<5;j++)
        {
            digit = &hand->digits[j];
            //if(!digit->is_extended) continue;
            //30.09.18   Check exit gesture
            if (digit->is_extended)
                checkExits[j] = true;

            // Walk through the 4 bones and draw it: Metacarpal, Proximal, Intermediate and Distal
            for(k=2;k<4;k++)
            {
                vec = &digit->bones[k].next_joint;
                vec1 = &digit->bones[k].prev_joint;
                //vec = &digit->stabilized_tip_position;

                fing[j].x = LeapScale*vec->x*+.01f;	fing[j].x = min(max(fing[j].x,-vw.aspx),vw.aspx);
                fing[j].y = LeapScale*vec->z*+.01f;	fing[j].y = min(max(fing[j].y,-vw.aspy),vw.aspy);
                fing[j].z = LeapScale*vec->y*-.01f+1.5f;fing[j].z = min(max(fing[j].z,-vw.aspz),vw.aspz);

                voxie_drawsph(&vf,fing[j].x,fing[j].y,fing[j].z,.04,0,0xffffff);

                ofing[j].x = LeapScale*vec1->x*+.01f;	ofing[j].x = min(max(ofing[j].x,-vw.aspx),vw.aspx);
                ofing[j].y = LeapScale*vec1->z*+.01f;	ofing[j].y = min(max(ofing[j].y,-vw.aspy),vw.aspy);
                ofing[j].z = LeapScale*vec1->y*-.01f+1.5f;ofing[j].z = min(max(ofing[j].z,-vw.aspz),vw.aspz);
                voxie_drawsph(&vf,ofing[j].x,ofing[j].y,ofing[j].z,.04,0,0xffffff);

                //voxie_drawlin(&vf,-vw.aspx,-vw.aspy,-vw.aspz,vw.aspx,vw.aspy,vw.aspz,0x010101);
                voxie_drawlin(&vf, fing[j].x,fing[j].y,fing[j].z,ofing[j].x,ofing[j].y,ofing[j].z,0xffffff);
                //	voxie_drawcone(&vf, fing[j].x,fing[j].y,fing[j].z,f,ofing[j].x,ofing[j].y,ofing[j].z,f,0,0xffffff);

            }
        }
    }

    // Use Cylinder to draw hands
    else if (rendmode == 2)
    {
        point3d thumb;
        /* detect hands and draw fingers */
        // Draw palm
        f = .03f;
        palmPos.x = LeapScale*vec->x*+.01f;		palmPos.x = min(max(palmPos.x,-vw.aspx),vw.aspx);
        palmPos.y = LeapScale*vec->z*+.01f;		palmPos.y = min(max(palmPos.y,-vw.aspy),vw.aspy);
        palmPos.z = LeapScale*vec->y*-.01f+1.5f;	palmPos.z = min(max(palmPos.z,-vw.aspz),vw.aspz);
        //voxie_drawsph(&vf,palmPos.x,palmPos.y,palmPos.z,f,0,0xffffff);
        //voxie_drawcone(&vf, palmPos.x,palmPos.y,palmPos.z,.1f,palmPos.x,palmPos.y,palmPos.z,.1f,0,0xffffff);

        // Draw fingers
        for(j=0;j<5;j++)
        {
            digit = &hand->digits[j];
            //if(!digit->is_extended) continue;

            // Walk through the 4 bones and draw it: Metacarpal, Proximal, Intermediate and Distal
            for(k=0;k<4;k++)
            {
                vec = &digit->bones[k].next_joint;
                vec1 = &digit->bones[k].prev_joint;
                //vec = &digit->stabilized_tip_position;

                fing[j].x = LeapScale*vec->x*+.01f;	fing[j].x = min(max(fing[j].x,-vw.aspx),vw.aspx);
                fing[j].y = LeapScale*vec->z*+.01f;	fing[j].y = min(max(fing[j].y,-vw.aspy),vw.aspy);
                fing[j].z = LeapScale*vec->y*-.01f+1.5f;fing[j].z = min(max(fing[j].z,-vw.aspz),vw.aspz);

                //voxie_drawsph(&vf,fing[j].x,fing[j].y,fing[j].z,.04,0,0xffffff);

                ofing[j].x = LeapScale*vec1->x*+.01f;	ofing[j].x = min(max(ofing[j].x,-vw.aspx),vw.aspx);
                ofing[j].y = LeapScale*vec1->z*+.01f;	ofing[j].y = min(max(ofing[j].y,-vw.aspy),vw.aspy);
                ofing[j].z = LeapScale*vec1->y*-.01f+1.5f;ofing[j].z = min(max(ofing[j].z,-vw.aspz),vw.aspz);

                if((k==0 && (j>=1 && j <=3))){ ; } // do nothing
                else {
                    voxie_drawcone(&vf, fing[j].x,fing[j].y,fing[j].z,f,ofing[j].x,ofing[j].y,ofing[j].z,f,0,0xffffff);
                    if (j==0 && k==0) { thumb.x = ofing[j].x, thumb.y = ofing[j].y, thumb.z = ofing[j].z;}
                    if (j==4 && k==0) { voxie_drawcone(&vf, thumb.x,thumb.y,thumb.z,f,ofing[j].x,ofing[j].y,ofing[j].z,f,0,0xffffff); }
                }


                if(k==1)
                {
                    if(j==0)
                    {
                        prefProximal[j].x = fing[j].x;
                        prefProximal[j].y = fing[j].y;
                        prefProximal[j].z = fing[j].z;
                    }
                    else
                    {
                        prefProximal[j].x = ofing[j].x;
                        prefProximal[j].y = ofing[j].y;
                        prefProximal[j].z = ofing[j].z;
                    }
                    if(j>=1) { voxie_drawcone(&vf,prefProximal[j-1].x,prefProximal[j-1].y,prefProximal[j-1].z,f,ofing[j].x,ofing[j].y,ofing[j].z,f,0,0xffffff); }
                }
            }
        }
    }

}

// Tracking index position for selecting application
static point3d indexPos;
static point3d rawIndexPos;

static void getIndexPos(LEAP_HAND *hand, float LeapScale, int rendmode,float r)
{
    float f;
    f = .03f;
    LEAP_VECTOR *vec;
    LEAP_DIGIT *digit;
    point3d fing;
    point3d pp, rr, dd, ff;

    if (hand == nullptr) {
      indexPos.x = 0;       indexPos.y = 0;       indexPos.z = 0;
      return;
    }

    // Read the index finger
    digit = &hand->digits[1];

    // Check the tip of Distal on index
    vec = &digit->bones[3].next_joint;
    fing.x = LeapScale*vec->x*+.01f;	fing.x = min(max(fing.x,-vw.aspx),vw.aspx);
    fing.y = LeapScale*vec->z*+.01f;	fing.y = min(max(fing.y,-vw.aspy),vw.aspy);
    fing.z = LeapScale*vec->y*-.01f+1.5f;fing.z = min(max(fing.z,-vw.aspz),vw.aspz);

    voxie_drawsph(&vf,fing.x,fing.y,fing.z,f,0,0xffffff);

    // draw the selected size
    // rr.x = 0.39f; dd.x = 0.00f; ff.x = 0.00f; pp.x = ((float)fing.x)-1/2*rr.x;
    // rr.y = 0.00f; dd.y = 0.39f; ff.y = 0.00f; pp.y = ((float)fing.y)-1/2*rr.y;
    // rr.z = 0.00f; dd.z = 0.00f; ff.z = 0.39f; pp.z = ((float)fing.z)-1/2*rr.z;
    // pp.x += (rr.x + dd.x + ff.x)*-0.5f;
    // pp.y += (rr.y + dd.y + ff.y)*-0.5f;
    // pp.z += (rr.z + dd.z + ff.z)*-0.5f;
    // voxie_drawcube(&vf,&pp,&rr,&dd,&ff,1,0xffffff);

    indexPos = fing;

    voxie_drawcone(&vf,indexPos.x,indexPos.y,indexPos.z, .05f,indexPos.x,indexPos.y,indexPos.z + 0.15, .01f, 0, 0xffffff);

}

// For VLInteract
static bool checkExitBox = false;
static void drawExitBox(LEAP_HAND *handl, float LeapScale, float r)
{
    float f;
    f = .03f;
    LEAP_VECTOR *vec;
    LEAP_BONE *arm;
    point3d exitBoxP;
    point3d pp, rr, dd, ff;

    // Find out the arm position on left hand
    arm = &handl->arm;
    vec = &arm->next_joint;
    exitBoxP.x = LeapScale*vec->x*+.01f;			exitBoxP.x = min(max(exitBoxP.x,-vw.aspx),vw.aspx);
    exitBoxP.y = LeapScale*vec->z*+.01f;			exitBoxP.y = min(max(exitBoxP.y,-vw.aspy),vw.aspy);
    exitBoxP.z = LeapScale*vec->y*-.01f+1.5f;	exitBoxP.z = min(max(exitBoxP.z,-vw.aspz),vw.aspz);

    // draw the selected size
    rr.x = r; dd.x = 0.00f; ff.x = 0.00f; pp.x = ((float)exitBoxP.x)+rr.x;
    rr.y = 0.00f; dd.y = r; ff.y = 0.00f; pp.y = ((float)exitBoxP.y)-1/2*rr.y;
    rr.z = 0.00f; dd.z = 0.00f; ff.z =r; pp.z = ((float)exitBoxP.z)-1/2*rr.z;
    pp.x += (rr.x + dd.x + ff.x)*-0.5f;
    pp.y += (rr.y + dd.y + ff.y)*-0.5f;
    pp.z += (rr.z + dd.z + ff.z)*-0.5f;
    voxie_drawcube(&vf,&pp,&rr,&dd,&ff,1,0xffffff);

    // Check whether the right hand index finger point touch the exit box
    if((indexPos.x >= pp.x-1.3*r && indexPos.x <= pp.x+1.3*r)
            &&(indexPos.y >= pp.y-1.3*r && indexPos.y <= pp.y+1.3*r))
        // &&(indexPos.z >= pp.z-1.4*r && indexPos.z <= pp.z+1.4*r))
    {
        checkExitBox = true;
        // voxie_uninit(0);
    }
    else { if (checkExitBox) checkExitBox = false; }

    pp.x = pp.x + .03f;
    pp.y = pp.y + .03f;
    pp.z = pp.z + .03f;
    voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"E");
}

static bool prevEventLast = false;
float menuRotateAngle = 0.0;

/* For MenuBox: 29.08.18 */
enum State { UNPRESSED, PRESSED };
static int DELAYTIME = 3;
struct MenuBox
{
    int status;
    int timer;
    int delayTimer;
    bool fromTop;
};


static void execute_status0(point3d p, point3d r, point3d d, point3d f, LEAP_HAND *handr,float LeapScale, struct MenuBox* menubox)
{

    voxie_drawspr(&vf,iconnam[0],&p,&r,&d,&f,0x707070 + 0x101010);

    if ((*menubox).delayTimer > 0)
    {
        (*menubox).delayTimer--;
    }
    else
    {
        (*menubox).timer = 0;
        (*menubox).status = 0;
        (*menubox).delayTimer = 0;
    }

    if ((*menubox).delayTimer == 0)
    {
        if ((indexPos.x >= p.x-1.0f*r.x && indexPos.x <= p.x+1.0f*r.x)
                &&(indexPos.y >= p.y-1.0f*d.y && indexPos.y <= p.y+1.0f*d.y)
                // &&(indexPos.z >= p.z-0.6f*f.z+0.15 && indexPos.z <= p.z+0.6f*f.z+0.15))
                &&(indexPos.z >= p.z-0.8f*f.z && indexPos.z <= p.z+0.8f*f.z))
        {
            if ((*menubox).fromTop == false &&
                    indexPos.z <= p.z - 0.4 * f.z)
                // indexPos.z >= p.z + 0.2f * f.z)
            {
                (*menubox).fromTop = true;
            }
            else if ((*menubox).fromTop == true &&
                     // (((&handr->palm)->velocity).y <= -60 && 	indexPos.z <= p.z))
                     indexPos.z >= p.z + 0.3f * f.z)
                // indexPos.z <= p.z - 0.3 * f.z)
            {
                voxie_printalph_(&vf,&p,&r,&d,0xffffff,"VE");
                (*menubox).timer = 0;
                (*menubox).status = 1;
                (*menubox).delayTimer = DELAYTIME;
                (*menubox).fromTop = false;
                return;
            }
        }
        else
        {
            (*menubox).fromTop = false;
        }
    }

}

static void execute_status1_enter_game_folder(point3d p, point3d r, point3d d, point3d f, LEAP_HAND *handr,float LeapScale, struct MenuBox* menubox)
{
    // testing: drawing elements in circle:   25.09.18
    if ((*menubox).delayTimer > 0)
    {
        (*menubox).delayTimer--;
    }
    else
    {
        (*menubox).timer = 0;
        (*menubox).delayTimer = 0;
    }

    if ((*menubox).delayTimer == 0)
    {
        float angle, x1, y1, z1;
        int i, j;
        float length = (float) (r.x * 0.5f);

        p.x -= 0.25f;
        p.y -= 0.25f;

        float fx, fy, fn, fscale;
        fscale = 1.f/min(min(vw.aspx,vw.aspy),vw.aspz/.2f);

        bool hitIcon = false;
        for (i = 0; i < 360; i += 360/GAMENUMS)
        {
            angle = i;
            x1 = 0.5 * cos(angle * PI / 180);
            y1 = 0.5 * sin(angle * PI / 180);
            z1 = 0;

            p.x = ((float) p.x + x1);
            p.y = ((float) p.y + y1);
            p.z = ((float) p.z + z1);

            j = (i / (360 / GAMENUMS));

            // Draw normal icons
            fx = p.x, fy = p.y;

            if ((indexPos.x >= p.x-1.0*length && indexPos.x <= p.x+1.0*length)
                    &&(indexPos.y >= p.y-1.0*length && indexPos.y <= p.y+1.0*length))
            {
                hitIcon = true;
                // return to status 0: edited by 05.09.18
                // if (((&handr->palm)->velocity).y <= -60 && j == RENDMODE_TITLE &&
                //         indexPos.z <= p.z)
                if ((*menubox).fromTop == false &&
                        // (indexPos.z <= p.z+0.8f*f.z && indexPos.z >= p.z+0.3f*f.z))
                        indexPos.z <= p.z - 0.4 * f.z)
                {
                    (*menubox).fromTop = true;
                }

                else if ((*menubox).fromTop == true &&
                         // (((&handr->palm)->velocity).y <= -60 && indexPos.z <= p.z))
                         (indexPos.z <= p.z+0.8f*f.z && indexPos.z >= p.z+0.3f*f.z))
                    // indexPos.z <= p.z - 0.3 * f.z)
                {
                    if (j == GAMENUMS - 1) {
                        (*menubox).timer = 0;
                        (*menubox).status = 1;
                        (*menubox).delayTimer = DELAYTIME;
                        (*menubox).fromTop = false;
                        return;
                    }

                    grendmode = j + MENUGAMESTART;
                    (*menubox).timer = 0;
                    (*menubox).status = 0;
                    (*menubox).delayTimer = DELAYTIME;
                    (*menubox).fromTop = false;
                    return;
                }
                point3d rBase = r;
                point3d dBase = d;
                point3d fBase = f;



                rBase.x *= cos(tim); rBase.y *= sin(tim);  rBase.z = 0.0f;
                dBase.x = -rBase.y; dBase.y = rBase.x; dBase.z = 0.0f;
                fBase.x = 0.0f; fBase.y = 0.0f;

                voxie_drawspr(&vf,iconGamenam[j],&p,&rBase,&dBase,&fBase,0x707070 + 0x101010);

                for(fn=-.01f;fn<=.01f;fn+=.004f)
                {
                    float g;
                    // g = fn+sin(tim*2.f)*.04f + 0.8f*vw.aspx*fscale;
                    // draw_platonic(3,fx,fy,0.f,g*1.7f,tim*.5,1,0xffffff);
                    g = fn+sin(tim*2.f)*.04f + 0.8f*vw.aspx*fscale/5;
                    draw_platonic(3,fx,fy,0.f,g*1.7f,tim*.5,1,0xffffff);


                    point3d pp, rr, dd;
                    float ftemp;
                    char tbuf[256];
                    sprintf(tbuf,"%s",iconGamest[j]); ftemp = (float)strlen(tbuf)*.5;
                    g = (cos(tim*2.f)+1.f)*(PI/4.f);

                    rr.x = 0.12f; dd.x =        0.00f; pp.x = vw.aspx*fscale*+.00f - rr.x*ftemp - dd.x*.5f;
                    rr.y = 0.00f; dd.y = sin(g)*0.25f; pp.y = vw.aspy*fscale*+.87f - rr.y*ftemp - dd.y*.5f;
                    rr.z = 0.00f; dd.z = cos(g)*0.25f; pp.z = vw.aspz*fscale*+.00f - rr.z*ftemp - dd.z*.5f;
                    voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%s",iconGamest[j]);
                }
                // voxie_drawsph(&vf,palmPos.x+x1,palmPos.y+y1, palmPos.z+z1, .01f, 0, 0xffffff);
            }
            // Draw normal icons
            else
            {
                voxie_drawspr(&vf,iconGamenam[j],&p,&r,&d,&f,0x707070 + 0x101010);
            }
        }

        if (hitIcon == false)
        {
            (*menubox).fromTop = false;
        }
    }
}

static void execute_status1(point3d p, point3d r, point3d d, point3d f, LEAP_HAND *handr,float LeapScale, struct MenuBox* menubox)
{
    // testing: drawing elements in circle:   30.08.18

    if ((*menubox).delayTimer > 0)
    {
        (*menubox).delayTimer--;
    }
    else
    {
        (*menubox).timer = 0;
        (*menubox).delayTimer = 0;
    }

    if ((*menubox).delayTimer == 0)
    {
        float angle, x1, y1, z1;
        int i, j;
        float length = (float) (r.x * 0.5f);

        p.x -= 0.25f;
        p.y -= 0.25f;

        float fx, fy, fn, fscale;
        fscale = 1.f/min(min(vw.aspx,vw.aspy),vw.aspz/.2f);
        // voxie_setview(&vf,-vw.aspx*fscale,-vw.aspy*fscale,-vw.aspz*fscale,+vw.aspx*fscale,+vw.aspy*fscale,+vw.aspz*fscale);

        //TO DO: Firework
        // 03.09.18
        bool hitIcon = false;
        for (i = 0; i < 360; i += 60)
        {
            angle = i;
            x1 = 0.5 * cos(angle * PI / 180);
            y1 = 0.5 * sin(angle * PI / 180);
            z1 = 0;

            p.x = ((float) p.x + x1);
            p.y = ((float) p.y + y1);
            p.z = ((float) p.z + z1);

            // voxie_drawcube(&vf,&p,&r,&d,&f,1,0xffffff);
            j = (i/60);

            //draw_platonic (int ind, float posx, float posy, float posz, float rad, float rot, int fillmode, int col)
            // Draw normal icons


            fx = p.x, fy = p.y;

            if ((indexPos.x >= p.x-1.0*length && indexPos.x <= p.x+1.0*length)
                    &&(indexPos.y >= p.y-1.0*length && indexPos.y <= p.y+1.0*length))
            {
                hitIcon = true;
                if ((*menubox).fromTop == false &&
                        // (indexPos.z <= p.z+0.8f*f.z && indexPos.z >= p.z+0.3f*f.z))
                        indexPos.z <= p.z - 0.4 * f.z)
                {
                    (*menubox).fromTop = true;
                }
                else if ((*menubox).fromTop == true &&
                         // (((&handr->palm)->velocity).y <= -60 && indexPos.z <= p.z))
                         (indexPos.z <= p.z+0.8f*f.z && indexPos.z >= p.z+0.3f*f.z))
                    // indexPos.z <= p.z - 0.3 * f.z)
                {
                    // Testing nested-circle 25.09.18
                    if (j == RENDMODE_GAMEFOLDER)
                    {
                        (*menubox).timer = 0;
                        (*menubox).status = 2;
                        (*menubox).delayTimer = DELAYTIME;
                        (*menubox).fromTop = false;
                        return;
                    }


                    // End here

                    // return to status 0: edited by 05.09.18
                    // if (((&handr->palm)->velocity).y <= -60 && j == RENDMODE_TITLE &&
                    //         indexPos.z <= p.z)
                    else
                    {
                        grendmode = j;
                        (*menubox).timer = 0;
                        (*menubox).status = 0;
                        (*menubox).delayTimer = DELAYTIME;
                        (*menubox).fromTop = false;
                        return;
                    }
                }
                point3d rBase = r;
                point3d dBase = d;
                point3d fBase = f;

                rBase.x *= cos(tim); rBase.y *= sin(tim);  rBase.z = 0.0f;
                dBase.x = -rBase.y; dBase.y = rBase.x; dBase.z = 0.0f;
                fBase.x = 0.0f; fBase.y = 0.0f;
                voxie_drawspr(&vf,iconnam[j],&p,&rBase,&dBase,&fBase,0x707070 + 0x101010);

                for(fn=-.01f;fn<=.01f;fn+=.004f)
                {
                    float g;
                    // g = fn+sin(tim*2.f)*.04f + 0.8f*vw.aspx*fscale;
                    // draw_platonic(3,fx,fy,0.f,g*1.7f,tim*.5,1,0xffffff);
                    g = fn+sin(tim*2.f)*.04f + 0.8f*vw.aspx*fscale/5;
                    draw_platonic(3,fx,fy,0.f,g*1.7f,tim*.5,1,0xffffff);


                    point3d pp, rr, dd;
                    float ftemp;
                    char tbuf[256];
                    sprintf(tbuf,"%s",iconst[j]); ftemp = (float)strlen(tbuf)*.5;
                    g = (cos(tim*2.f)+1.f)*(PI/4.f);

                    rr.x = 0.12f; dd.x =        0.00f; pp.x = vw.aspx*fscale*+.00f - rr.x*ftemp - dd.x*.5f;
                    rr.y = 0.00f; dd.y = sin(g)*0.25f; pp.y = vw.aspy*fscale*+.87f - rr.y*ftemp - dd.y*.5f;
                    rr.z = 0.00f; dd.z = cos(g)*0.25f; pp.z = vw.aspz*fscale*+.00f - rr.z*ftemp - dd.z*.5f;
                    voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%s",iconst[j]);
                }
                // voxie_drawsph(&vf,palmPos.x+x1,palmPos.y+y1, palmPos.z+z1, .01f, 0, 0xffffff);
            }
            // Draw normal icons
            else
            {

                voxie_drawspr(&vf,iconnam[j],&p,&r,&d,&f,0x707070 + 0x101010);
            }

        }

        if (hitIcon == false)
        {
            (*menubox).fromTop = false;
        }
    }
}

static void menuEvent(point3d p, point3d r, point3d d, point3d f, LEAP_HAND *handr ,float LeapScale, struct MenuBox* menubox)
{
    getIndexPos(handr, LeapScale, 2, .2);

    if ((*menubox).status == 0)
    {
        execute_status0(p, r, d, f, handr, LeapScale, menubox);
    }

    else if ((*menubox).status == 1)
    {
        execute_status1(p, r, d, f, handr, LeapScale, menubox);
    }
    else if ((*menubox).status == 2)
    {
        execute_status1_enter_game_folder(p, r, d, f, handr, LeapScale, menubox);
    }

}

enum ExitStatus { NOP, READY, FIRE };
// 0:NO OP;    1: READY,   2: TRIGGER
static int checkExitStatus = 0;
static int delayExitTimer = 0;
static int DELAYEXIT = 5;

static void checkExitGesture(LEAP_HAND *hand, float LeapScale, int rendmode)
{
    if (hand->type == eLeapHandType_Left)
    {
        int i, j;
        LEAP_DIGIT *digit;
        bool checkExits[5] = { false, false, false, false, false };
        for (i = 0; i < 5; i++)
        {
            digit = &hand->digits[i];
            if (digit->is_extended)
                checkExits[i] = true;
        }

        if (checkExitStatus == NOP && checkExits[0] && checkExits[1] && !checkExits[2] && !checkExits[3] && !checkExits[4])
        {
            if (delayExitTimer < DELAYEXIT)
            {
                delayExitTimer++;
            }
            else
            {
                checkExitStatus = READY;
                delayExitTimer = 0;
            }
        }
        else if (checkExitStatus == READY && !checkExits[2] && !checkExits[3] && !checkExits[4])
        {
            drawHands(hand, LeapScale, 2);
            if (!checkExits[0] && checkExits[1] )
            {
                if (delayExitTimer < DELAYEXIT)
                {
                    delayExitTimer++;
                }
                else
                {
                    checkExitStatus = FIRE;
                    delayExitTimer = 0;
                }
            }
        }
        else
        {
            checkExitStatus = NOP;
            delayExitTimer = 0;
        }
    }
}


/* end here */
/************************************************/




static void calcluts (void)
{
    int i, j;

    j = vw.framepervol*24;
    for(i=0;i<j;i++) //WARNING:do not reverse for loop!
    {
        clutmid[i] = (int)(cos((double)(i+.5)*PI*2.0/(float)j)*-256.0);
        div3[i] = i/3;
        div24[i] = (div3[i]>>3);
        mod3[i] = i%3;
        mod24[i] = i%24;
        oneupmod24[i] = (1<<mod24[i]);
    }
}


static void loadini (void)
{
    float f, animdefscale = 0.5f;
    int i, j, oficnt, ficnt, fileng, gotmun = 0, keymenucnt = 0;
    char *fibuf, och, *cptr, *eptr, ktid[] = "[VOXIEDEMO]\r\n";

    if (!kzopen("voxiedemo.ini")) return;
    fileng = kzfilelength();
    fibuf = (char *)malloc(fileng+1); if (!fibuf) { kzclose(); return; }
    kzread(fibuf,fileng);
    kzclose();

    //Preprocessor (#include)
    for(ficnt=0;ficnt<fileng;ficnt++)
    {
        oficnt = ficnt;
        cptr = &fibuf[ficnt];
        for(;ficnt<fileng;ficnt++) if (fibuf[ficnt] == '\r') break;
        och = fibuf[ficnt]; fibuf[ficnt] = 0;

        if (!memcmp(cptr,"#include ",9))
        {
            cptr += 9;
            if (cptr[0] == '<') { cptr++; for(i=0;cptr[i];i++) if (cptr[i] == '>') { cptr[i] = 0; break; } } //Strip <>
            if (cptr[0] == 34) { cptr++; for(i=0;cptr[i];i++) if (cptr[i] == 34) { cptr[i] = 0; break; } } //Strip ""
            if (kzopen(cptr))
            {
                int fileng2;

                ficnt++; memmove(&fibuf[oficnt],&fibuf[ficnt],fileng-ficnt); fileng += oficnt-ficnt; ficnt = oficnt; //remove #include line

                fileng2 = kzfilelength();
                fibuf = (char *)realloc(fibuf,fileng+fileng2+1); if (!fibuf) { kzclose(); return; }
                memmove(&fibuf[oficnt+fileng2],&fibuf[oficnt],fileng-ficnt); //shift file memory up
                kzread(&fibuf[oficnt],fileng2); //insert included file
                kzclose();

                fileng += fileng2;
            }
        } else { fibuf[ficnt] = och; }

        while ((ficnt < fileng) && (fibuf[ficnt] < 32)) { ficnt++; } ficnt--;
    }

    for(ficnt=0;ficnt<fileng;ficnt++) if (!memcmp(&fibuf[ficnt],ktid,strlen(ktid))) break;
    for(;ficnt<fileng;ficnt++)
    {
        cptr = &fibuf[ficnt];
        for(;ficnt<fileng;ficnt++) if (fibuf[ficnt] == '\r') break;
        och = fibuf[ficnt]; fibuf[ficnt] = 0;

        while (1)
        {
            eptr = 0;
            if (!memcmp(cptr,"rendmode="   , 9)) grendmode = min(max(strtol(&cptr[9],&eptr,0),0),RENDMODE_END-1);
            if (!memcmp(cptr,"showborder=" ,11)) gshowborder = min(max(strtol(&cptr[11],&eptr,0),0),1);
            if (!memcmp(cptr,"menutimeout=",12)) gmenutimeout = max(strtol(&cptr[12],&eptr,0),-1);
            if (!memcmp(cptr,"mountzip="   , 9)) voxie_mountzip(&cptr[9]);
            if (!memcmp(cptr,"mediadir="   , 9)) { i = (int)strlen(&cptr[9]); gmediadir = (char *)malloc(i+1); memcpy(gmediadir,&cptr[9],i+1); }

            if (!memcmp(cptr,"recfile="    , 8)) { if (grecn < GRECMAX) { sprintf(grec[grecn].file,"%.*s",sizeof(grec[0].file)-1,&cptr[8]); grec[grecn].mode = 0; grec[grecn].rep = 1; grecn++; } }
            if (!memcmp(cptr,"recmode="    , 8)) { if (grecn) grec[grecn-1].mode = min(max(strtol(&cptr[8],&eptr,0),0),2); }
            if (!memcmp(cptr,"recrep="     , 7)) { if (grecn) grec[grecn-1].rep = max(strtol(&cptr[7],&eptr,0),0); }

            if (!memcmp(cptr,"demfile="    , 8))
            {
                if (gdemn < GDEMMAX)
                {
                    sprintf(gdem[gdemn].file,"%.*s",sizeof(gdem[0].file)-1,&cptr[8]);
                    f = 2.f/max(max(vw.aspx,vw.aspy),vw.aspz);
                    gdem[gdemn].r.x = f*2.f; gdem[gdemn].d.x = 0.f; gdem[gdemn].f.x = 0.f;
                    gdem[gdemn].r.y = 0.f; gdem[gdemn].d.y = f*2.f; gdem[gdemn].f.y = 0.f;
                    gdem[gdemn].r.z = 0.f; gdem[gdemn].d.z = 0.f; gdem[gdemn].f.z =-.4; //vw.aspz*f;
                    gdem[gdemn].p.x = gdem[gdemn].r.x*-.5f;
                    gdem[gdemn].p.y = gdem[gdemn].d.y*-.5f;
                    gdem[gdemn].p.z = gdem[gdemn].f.z*-.5f;

                    gdem[gdemn].colorkey = 0x80ff00ff;
                    gdem[gdemn].usesidebyside = 0;
                    gdem[gdemn].mapzen = 0;
                    gdemn++;
                }
            }
            if (!memcmp(cptr,"demcolorkey=",12))   { if (gdemn) gdem[gdemn-1].colorkey      = strtoul(&cptr[12],&eptr,0); }
            if (!memcmp(cptr,"demscalex="  ,10))   { if (gdemn) { f = strtod(&cptr[10],&eptr); gdem[gdemn-1].r.x *= f; gdem[gdemn-1].p.x = gdem[gdemn-1].r.x*-.5f; } }
            if (!memcmp(cptr,"demscaley="  ,10))   { if (gdemn) { f = strtod(&cptr[10],&eptr); gdem[gdemn-1].d.y *= f; gdem[gdemn-1].p.y = gdem[gdemn-1].d.y*-.5f; } }
            if (!memcmp(cptr,"demscalez="  ,10))   { if (gdemn) { f = strtod(&cptr[10],&eptr); gdem[gdemn-1].f.z *= f; gdem[gdemn-1].p.z = gdem[gdemn-1].f.z*-.5f; } }
            if (!memcmp(cptr,"demscale="   , 9))   { if (gdemn) { f = strtod(&cptr[ 9],&eptr); gdem[gdemn-1].f.z *= f; gdem[gdemn-1].p.z = gdem[gdemn-1].f.z*-.5f; } }
            if (!memcmp(cptr,"demsidebyside=",14)) { if (gdemn) gdem[gdemn-1].usesidebyside = min(max(strtol(&cptr[14],&eptr,0),0),1); }
            if (!memcmp(cptr,"demmapzen="  ,10))   { if (gdemn) gdem[gdemn-1].mapzen        = min(max(strtol(&cptr[10],&eptr,0),0),1); }

            if (!memcmp(cptr,"mystring="   , 9))   { sprintf(gmyst,"%.*s",sizeof(gmyst)-1,&cptr[9]); }

            if (!memcmp(cptr,"chesscol1="  ,10))   { gchesscol[0] = strtoul(&cptr[10],&eptr,0); }
            if (!memcmp(cptr,"chesscol2="  ,10))   { gchesscol[1] = strtoul(&cptr[10],&eptr,0); }
            if (!memcmp(cptr,"chessailev1=",12))   { gchessailev[0] = min(max(strtoul(&cptr[12],&eptr,0),0),8); }
            if (!memcmp(cptr,"chessailev2=",12))   { gchessailev[1] = min(max(strtoul(&cptr[12],&eptr,0),0),8); }
            if (!memcmp(cptr,"chesstime="  ,10))   { gchesstime = strtod(&cptr[10],&eptr); }

            if (!memcmp(cptr,"minplays="   , 9))   { gminplays = min(max(strtol(&cptr[9],&eptr,0),1),4); }
            if (!memcmp(cptr,"dualnav="    , 8))   { gdualnav  = min(max(strtol(&cptr[8],&eptr,0),0),1); }

            if (!memcmp(cptr,"pakrendmode=",12))   { gpakrendmode    = max(strtol(&cptr[12],&eptr,0),0); }
            if (!memcmp(cptr,"pakxdim="    , 8))   { gpakxdim        = max(strtol(&cptr[8],&eptr,0),0); }
            if (!memcmp(cptr,"pakydim="    , 8))   { gpakydim        = max(strtol(&cptr[8],&eptr,0),0); }

            if (!memcmp(cptr,"munxsiz="    , 8))   { if (!gotmun) { gotmun = 1; munn = 0; memset(gmun,0,sizeof(gmun)); } if (munn < MUNMAX) gmun[munn].xsiz = max(strtol(&cptr[8],&eptr,0),0); }
            if (!memcmp(cptr,"munysiz="    , 8))   { if (!gotmun) { gotmun = 1; munn = 0; memset(gmun,0,sizeof(gmun)); } if (munn < MUNMAX) gmun[munn].ysiz = max(strtol(&cptr[8],&eptr,0),0); }
            if (!memcmp(cptr,"munzsiz="    , 8))   { if (!gotmun) { gotmun = 1; munn = 0; memset(gmun,0,sizeof(gmun)); } if (munn < MUNMAX) gmun[munn].zsiz = max(strtol(&cptr[8],&eptr,0),0); }
            if (!memcmp(cptr,"munxwrap="   , 9))   { if (!gotmun) { gotmun = 1; munn = 0; memset(gmun,0,sizeof(gmun)); } if (munn < MUNMAX) gmun[munn].xwrap = min(max(strtol(&cptr[9],&eptr,0),0),1); }
            if (!memcmp(cptr,"munywrap="   , 9))   { if (!gotmun) { gotmun = 1; munn = 0; memset(gmun,0,sizeof(gmun)); } if (munn < MUNMAX) gmun[munn].ywrap = min(max(strtol(&cptr[9],&eptr,0),0),1); }
            if (!memcmp(cptr,"munzwrap="   , 9))   { if (!gotmun) { gotmun = 1; munn = 0; memset(gmun,0,sizeof(gmun)); } if (munn < MUNMAX) gmun[munn].zwrap = min(max(strtol(&cptr[9],&eptr,0),0),1); }
            if (!memcmp(cptr,"munboard="   , 9)) //scan ini for non-white chars until munxsiz*munysiz*munzsiz exhausted (or file ends)
            {
                if (munn < MUNMAX)
                {
                    int nbytes;

                    nbytes = min(max(gmun[munn].xsiz*gmun[munn].ysiz*gmun[munn].zsiz,1),1048576);
                    gmun[munn].board = (char *)malloc(nbytes);

                    j = 0;
                    for(i=ficnt;i<fileng;i++)
                    {
                        if (fibuf[i] <= 32) continue;
                        gmun[munn].board[j] = fibuf[i]; j++; if (j >= nbytes) break;
                    }
                    munn++;

                    fibuf[ficnt] = och; ficnt = i; och = fibuf[ficnt]; //hack to move outer loop file pointer up
                    break;
                }
            }

            if (!memcmp(cptr,"snaketime="  ,10))   { gsnaketime      = max(strtol(&cptr[10],&eptr,0),0); }
            if (!memcmp(cptr,"snakenumpels=",13))  { gsnakenumpels   = max(strtol(&cptr[13],&eptr,0),0); }
            if (!memcmp(cptr,"snakenumtetras=",15)){ gsnakenumtetras = max(strtol(&cptr[15],&eptr,0),0); }
            if (!memcmp(cptr,"snakepelspeed=",14)) { gsnakepelspeed  = strtod(&cptr[14],&eptr); }

            if (!memcmp(cptr,"animfile="   , 9))
            {
                if (ganimn < GANIMMAX)
                {
                    sprintf(ganim[ganimn].file,"%.*s",sizeof(ganim[0].file)-1,&cptr[9]);
                    ganim[ganimn].snd[0] = 0;
                    f = animdefscale;
                    ganim[ganimn].defscale = f;
                    ganim[ganimn].p.x = 0.f; ganim[ganimn].p_init.x = 0.f;
                    ganim[ganimn].p.y = 0.f; ganim[ganimn].p_init.y = 0.f;
                    ganim[ganimn].p.z = 0.f; ganim[ganimn].p_init.z = 0.f;
                    ganim[ganimn].ang[0] = 0.f; ganim[ganimn].ang_init[0] = 0.f;
                    ganim[ganimn].ang[1] = 0.f; ganim[ganimn].ang_init[1] = 0.f;
                    ganim[ganimn].ang[2] = 0.f; ganim[ganimn].ang_init[2] = 0.f;
                    ganim[ganimn].sc = f;
                    ganim[ganimn].fps = 2.0f;
                    ganim[ganimn].tt = 0;
                    ganim[ganimn].filetyp = 0; //0=PNG, 1=KV6/STL,etc..
                    ganim[ganimn].mode = 0; //0=forward, 1=pingpong
                    ganim[ganimn].n = 0;
                    ganim[ganimn].mal = 0;
                    ganim[ganimn].cnt = 0;
                    ganim[ganimn].loaddone = 0;
                    ganimn++;
                }
            }
            if (!memcmp(cptr,"animmode="   , 9)) { if (ganimn) ganim[ganimn-1].mode = min(max(strtol(&cptr[9],&eptr,0),0),1); }
            if (!memcmp(cptr,"animsnd="    , 8)) { if (ganimn) sprintf(ganim[ganimn-1].snd,"%.*s",sizeof(ganim[0].snd)-1,&cptr[8]); }
            if (!memcmp(cptr,"animfps="    , 8)) { if (ganimn) ganim[ganimn-1].fps = strtod(&cptr[8],&eptr); }
            if (!memcmp(cptr,"animscale="  ,10))
            {
                f = strtod(&cptr[10],&eptr);
                if (!ganimn) animdefscale = f;
                else
                {
                    ganim[ganimn-1].defscale = f;
                    ganim[ganimn-1].sc = f;
                }
            }

            if (!memcmp(cptr,"music_rendmode_",15))
            {
                i = strtol(&cptr[15],&eptr,0);
                for(j=0;j<gmusn;j++)
                    if (i == gmus[j].rendmode) break;
                if (j < GMUSMAX)
                {
                    gmus[j].rendmode = i;
                    sprintf(gmus[j].file,"%.*s",sizeof(gmus[0].file)-1,&eptr[1]);
                    if (j == gmusn) gmusn++;
                }
            }

            if (!memcmp(cptr,"key_",4))
            {
                if ((cptr[4] >= 'p') && (cptr[4] <= 'q') && (cptr[5] >= '1') && (cptr[5] <= '4'))
                {
                    i = (cptr[4]-'p') + (cptr[5]-'1')*2;
                    j = -1;
                    if (cptr[6] == 'l') j = 0;
                    else if (cptr[6] == 'r') j = 1;
                    else if (cptr[6] == 'u') j = 2;
                    else if (cptr[6] == 'd') j = 3;
                    else if (cptr[6] == 'a') j = 4;
                    else if (cptr[6] == 'b') j = 5;
                    else if (cptr[6] == 'm') j = 6;
                    if (j >= 0)
                    {
                        if (cptr[7] == '=') keyremap[i][j] = strtoul(&cptr[8],&eptr,0);
                    }
                }
                else if (!memcmp(&cptr[4],"menu=",5))
                {
                    if (keymenucnt < 2)
                    {
                        keymenu[keymenucnt] = strtoul(&cptr[9],&eptr,0);
                        keymenucnt++;
                    }
                }
            }
            if (!eptr) break;
            cptr = eptr;

            while ((cptr[0] == 9) || (cptr[0] == 32)) cptr++;
            if (cptr[0] != ';') break;
            cptr++;
            while ((cptr[0] == 9) || (cptr[0] == 32)) cptr++;
            if ((cptr[0] == '/') && (cptr[1] == '/')) break;
            if (cptr[0] < 32) break;
        }

        fibuf[ficnt] = och;
        while ((ficnt < fileng) && (fibuf[ficnt] < 32)) { ficnt++; } ficnt--;
    }
    free(fibuf);

    for(i=gdemn-1;i>=0;i--)
    {
        gdem[i].sp = gdem[i].p;
        gdem[i].sr = gdem[i].r;
        gdem[i].sd = gdem[i].d;
        gdem[i].sf = gdem[i].f;
    }
}

static void genpath_voxiedemo_exe (char *filnam, char *tbuf, int bufleng)
{
    static char gtbuf[MAX_PATH] = {0};
    static int gtleng = 0;
    int i;

    if (!gtbuf[0])
    {
        //Full path to exe (i.e. c:\kenstuff\voxon\voxiedemo.exe)
#if defined(_WIN32)
        gtleng = GetModuleFileName(0,gtbuf,bufleng-strlen(filnam)); //returns strlen?
#else
        char tbuf2[32];
        sprintf(tbuf2,"/proc/%d/exe",getpid());
        gtleng = min(readlink(tbuf2,gtbuf,bufleng-strlen(filnam),bufleng-strlen(filnam)-1); if (gtleng >= 0) gtbuf[gtleng] = 0;
#endif
        gtleng = strlen(gtbuf);
        while ((gtleng > 0) && (gtbuf[gtleng-1] != '\\') && (gtbuf[gtleng-1] != '/')) gtleng--;
    }
    i = strlen(filnam);
    memmove(&tbuf[gtleng],filnam,i+1);
    memcpy(tbuf,gtbuf,gtleng);
}

static void genpath_voxiedemo_media (char *filnam, char *tbuf, int bufleng)
{
    char nst[MAX_PATH];

    genpath_voxiedemo_exe("",nst,bufleng);
    if (gmediadir) strcat(nst,gmediadir);
    strcat(nst,filnam);
    strcpy(tbuf,nst);
}

int cmdline2arg (char *cmdline, char **argv)
{
    int i, j, k, inquote, argc;

    //Convert Windows command line into ANSI 'C' command line...
    argv[0] = (char *)"exe"; argc = 1; j = inquote = 0;
    for(i=0;cmdline[i];i++)
    {
        k = (((cmdline[i] != ' ') && (cmdline[i] != '\t')) || (inquote));
        if (cmdline[i] == '\"') inquote ^= 1;
        if (j < k) { argv[argc++] = &cmdline[i+inquote]; j = inquote+1; continue; }
        if ((j) && (!k))
        {
            if ((j == 2) && (cmdline[i-1] == '\"')) cmdline[i-1] = 0;
            cmdline[i] = 0; j = 0;
        }
    }
    if ((j == 2) && (cmdline[i-1] == '\"')) cmdline[i-1] = 0;
    argv[argc] = 0;
    return(argc);
}



/* End here*/




int WINAPI WinMain (HINSTANCE hinst, HINSTANCE hpinst, LPSTR cmdline, int ncmdshow)
{

    //voxie_frame_t vf;
    voxie_inputs_t in;
    voxie_xbox_t vx[4];
    int ovxbut[4], vxnplays;
    voxie_nav_t nav[4] = {0};
    int onavbut[4];
    pol_t pt[3];
    double d;
    int i, mousx = 256, mousy = 256, mousz = 0;
    // point3d pp, rr, dd, ff, pos = {0.0,0.0,0.0}, inc = {0.3,0.2,0.1};
    /* 27/08/18 */
    static int inited = -1;
    static double timig = -1e32;
    static float fofy = 0.f;
    static int icperow, icnum;
    float fscale, f;
    bool checkPressed = false;
    point3d m6p, m6r, m6d, m6f, m6ru, m6du, m6fu; int m6but, m6cnt;
    float g, fx, fy, fz, fr, gx, gy, gz, x0, y0, z0, x1, y1, z1;
    int obstatus, bstatus = 0, argc, argfilindex[2];
    int j, k, h, n, x, y, z, xx, yy, col, *wptr, *rptr, showphase, orendmode = -17;
    char *argv[MAX_PATH>>1], tbuf[MAX_PATH];
    /* end */

#if (USELEAP)
    /************************************************/
    /* adding Leap motion device, by Yi-Ting, Hsieh */
    int numframes = 0;
    char* event = nullptr;
    Leap_Voxon leap_voxon;		// calling Leap_Voxon for setting up leap motion service

    enum State initial_state;
    initial_state = UNPRESSED;
    struct MenuBox menubox = { 0, 0, 0, false };
    enum ExitStatus trackExitStatus;
    trackExitStatus = NOP;

    /* end here */
#endif
#if (USEMAG6D)
    gusemag6d = mag6d_init();
#endif

    platonic_init();

    if (voxie_load(&vw) < 0) //Load voxiebox.dll and get settings from voxiebox.ini. May override settings in vw structure here if desired.
    { MessageBox(0,"Error: can't load voxiebox.dll","",MB_OK); }

    loadini(); //Load settings from voxiedemo.ini

    argc = cmdline2arg(cmdline,argv); argfilindex[0] = -1; argfilindex[1] = -1;
    for(i=1;i<argc;i++)
    {
        if ((argv[i][0] != '-') && (argv[i][0] != '/'))
        {
            if (argfilindex[0] < 0) { argfilindex[0] = i; continue; }
            if (argfilindex[1] < 0) { argfilindex[1] = i; continue; }
            continue;
        }
        if ((argv[i][1] == 'R') || (argv[i][1] == 'r')) { grendmode    = min(max(atol(&argv[i][2]), 0  ), RENDMODE_END-1); continue; }
        if ((argv[i][1] == 'E') || (argv[i][1] == 'e')) { vw.useemu    = min(max(atol(&argv[i][2]), 0  ),  1); vw.usekeystone = !vw.useemu; continue; }
        if ((argv[i][1] == 'C') || (argv[i][1] == 'c')) { vw.usecol    = min(max(atol(&argv[i][2]),-6  ),  1); continue; }
        if ((argv[i][1] == 'Z') || (argv[i][1] == 'z')) { vw.aspz      = min(max(atof(&argv[i][2]),.01f),1.f); continue; }
    }

    calcluts();

    voxie_init(&vw);//Start video and audio.
    /*changed 18.09.18 */
    while (!voxie_breath(&in))
    {
        otim = tim; tim = voxie_klock(); dtim = tim-otim;
        obstatus = bstatus; fx = in.dmousx; fy = in.dmousy; fz = in.dmousz; bstatus = in.bstat;
        for(vxnplays=0;vxnplays<4;vxnplays++)
        {
            ovxbut[vxnplays] = vx[vxnplays].but;
#if (!USEMAG6D)
            if (!voxie_xbox_read(vxnplays,&vx[vxnplays])) break; //but, lt, rt, tx0, ty0, tx1, ty1
#else
            memset(&vx[vxnplays],0,sizeof(vx[0]));
#endif
        }

        while (i = voxie_keyread())
        {
            if (((((i>>8)&255) == 0x3e) && (i&0x300000)) ||
                    ((((i>>8)&255) == 0x01) && (i&0x030000))) { voxie_quitloop(); break; } //Alt+F4 or Shift+ESC: quit
            if ((((i>>8)&255) == 0x1e) && (i&0xc0000)) { gautocycle = !gautocycle; cycletim = tim+5.0; continue; } //Ctrl+A
            //if ((((i>>8)&255) == 0x13) && (i&0xc0000)) { gautorotatespd = (!gautorotatespd)*5; continue; } //Ctrl+R
            if ((((i>>8)&255) == 0x13) && (i&0xc0000)) { gautorotatespd[2] = (!gautorotatespd[2])*5; continue; } //Ctrl+R
            if ((((i>>8)&255) == keymenu[0]) || (((i>>8)&255) == keymenu[1]))
            {
                if (grendmode != RENDMODE_TITLE) { gcurselmode = grendmode-1; grendmode = RENDMODE_TITLE; }
                gtimbore = tim+gmenutimeout;
            }


            switch(i&255)
            {
            case 27: //ESC: quit immediately
                if (((i>>8)&255) == 0x01)
                {
                    if (((keymenu[0] == 0x01) || (keymenu[1] == 0x01)) && (!(i&0x30000))) break; //If menu key is ESC, require Shift+ESC to quit
                    voxie_quitloop();
                }
                break;
            }
        }

        /* Adding menu 16.09.18  */
        if ((vx[0].but&~ovxbut[0])&(1<<5)) grendmode = RENDMODE_TITLE; //Back button

        if (grendmode != orendmode)
        {
            switch(grendmode)
            {
            case RENDMODE_TITLE:
                genpath_voxiedemo_media("voxiedemo_title.jpg",tbuf,sizeof(tbuf));
                voxie_menu_reset(menu_title_update,0,tbuf);
                voxie_menu_addtab("Select..",350,0,650,400);
                voxie_menu_additem("Up"    ,256, 42,128,96,MENU_UP   ,MENU_BUTTON+3,0,0x808080,0.0,0.0,0.0,0.0,0.0);
                voxie_menu_additem("Left"  , 96,162,128,96,MENU_LEFT ,MENU_BUTTON+3,0,0x808080,0.0,0.0,0.0,0.0,0.0);
                voxie_menu_additem("Select",256,162,128,96,MENU_ENTER,MENU_BUTTON+3,0,0x808080,0.0,0.0,0.0,0.0,0.0);
                voxie_menu_additem("Right" ,416,162,128,96,MENU_RIGHT,MENU_BUTTON+3,0,0x808080,0.0,0.0,0.0,0.0,0.0);
                voxie_menu_additem("Down"  ,256,282,128,96,MENU_DOWN ,MENU_BUTTON+3,0,0x808080,0.0,0.0,0.0,0.0,0.0);
                break;

            case RENDMODE_PHASESYNC:

                	genpath_voxiedemo_media("voxiedemo_phasesync.jpg",tbuf,sizeof(tbuf));
                	voxie_menu_reset(menu_phasesync_update,0,tbuf);
                	voxie_menu_addtab("PhaseSync",350,0,650,400);

                	voxie_menu_additem("Wireframe:",114, 96,128, 64,0              ,MENU_TEXT    ,0                    ,0x808080,0.0,0.0,0.0,0.0,0.0);
                	voxie_menu_additem("Zigzag"    ,244, 40,128,128,MENU_ZIGZAG    ,MENU_BUTTON+1,gphasesync_curmode==0,0x808080,0.0,0.0,0.0,0.0,0.0);
                	voxie_menu_additem("Sine"      ,362, 40,128,128,MENU_SINE      ,MENU_BUTTON+2,gphasesync_curmode!=0,0x808080,0.0,0.0,0.0,0.0,0.0);

                // 	i = (gphasesync_cursc == 1)*1 +
                // 		 (gphasesync_cursc == 2)*2 +
                // 		 (gphasesync_cursc == 4)*3 +
                // 		 (gphasesync_cursc == 8)*4 +
                // 		 (gphasesync_cursc ==16)*5;
                	voxie_menu_additem("Hump Level",128,224,360, 64,MENU_HUMP_LEVEL,MENU_HSLIDER ,0                    ,0x808080,(double)i,1.0,5.0,1.0,1.0);

                break;

            case RENDMODE_KEYSTONECAL:
                	genpath_voxiedemo_media("voxiedemo_keystonecal.jpg",tbuf,sizeof(tbuf));
                	voxie_menu_reset(menu_keystonecal_update,&keystone,tbuf);
                	voxie_menu_addtab("Keystone",350,0,650,400);

                	voxie_menu_additem("Put Cursor on..", 64, 32,128, 64,0              ,MENU_TEXT    ,0,0x808080,0.0,0.0,0.0,0.0,0.0);
                	voxie_menu_additem("Ceiling"        ,190, 50,256,100,MENU_CEILING   ,MENU_BUTTON+3,0,0x808080,0.0,0.0,0.0,0.0,0.0);
                	voxie_menu_additem("Floor"          ,190,180,256,100,MENU_FLOOR     ,MENU_BUTTON+3,0,0x808080,0.0,0.0,0.0,0.0,0.0);

                	if (vw.dispnum > 1)
                	{
                		voxie_menu_additem("Select"   ,      45,332, 64, 64,0                   ,MENU_TEXT    ,0                                 ,0x908070, 0.0,0.0,0.0,0.0,0.0);
                		voxie_menu_additem("Display"  ,      10,352, 64, 64,0                   ,MENU_TEXT    ,0                                 ,0x908070, 0.0,0.0,0.0,0.0,0.0);
                		// for(i=0;i<vw.dispnum;i++)
                		// {
                		// 	tbuf[0] = i+'1'; tbuf[1] = 0;
                		// 	voxie_menu_additem(tbuf ,i*64+136,320, 64, 64,MENU_DISP_CUR+i     ,MENU_BUTTON+(i==0)+(i==vw.dispnum-1)*2,i==vw.dispcur,0x908070, 0.0,0.0,0.0,0.0,0.0);
                		// }
                	}

                break;

            case RENDMODE_HEIGHTMAP:
                	genpath_voxiedemo_media("voxiedemo_heimap.jpg",tbuf,sizeof(tbuf));
                	voxie_menu_reset(menu_heightmap_update,0,tbuf);
                	voxie_menu_addtab("HeightMap",350,0,650,400);
                  //
                	// voxie_menu_additem("Prev"         , 34, 32,280, 64,MENU_PREV          ,MENU_BUTTON+3,0                      ,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("Next"         ,338, 32,280, 64,MENU_NEXT          ,MENU_BUTTON+3,0                      ,0x808080,0.0,0.0,0.0,0.0,0.0);
                  //
                	// voxie_menu_additem("AutoCycle"    , 48,148, 72, 64,0                  ,MENU_TEXT    ,0                      ,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("Off"          ,168,122, 72, 64,MENU_AUTOCYCLEOFF  ,MENU_BUTTON+1,(gautocycle==0)        ,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("On"           ,240,122, 72, 64,MENU_AUTOCYCLEON   ,MENU_BUTTON+2,(gautocycle!=0)        ,0x808080,0.0,0.0,0.0,0.0,0.0);
                  //
                	// voxie_menu_additem("Texel Filter" , 14,236, 72, 64,0                  ,MENU_TEXT    ,0                      ,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("Near"         ,168,210, 72, 64,MENU_TEXEL_NEAREST ,MENU_BUTTON+1,(gheightmap_flags&2)==0,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("Bilin"        ,240,210, 72, 64,MENU_TEXEL_BILINEAR,MENU_BUTTON+2,(gheightmap_flags&2)!=0,0x808080,0.0,0.0,0.0,0.0,0.0);
                  //
                	// voxie_menu_additem("Slice Dither" ,320,148, 72, 64,0                  ,MENU_TEXT    ,0                      ,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("Off"          ,474,122, 72, 64,MENU_SLICEDITHEROFF,MENU_BUTTON+1,(gheightmap_flags&1)==0,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("On"           ,546,122, 72, 64,MENU_SLICEDITHERON ,MENU_BUTTON+2,(gheightmap_flags&1)!=0,0x808080,0.0,0.0,0.0,0.0,0.0);
                  //
                	// voxie_menu_additem("Texture"      ,332,236, 72, 64,0                  ,MENU_TEXT    ,0                      ,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("Off"          ,474,210, 72, 64,MENU_TEXTUREOFF    ,MENU_BUTTON+1,(gheightmap_flags&4)==0,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("On"           ,546,210, 72, 64,MENU_TEXTUREON     ,MENU_BUTTON+2,(gheightmap_flags&4)!=0,0x808080,0.0,0.0,0.0,0.0,0.0);

                	voxie_menu_additem("Reset camera" , 32,304,586, 64,MENU_RESET         ,MENU_BUTTON+3,0                      ,0x808080,0.0,0.0,0.0,0.0,0.0);

                break;
                //
            case RENDMODE_CHESS:
                	genpath_voxiedemo_media("voxiedemo_chess.jpg",tbuf,sizeof(tbuf));
                	voxie_menu_reset(menu_chess_update,0,tbuf);
                	voxie_menu_addtab("Chess",350,0,650,410);
                	// voxie_menu_additem("Hint"      , 32, 32,586, 64,MENU_HINT       ,MENU_BUTTON+3,0                 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                	voxie_menu_additem("Auto move" ,172,156,128, 64,0               ,MENU_TEXT    ,0                 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                	// voxie_menu_additem("Off"       ,300,132, 64, 64,MENU_AUTOMOVEOFF,MENU_BUTTON+1,gchess_automove==0,0x808080,0.0,0.0,0.0,0.0,0.0);
                	voxie_menu_additem("On"        ,364,132, 64, 64,MENU_AUTOMOVEON ,MENU_BUTTON+2,gchess_automove!=0,0x808080,0.0,0.0,0.0,0.0,0.0);
                	voxie_menu_additem("Difficulty",128,222,400, 64,MENU_DIFFICULTY ,MENU_HSLIDER ,0                 ,0x808080,(double)gchessailev[1],1.0,6.0,1.0,1.0);
                	voxie_menu_additem("Reset game", 32,324,586, 64,MENU_RESET      ,MENU_BUTTON+3,0                 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                break;


            case RENDMODE_MODELANIM:

                				genpath_voxiedemo_media("voxiedemo_modelanim.jpg",tbuf,sizeof(tbuf));
                				voxie_menu_reset(menu_modelanim_update,0,tbuf);
                				voxie_menu_addtab("ModelAnim",200,0,800,430);

                // 				voxie_menu_additem("Prev Model"      , 80, 19,138, 64,MENU_PREV         ,MENU_BUTTON+3,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Next Model"      ,226, 19,138, 64,MENU_NEXT         ,MENU_BUTTON+3,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                //
                // 				voxie_menu_additem("Prev Frame"      ,440, 19,138, 64,MENU_FRAME_PREV   ,MENU_BUTTON+3,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Next Frame"      ,586, 19,138, 64,MENU_FRAME_NEXT   ,MENU_BUTTON+3,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                //
                // 				voxie_menu_additem("Draw stats"      , 60,129, 64, 64,0                 ,MENU_TEXT    ,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Off"             ,188,103, 64, 64,MENU_DRAWSTATSOFF ,MENU_BUTTON+1,gdrawstats==0 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("On"              ,252,103, 64, 64,MENU_DRAWSTATSON  ,MENU_BUTTON+2,gdrawstats==1 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                //
                // 				voxie_menu_additem("Cross section"   , 22,209, 64, 64,0                 ,MENU_TEXT    ,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Off"             ,188,183, 64, 64,MENU_SLICEOFF     ,MENU_BUTTON+1,gslicemode==0 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("On"              ,252,183, 64, 64,MENU_SLICEON      ,MENU_BUTTON+2,gslicemode==1 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                //
                // 				voxie_menu_additem("AutoCycle"       ,450,129, 64, 64,0                 ,MENU_TEXT    ,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Off"             ,568,103, 64, 64,MENU_AUTOCYCLEOFF ,MENU_BUTTON+1,gautocycle==0 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("On"              ,632,103, 64, 64,MENU_AUTOCYCLEON  ,MENU_BUTTON+2,gautocycle==1 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                //
                // 				voxie_menu_additem("Reset pos&ori"   , 92,265,256, 64,MENU_RESET        ,MENU_BUTTON+3,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Pause"           , 92,345,256, 64,MENU_PAUSE        ,MENU_BUTTON+3,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Load All pos&ori",458,265,256, 64,MENU_LOAD         ,MENU_BUTTON+3,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Save All pos&ori",458,345,256, 64,MENU_SAVE         ,MENU_BUTTON+3,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                //
#if 0
                // 				voxie_menu_addtab("AutoRot.",400,0,600,280);
                // 				voxie_menu_additem("Axis"            ,140, 59, 64, 64,0                 ,MENU_TEXT    ,0             ,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Off"             ,218, 33, 64, 64,MENU_AUTOROTATEOFF,MENU_BUTTON+1,gautorotateax==-1,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("X"               ,282, 33, 64, 64,MENU_AUTOROTATEX  ,MENU_BUTTON  ,gautorotateax==0,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Y"               ,346, 33, 64, 64,MENU_AUTOROTATEY  ,MENU_BUTTON  ,gautorotateax==1,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Z"               ,410, 33, 64, 64,MENU_AUTOROTATEZ  ,MENU_BUTTON+2,gautorotateax==2,0x808080,0.0,0.0,0.0,0.0,0.0);
                // 				voxie_menu_additem("Speed"           ,128,134,360, 64,MENU_AUTOROTATESPD,MENU_HSLIDER ,0             ,0x808080,(double)gautorotatespd,0.0,10.0,1.0,1.0);
#else
                				voxie_menu_addtab("AutoRot.",400,0,600,380);
                				voxie_menu_additem("Rotation axis"   ,224, 32, 64, 64,0                 ,MENU_TEXT    ,0                ,0x808080,0.0,0.0,0.0,0.0,0.0);
                				voxie_menu_additem("X"               ,128, 64,360, 64,MENU_AUTOROTATEX  ,MENU_HSLIDER ,0                ,0x808080,(double)gautorotatespd[0],0.0,10.0,1.0,1.0);
                				voxie_menu_additem("Y"               ,128,160,360, 64,MENU_AUTOROTATEY  ,MENU_HSLIDER ,0                ,0x808080,(double)gautorotatespd[1],0.0,10.0,1.0,1.0);
                				voxie_menu_additem("Z"               ,128,256,360, 64,MENU_AUTOROTATEZ  ,MENU_HSLIDER ,0                ,0x808080,(double)gautorotatespd[2],0.0,10.0,1.0,1.0);
#endif
                break;


            case RENDMODE_DOTMUNCH:
                  genpath_voxiedemo_media("voxiedemo_dotmunch.jpg",tbuf,sizeof(tbuf));
                  voxie_menu_reset(menu_dotmunch_update,0,tbuf);
                  voxie_menu_addtab("DotMunch",350,0,650,400);

                  voxie_menu_additem("Prev Level"         ,110, 32,200, 96,MENU_PREV        ,MENU_BUTTON+3,0            ,0x808080,0.0,0.0,0.0,0.0,0.0);
                  voxie_menu_additem("Next Level"         ,342, 32,200, 96,MENU_NEXT        ,MENU_BUTTON+3,0            ,0x808080,0.0,0.0,0.0,0.0,0.0);

                  voxie_menu_additem("Reset game", 32,304,586, 64,MENU_RESET      ,MENU_BUTTON+3,0                 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                  break;


            case RENDMODE_FLYSTOMP:
                genpath_voxiedemo_media("voxiedemo_flystomp.jpg",tbuf,sizeof(tbuf));
                voxie_menu_reset(menu_flystomp_update,0,tbuf);
                voxie_menu_addtab("FlyStomp",350,0,650,400);
                voxie_menu_additem("Reset game", 32,304,586, 64,MENU_RESET      ,MENU_BUTTON+3,0                 ,0x808080,0.0,0.0,0.0,0.0,0.0);
                break;

            case RENDMODE_SNAKETRON:
        					genpath_voxiedemo_media("voxiedemo_snaketron.jpg",tbuf,sizeof(tbuf));
        					voxie_menu_reset(menu_snaketron_update,0,tbuf);
        					voxie_menu_addtab("SnakeTron",350,0,650,400);

        						  if (gsnakepelspeed < 0.75f/16.f)i = 1;
        					else if (gsnakepelspeed < 0.75f/8.f) i = 2;
        					else if (gsnakepelspeed < 0.75f/4.f) i = 3;
        					else if (gsnakepelspeed < 0.75f/2.f) i = 4;
        					else if (gsnakepelspeed < 0.75f    ) i = 5;
        					else if (gsnakepelspeed < 0.75f*2.f) i = 6;
        					else                                 i = 7;
        					voxie_menu_additem("Pellet speed",128,124,400, 64,MENU_SPEED     ,MENU_HSLIDER ,0                 ,0x808080,(double)i,1.0,7.0,1.0,1.0);

        					voxie_menu_additem("Reset game", 32,304,586, 64,MENU_RESET      ,MENU_BUTTON+3,0                 ,0x808080,0.0,0.0,0.0,0.0,0.0);
        					break;

            default:
                voxie_menu_reset(0,0,0);
            }
            orendmode = grendmode; voxie_playsound_update(-1,-1,0,0,1.f);/*kill all sound*/
            for(j=0;j<gmusn;j++) { if (gmus[j].rendmode == grendmode) { voxie_playsound(gmus[j].file,-1,100,100,1.f); break; } }
        }

        if (vw.useemu)
        {
            static int autonudge = 0;
            i = (voxie_keystat(0x1b)&1) - (voxie_keystat(0x1a)&1);
            if (i || autonudge)
            {
                if (i) autonudge = voxie_keystat(0x2a);
                if (autonudge) vw.emuhang += cos(tim*1.0)*(float)autonudge*.0005;
                if (voxie_keystat(0xb8)|voxie_keystat(0xdd)) vw.emuvang = min(max(vw.emuvang+(float)i*dtim*2.0,-PI*.5),0.0); //(Ralt|RMenu)+[,]
                else if (voxie_keystat(0x1d)|voxie_keystat(0x9d)) vw.emudist = min(max(vw.emudist-(float)i*dtim*2048.0,400.0),4000.0); //Ctrl+[,]
                else                                              vw.emuhang += (float)i*dtim*2.0; //[,]
                voxie_init(&vw);
            }
        }

#if (USEMAG6D)
        		if (gusemag6d >= 0)
        		{
        			dpoint3d ipos, irig, idow, ifor;

        			gusemag6d = mag6d_getnumdevices();
        			if (gusemag6d > 0)
        			{
        				m6cnt = mag6d_read(0,&ipos,&irig,&idow,&ifor,&m6but);

        				m6ru.x = irig.x; m6du.x = idow.x; m6fu.x = ifor.x;
        				m6ru.y = irig.y; m6du.y = idow.y; m6fu.y = ifor.y;
        				m6ru.z = irig.z; m6du.z = idow.z; m6fu.z = ifor.z;

        				m6r.x = irig.x; m6d.x = idow.x; m6f.x = ifor.x; m6p.x = (   ipos.x);
        				m6r.y =-irig.z; m6d.y =-idow.z; m6f.y =-ifor.z; m6p.y = (-1-ipos.z);
        				m6r.z = irig.y; m6d.z = idow.y; m6f.z = ifor.y; m6p.z = (   ipos.y);
        			}
        		}
#endif
        for(i=0;i<4;i++)
        {
          onavbut[i] = nav[i].but; voxie_nav_read(i,&nav[i]);
        }

        /* end here */

        voxie_frame_start(&vf);
        gxres = vf.x1-vf.x0;
        gyres = vf.y1-vf.y0;

        showphase = 0;

        switch(grendmode)
        {
        case RENDMODE_TITLE: //title/icon-based mode selection
        {
            static int inited = -1;

            point3d pp, rr, dd, ff;
            /* 27/08/18 */
            if (inited <= 0)
            {
              if (inited < 0)
              {
                voxie_mountzip("icons.zip");
              }
              inited = 1;
            }

            voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,vw.aspx,vw.aspy,vw.aspz);

#if (USELEAP)
            /****************18.09.18***********************/
            /* adding Leap motion device, by Yi-Ting, Hsieh */
            if (voxie_keystat(0x2)) { leap_voxon.set_rendmode(0); }		// setting the rendmode
            if (voxie_keystat(0x3)) { leap_voxon.set_rendmode(1); }
            if (voxie_keystat(0x4)) { leap_voxon.set_rendmode(2); }

            // calling Leap_Voxon for setting up leap motion service
            leap_voxon.getFrame();
            leap_voxon.track_event();
            /* end here */
#endif
            //draw wireframe box
            /* 27/08/18 */

            // For the base
            rr.x = 0.39f; dd.x = 0.00f; ff.x = 0.00f; pp.x = ((float) 0.0f);
            rr.y = 0.00f; dd.y = 0.39f; ff.y = 0.00f; pp.y = ((float) 0.0f);
            rr.z = 0.00f; dd.z = 0.00f; ff.z = 0.39f; pp.z = 0.0f;

            // End here : testing
            // checkPressed = menuBox.checkHitBox(pp, rr, dd, ff, indexPos);
            /*end here */
#if (USELEAP)

            /************************************************/
            /* adding Leap motion device, by Yi-Ting, Hsieh */

            if(leap_voxon.getNumHands()==1)
            {
                menuEvent(pp, rr, dd, ff, leap_voxon.getHand(), leap_voxon.getLeapScale(), &menubox);
                drawHands(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
            }
            else if(leap_voxon.getNumHands()==2)
            {
                menuEvent(pp, rr, dd, ff, leap_voxon.getRightHand(), leap_voxon.getLeapScale(), &menubox);
                drawHands(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                drawHands(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
            }
            else
            {
                menuEvent(pp, rr, dd, ff, leap_voxon.getHand(), leap_voxon.getLeapScale(), &menubox);
            }
#endif

          }

            /* end here */
            /************************************************/
            break;


        case RENDMODE_PHASESYNC: //'`': heightmap from UDP
            {
                point3d pp, rr, dd;

                if ((voxie_keystat(keyremap[0][4]) == 1) || (voxie_keystat(keyremap[0][5]) == 1)) { gphasesync_curmode ^= 1; }
                if ((voxie_keystat(keyremap[0][0]) == 1) && (gphasesync_cursc < 16)) gphasesync_cursc <<= 1;
                if ((voxie_keystat(keyremap[0][1]) == 1) && (gphasesync_cursc >  1)) gphasesync_cursc >>= 1;

                voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,+vw.aspx,+vw.aspy,+vw.aspz);
                if (!gphasesync_curmode)
                {
                    for(y=-16;y<=16;y+=16)
                        for(x=-16;x<16;x+=gphasesync_cursc*2)
                        {
                            f = 1.f; if ((gphasesync_cursc == 16) && (!y)) f = -1.f;
                            voxie_drawlin(&vf,(float)(x+gphasesync_cursc*0)*vw.aspx/16.f,(float)y*vw.aspy/16.f,-vw.aspz*f,
                                          (float)(x+gphasesync_cursc*1)*vw.aspx/16.f,(float)y*vw.aspy/16.f,+vw.aspz*f,0xffffff);
                            voxie_drawlin(&vf,(float)(x+gphasesync_cursc*1)*vw.aspx/16.f,(float)y*vw.aspy/16.f,+vw.aspz*f,
                                          (float)(x+gphasesync_cursc*2)*vw.aspx/16.f,(float)y*vw.aspy/16.f,-vw.aspz*f,0xffffff);

                            voxie_drawlin(&vf,(float)y*vw.aspx/16.f,(float)(x+gphasesync_cursc*0)*vw.aspy/16.f,-vw.aspz*f,
                                          (float)y*vw.aspx/16.f,(float)(x+gphasesync_cursc*1)*vw.aspy/16.f,+vw.aspz*f,0xffffff);
                            voxie_drawlin(&vf,(float)y*vw.aspx/16.f,(float)(x+gphasesync_cursc*1)*vw.aspy/16.f,+vw.aspz*f,
                                          (float)y*vw.aspx/16.f,(float)(x+gphasesync_cursc*2)*vw.aspy/16.f,-vw.aspz*f,0xffffff);
                        }
                }
                else
                {
                    for(j=0;j<=4;j++)
                    {
                        int m, o;
                        if ((gphasesync_cursc == 16) && (j == 2)) m = 0; else m = (vf.drawplanes>>1);
                        for(o=0;o<vw.dispnum;o++)
                        {
                            k = (((gyres-2)*j)>>2);
                            for(y=k;y<=k+1;y++)
                                for(x=0;x<gxres;x++)
                                {
                                    i = (((x*vf.drawplanes*16)/(gxres*gphasesync_cursc)+m)%vf.drawplanes); //if (vw.usecol > 0) i = div24[i]*24 + mod3[i]*8 + div3[mod24[i]];
                                    *(int *)(vf.fp*vw.framepervol*o + vf.fp*div24[i] + vf.p*y + x*4 + vf.f) |= oneupmod24[i];
                                }
                            k = (((gxres-2)*j)>>2);
                            for(x=k;x<=k+1;x++)
                                for(y=0;y<gyres;y++)
                                {
                                    i = (((y*vf.drawplanes*16)/(gyres*gphasesync_cursc)+m)%vf.drawplanes); //if (vw.usecol > 0) i = div24[i]*24 + mod3[i]*8 + div3[mod24[i]];
                                    *(int *)(vf.fp*vw.framepervol*o + vf.fp*div24[i] + vf.p*y + x*4 + vf.f) |= oneupmod24[i];
                                }
                        }
                    }
                }

                rr.x = 0.12f; dd.x = 0.00f;
                rr.y = 0.00f; dd.y = 0.16f;
                rr.z = 0.00f; dd.z = 0.00f;
                for(y=-1;y<=1;y+=2)
                    for(x=-1;x<=1;x+=2)
                    {
                        pp.x = rr.x*-1.50f + (float)x*.20f; pp.y = dd.y*-.50f + (float)y*.25f; pp.z = -vw.aspz; voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"TOP");
                        pp.x = rr.x*-1.50f + (float)x*.20f; pp.y = dd.y*-.50f + (float)y*.25f; pp.z =      0.f; voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"MID");
                        pp.x = rr.x*-1.50f + (float)x*.80f; pp.y = dd.y*-.50f + (float)y*.80f; pp.z =      0.f; voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"MID");
                        pp.x = rr.x*-1.50f + (float)x*.20f; pp.y = dd.y*-.50f + (float)y*.25f; pp.z = +vw.aspz; voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"BOT");
                    }
                /************************************************/
                /* adding Leap motion device, by Yi-Ting, Hsieh */
#if (USELEAP)

                // calling Leap_Voxon for setting up leap motion service
                leap_voxon.getFrame();
                leap_voxon.track_event();

                // Draw hands if there is any hand on Canvas
                if(leap_voxon.getNumHands()==1)
                {
                    drawHands(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                    checkExitGesture(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                }
                else if(leap_voxon.getNumHands()==2)
                {
                    drawHands(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                    drawHands(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                    checkExitGesture(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                    getIndexPos(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode(),.2);
                    // drawExitBox(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),.3);
                }

                if (checkExitBox == true || checkExitStatus == FIRE)
                {
                    grendmode = RENDMODE_TITLE; checkExitBox = false;
                    checkExitStatus = NOP;
                    leap_voxon.set_swipemode(false);

                    menubox.timer = 0;
                    menubox.status = 1;
                    menubox.delayTimer = DELAYTIME;
                    menubox.fromTop = false;
                }
#endif
                /* end here */
                /************************************************/
                showphase = 1;
            }
            break;

        case RENDMODE_KEYSTONECAL: //quadrilateral projection compensation
        {
            static int cornind = 0;
            point3d fp, a0, a1, b0, b1, ai, bi, ci;
            int igind = -1;

            voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,+vw.aspx,+vw.aspy,+vw.aspz);

            for(f=-0.5;f<=0.5;f+=0.5)
            {
                voxie_drawlin(&vf,-vw.aspx  , vw.aspy*f,-vw.aspz,vw.aspx  ,vw.aspy*f,-vw.aspz,0xffffff);
                voxie_drawlin(&vf,-vw.aspx  , vw.aspy*f,+vw.aspz,vw.aspx  ,vw.aspy*f,+vw.aspz,0xffffff);
                voxie_drawlin(&vf, vw.aspx*f,-vw.aspy  ,-vw.aspz,vw.aspx*f,vw.aspy  ,-vw.aspz,0xffffff);
                voxie_drawlin(&vf, vw.aspx*f,-vw.aspy  ,+vw.aspz,vw.aspx*f,vw.aspy  ,+vw.aspz,0xffffff);
            }
            for(f=-1.0;f<=1.0;f+=0.5)
                for(g=-1.0;g<=1.0;g+=0.5) voxie_drawlin(&vf,vw.aspx*f,vw.aspy*g,-vw.aspz,vw.aspx*f,vw.aspy*g,+vw.aspz,0xffffff);

            if (labs(vx[0].tx0) < 4096) vx[0].tx0 = 0;
            if (labs(vx[0].ty0) < 4096) vx[0].ty0 = 0;
            if (labs(vx[0].tx1) < 4096) vx[0].tx1 = 0;
            if (labs(vx[0].ty1) < 4096) vx[0].ty1 = 0;

            for(i=0;i<vw.dispnum;i++) { if (voxie_keystat(i+0x2) == 1) { vw.dispcur = i; voxie_init(&vw); } } //'1', '2'
            if ((bstatus&(~obstatus)&2) || (nav[0].but&(~onavbut[0])&2)) { vw.dispcur++; if (vw.dispcur >= vw.dispnum) { vw.dispcur = 0; } voxie_init(&vw); } //RMB

            //Projection compensation GUI
            if ((!(bstatus&1)) && (!(vx[0].but&0xf000)) && (!(nav[0].but&1)))
            {
                keystone.curx = min(max(keystone.curx + fx*.01f   + vx[0].tx0*+.0000001f + nav[0].dx*dtim*.01f,-vw.aspx),vw.aspx);
                keystone.cury = min(max(keystone.cury + fy*.01f   + vx[0].ty0*-.0000001f + nav[0].dy*dtim*.01f,-vw.aspy),vw.aspy);
                keystone.curz +=                        fz*-.001f + vx[0].ty1*-.0000001f + nav[0].dz*dtim*.01f;
                keystone.curz += ((voxie_keystat(keyremap[0][4]) || voxie_keystat(0xd1) || voxie_keystat(0x28))
                        - (voxie_keystat(keyremap[0][5]) || voxie_keystat(0xc9) || voxie_keystat(0x27)))*dtim; //ButA,PGDN - ButB,PGUP
                //if (bstatus&~obstatus&2) { if (keystone.curz >= 0.f) keystone.curz = -vw.aspz; else keystone.curz = vw.aspz; } //RMB sets cursor height to ceil/floor
                keystone.curz = min(max(keystone.curz,-vw.aspz),vw.aspz);

                fp.x = keystone.curx; fp.y = keystone.cury; fp.z = keystone.curz;
            }
            else
            {
                if ((!(obstatus&1)) && (!(ovxbut[0]&0xf000)))
                {
                    switch(vw.flip)
                    {
                    case 0: cornind = ((keystone.curx >= 0.f) ^ (keystone.cury >= 0.f))*2 + (keystone.cury >= 0.f)*4; break;
                    case 1: cornind = ((keystone.cury <  0.f) ^ (keystone.curx >= 0.f))*2 + (keystone.curx >= 0.f)*4; break;
                    case 2: cornind = ((keystone.curx <  0.f) ^ (keystone.cury <  0.f))*2 + (keystone.cury <  0.f)*4; break;
                    case 3: cornind = ((keystone.cury >= 0.f) ^ (keystone.curx <  0.f))*2 + (keystone.curx <  0.f)*4; break;
                    }
                    cornind += (keystone.curz > 0.f);
                }
                switch(vw.flip)
                {
                case 0: vw.disp[vw.dispcur].keyst[cornind].x += (fx*.01f + vx[0].tx0*+.0000001f + nav[0].dx*dtim*.01f)*32.f;
                    vw.disp[vw.dispcur].keyst[cornind].y += (fy*.01f + vx[0].ty0*-.0000001f + nav[0].dy*dtim*.01f)*32.f; break;
                case 1: vw.disp[vw.dispcur].keyst[cornind].x -= (fy*.01f + vx[0].ty0*+.0000001f + nav[0].dy*dtim*.01f)*32.f;
                    vw.disp[vw.dispcur].keyst[cornind].y += (fx*.01f + vx[0].tx0*-.0000001f + nav[0].dx*dtim*.01f)*32.f; break;
                case 2: vw.disp[vw.dispcur].keyst[cornind].x -= (fx*.01f + vx[0].tx0*+.0000001f + nav[0].dx*dtim*.01f)*32.f;
                    vw.disp[vw.dispcur].keyst[cornind].y -= (fy*.01f + vx[0].ty0*-.0000001f + nav[0].dy*dtim*.01f)*32.f; break;
                case 3: vw.disp[vw.dispcur].keyst[cornind].x += (fy*.01f + vx[0].ty0*+.0000001f + nav[0].dy*dtim*.01f)*32.f;
                    vw.disp[vw.dispcur].keyst[cornind].y -= (fx*.01f + vx[0].tx0*-.0000001f + nav[0].dx*dtim*.01f)*32.f; break;
                }
                voxie_init(&vw);

                igind = cornind;
                fp.x = ((float)(keystone.curx >= 0)*2.f-1.f)*vw.aspx;
                fp.y = ((float)(keystone.cury >= 0)*2.f-1.f)*vw.aspy;
                fp.z = ((float)(keystone.curz >= 0)*2.f-1.f)*vw.aspz;
            }

            if (bstatus&4) //MMB: refine cal settings by minimizing bad degrees of freedom
            {
                point2d nproj[8];
                int bad, good;
                memcpy(nproj,&vw.disp[vw.dispcur].keyst,sizeof(nproj));

                for(i=1;i<4;i++)
                    for(j=0;j<i;j++)
                    {
                        f = -64.f;
                        a0.x = nproj[i*2].x; a1.x = (nproj[i*2+1].x-a0.x)*f + a0.x;
                        a0.y = nproj[i*2].y; a1.y = (nproj[i*2+1].y-a0.y)*f + a0.y;
                        a0.z = -256.f      ; a1.z = (+256.f        -a0.z)*f + a0.z;
                        b0.x = nproj[j*2].x; b1.x = (nproj[j*2+1].x-b0.x)*f + b0.x;
                        b0.y = nproj[j*2].y; b1.y = (nproj[j*2+1].y-b0.y)*f + b0.y;
                        b0.z = -256.f      ; b1.z = (+256.f        -b0.z)*f + b0.z;
                        roundcylminpath(&a0,&a1,&b0,&b1,&ai,&bi);

                        if (igind*2+0 == cornind) f = 1.0f;
                        else if (igind*2+1 == cornind) f = 0.0f;
                        else                           f = 0.5f;
                        ci.x = (bi.x-ai.x)*f + ai.x;
                        ci.y = (bi.y-ai.y)*f + ai.y;
                        ci.z = (bi.z-ai.z)*f + ai.z;

                        //(nproj[i*2+1].x-a0.x)*f + a0.x = ci.x
                        //(nproj[i*2+1].y-a0.y)*f + a0.y = ci.y
                        //(nproj[i*2+1].z-a0.z)*f + a0.z = ci.z
                        f = (+256.f-a0.z)/(ci.z-a0.z);
                        nproj[i*2+1].x = (ci.x-a0.x)*f + a0.x;
                        nproj[i*2+1].y = (ci.y-a0.y)*f + a0.y;
                        f = (+256.f-b0.z)/(ci.z-b0.z);
                        nproj[j*2+1].x = (ci.x-b0.x)*f + b0.x;
                        nproj[j*2+1].y = (ci.y-b0.y)*f + b0.y;
                    }

                bad = 0; good = 0;
                for(i=8-1;i>=0;i--)
                {
                    if (((*(int *)&nproj[i].x)&0x7f800000) == 0x7f800000) bad = 1; //detect NaN
                    if (((*(int *)&nproj[i].y)&0x7f800000) == 0x7f800000) bad = 1; //detect NaN
                    if (fabs(nproj[i].x) > 4096) bad = 1;
                    if (fabs(nproj[i].y) > 4096) bad = 1;
                    if (nproj[0].x != nproj[i].x) good |= 1;
                    if (nproj[0].y != nproj[i].y) good |= 2;
                }
                if ((!bad) && (good == 3)) //This check never verifiably worked!
                {
                    memcpy(&vw.disp[vw.dispcur].keyst,nproj,sizeof(nproj));
                    voxie_init(&vw);
                }
            }

            f = .03f; //Draw cursor
            voxie_drawbox(&vf,fp.x-f,fp.y-f,fp.z-f,
                          fp.x+f,fp.y+f,fp.z+f,2,0x00ffff);

            //Don't print text if projection is in messed up state
            //0   2
            //
            //6   4
            //0&4 must be on opposite sides of the 2-6 line
            //2&6 must be on opposite sides of the 0-4 line
            for(i=2-1;i>=0;i--)
            {
                f =  (vw.disp[vw.dispcur].keyst[i+4].x-vw.disp[vw.dispcur].keyst[i+0].x); g =(vw.disp[vw.dispcur].keyst[i+4].y-vw.disp[vw.dispcur].keyst[i+0].y);
                if (((vw.disp[vw.dispcur].keyst[i+2].x-vw.disp[vw.dispcur].keyst[i+0].x)*g < (vw.disp[vw.dispcur].keyst[i+2].y-vw.disp[vw.dispcur].keyst[i+0].y)*f) ==
                        ((vw.disp[vw.dispcur].keyst[i+6].x-vw.disp[vw.dispcur].keyst[i+0].x)*g < (vw.disp[vw.dispcur].keyst[i+6].y-vw.disp[vw.dispcur].keyst[i+0].y)*f)) break;
                f =  (vw.disp[vw.dispcur].keyst[i+6].x-vw.disp[vw.dispcur].keyst[i+2].x); g =(vw.disp[vw.dispcur].keyst[i+6].y-vw.disp[vw.dispcur].keyst[i+2].y);
                if (((vw.disp[vw.dispcur].keyst[i+0].x-vw.disp[vw.dispcur].keyst[i+2].x)*g < (vw.disp[vw.dispcur].keyst[i+0].y-vw.disp[vw.dispcur].keyst[i+2].y)*f) ==
                        ((vw.disp[vw.dispcur].keyst[i+4].x-vw.disp[vw.dispcur].keyst[i+2].x)*g < (vw.disp[vw.dispcur].keyst[i+4].y-vw.disp[vw.dispcur].keyst[i+2].y)*f)) break;
            }
            if (i < 0)
                for(i=8-1;i>=0;i--)
                {
                    point3d pp, rr, dd;

                    rr.x = 0.12; dd.x = 0.00;
                    rr.y = 0.00; dd.y = 0.16;
                    rr.z = 0.00; dd.z = 0.00;

                    if (!((i+2)&4)) pp.x = -vw.aspx; else pp.x = vw.aspx-1.f;
                    pp.y = ((i>>2)*5+(i&1))*0.25-0.92;
                    pp.z = ((float)(i&1)-.5f)*vw.aspz*2.f;

                    voxie_printalph_(&vf,&pp,&rr,&dd,0xffff00+(i&1)*(0x00ffff-0xffff00),"%7.2f",vw.disp[vw.dispcur].keyst[i].x); pp.y += 0.20;
                    voxie_printalph_(&vf,&pp,&rr,&dd,0xffff00+(i&1)*(0x00ffff-0xffff00),"%7.2f",vw.disp[vw.dispcur].keyst[i].y);
                }

            {
                point3d pp, rr, dd;
                pp.x = 0.0f; rr.x = 0.12f; dd.x = 0.00f;
                pp.y = 0.0f; rr.y = 0.00f; dd.y = 0.16f;
                pp.z = 0.0f; rr.z = 0.00f; dd.z = 0.00f;
                voxie_printalph_(&vf,&pp,&rr,&dd,(rand()<<15)+rand(),"%d/%d",vw.dispcur,vw.dispnum);
            }


            /************************************************/
            /* adding Leap motion device, by Yi-Ting, Hsieh */
#if (USELEAP)

            // calling Leap_Voxon for setting up leap motion service
            leap_voxon.getFrame();
            leap_voxon.track_event();

            // Draw hands if there is any hand on Canvas
            if(leap_voxon.getNumHands()==1)
            {
                drawHands(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                checkExitGesture(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());

            }
            else if(leap_voxon.getNumHands()==2)
            {
                drawHands(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                drawHands(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                checkExitGesture(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                // getIndexPos(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode(),.2);
                // drawExitBox(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),.3);
            }

            if (checkExitBox == true || checkExitStatus == FIRE)
            {
                grendmode = RENDMODE_TITLE; checkExitBox = false;
                checkExitStatus = NOP;
                leap_voxon.set_swipemode(false);

                menubox.timer = 0;
                menubox.status = 1;
                menubox.delayTimer = DELAYTIME;
                menubox.fromTop = false;
            }

#endif
            /* end here */
            /************************************************/

        }
            break;


            //--------------------------------------------------------------------------------------------------

        case RENDMODE_HEIGHTMAP: //render heightmap from file
        {
            static float avghgt = -1.f;
            static int autoadj = 1;
            float fval;

            if (labs(vx[0].tx0) < 4096) vx[0].tx0 = 0;
            if (labs(vx[0].ty0) < 4096) vx[0].ty0 = 0;
            if (labs(vx[0].tx1) < 4096) vx[0].tx1 = 0;
            if (labs(vx[0].ty1) < 4096) vx[0].ty1 = 0;

            //Cycle image
            if ((voxie_keystat(0xc9) == 1) || (ghitkey == 0xc9) || (voxie_keystat(0x27) == 1) || ((vx[0].but&~ovxbut[0])&((1<<14)|(1<<8))) || (nav[0].but&~onavbut[0]&1)) { ghitkey = 0; gdemi--; if (gdemi <      0) gdemi = max(gdemn-1,0); cycletim = tim+5.0; } //PGUP
            if ((voxie_keystat(0xd1) == 1) || (ghitkey == 0xd1) || (voxie_keystat(0x28) == 1) || ((vx[0].but&~ovxbut[0])&((1<<13)|(1<<9))) || (nav[0].but&~onavbut[0]&2)) { ghitkey = 0; gdemi++; if (gdemi >= gdemn) gdemi =              0; cycletim = tim+5.0; } //PGDN
            if ((gautocycle) && (tim > cycletim))
            {
                gdemi++; if (gdemi >= gdemn) gdemi = 0;
                cycletim = tim+5.0;
            }

            if ((voxie_keystat(0x35) == 1) || (ghitkey == 0x0e) || (voxie_keystat(0x0e) == 1) || ((vx[0].but&~ovxbut[0])&(1<<4))) //'/' or Backspace:Reset view
            {
                ghitkey = 0;
                gdem[gdemi].p = gdem[gdemi].sp;
                gdem[gdemi].r = gdem[gdemi].sr;
                gdem[gdemi].d = gdem[gdemi].sd;
                gdem[gdemi].f = gdem[gdemi].sf;
            }

            //auto-adjust height smoothly, using avghgt from previous voxie_drawheimap()
            //if (voxie_keystat(0x32) == 1) { autoadj = !autoadj; } //M:toggle autoadj
            if ((autoadj) && (avghgt >= 0.f) && (avghgt != 0x80000000))
            {
                //gdem[gdemi].p.z + gdem[gdemi].f.z*avghgt/256.0 = 0.0
                gdem[gdemi].p.z += (gdem[gdemi].f.z*avghgt/-256.f - gdem[gdemi].p.z)*.125f;
            }

            //Move (scroll/pan)
            gdem[gdemi].p.x += - nav[0].dx*dtim*.012f - nav[0].ax*dtim*.008f; //Mouse
            gdem[gdemi].p.y += - nav[0].dy*dtim*.012f - nav[0].ay*dtim*.008f;
            gdem[gdemi].p.x += ((float)((voxie_keystat(keyremap[0][0])!=0) - (voxie_keystat(keyremap[0][1])!=0)))*dtim; //Rig - Lef
            gdem[gdemi].p.y += ((float)((voxie_keystat(keyremap[0][2])!=0) - (voxie_keystat(keyremap[0][3])!=0)))*dtim; // Up - Dow
            //if (voxie_keystat(0x1e)) { gdem[gdemi].p.z += dtim*128.f/gdem[gdemi].f.z; } //A
            //if (voxie_keystat(0x1f)) { gdem[gdemi].p.z -= dtim*128.f/gdem[gdemi].f.z; } //S


            i = vx[0].tx0 + vx[0].tx1;
            j = vx[0].ty0 + vx[0].ty1;
            if (i|j)
            {
                f = ((float)i)*+.0002f;
                g = ((float)j)*-.0002f;
                gdem[gdemi].p.x += gdem[gdemi].r.x*f + gdem[gdemi].d.x*g;
                gdem[gdemi].p.y += gdem[gdemi].r.y*f + gdem[gdemi].d.y*g;
            }

            fval = (float)(((voxie_keystat(0x34)!=0) || voxie_keystat(keyremap[1][1])) -
                    ((voxie_keystat(0x33)!=0) || voxie_keystat(keyremap[1][0])))*dtim;
            fval += nav[0].az*dtim*.008f;
            if (fval != 0.f)
            {
                float c, s, rd, u, v;

                fx = gdem[gdemi].r.x; gx = gdem[gdemi].d.x;
                fy = gdem[gdemi].r.y; gy = gdem[gdemi].d.y;
                gz = gdem[gdemi].f.z;

                c = cos(fval); s = sin(fval);
                g = gdem[gdemi].r.x;
                gdem[gdemi].r.x = gdem[gdemi].r.x*c + gdem[gdemi].r.y*s;
                gdem[gdemi].r.y = gdem[gdemi].r.y*c -               g*s;
                g = gdem[gdemi].d.x;
                gdem[gdemi].d.x = gdem[gdemi].d.x*c + gdem[gdemi].d.y*s;
                gdem[gdemi].d.y = gdem[gdemi].d.y*c -               g*s;

                rd = 1.f/(fx*gy - fy*gx);
                u = (gdem[gdemi].p.x*gy - gdem[gdemi].p.y*gx)*rd;
                v = (gdem[gdemi].p.y*fx - gdem[gdemi].p.x*fy)*rd;
                gdem[gdemi].p.x = gdem[gdemi].r.x*u + gdem[gdemi].d.x*v;
                gdem[gdemi].p.y = gdem[gdemi].r.y*u + gdem[gdemi].d.y*v;
            }


            //Zoom, preserving center
            fval = 1.f;
            if (voxie_keystat(keyremap[0][4])) fval *= pow(4.00,(double)dtim); //ButA
            if (voxie_keystat(keyremap[0][5])) fval *= pow(0.25,(double)dtim); //ButB
            if (vx[0].lt) fval /= pow(1.0+vx[0].lt/256.0,(double)dtim); //Xbox
            if (vx[0].rt) fval *= pow(1.0+vx[0].rt/256.0,(double)dtim); //Xbox
            if (fz != 0.f) fval *= pow(0.5,(double)fz*.001f); //dmousz


            /************************************************/
            /* adding Leap motion device, by Yi-Ting, Hsieh */
#if (USELEAP)
            // Check Event
            // Zoom in: Right hand Pinch
            // Zoom out: Left hand Pinch
            if (leap_voxon.getEventType() != nullptr)
            {
                // if (strcmp(leap_voxon.getEventType(),"Pinch")==0)
                // {
                // 		if(leap_voxon.getHand()->type==eLeapHandType_Right){ fval *= pow(4.00,(double)dtim); }
                // 		else if(leap_voxon.getHand()->type==eLeapHandType_Left){ fval *= pow(0.25,(double)dtim); }
                // }

                /* 08/18/2018 : new Update:
                              Tracking the palm position to zoom in/out*/
                if (strcmp(leap_voxon.getEventType(),"Grab")==0)
                {
                    if ((float)((&(&leap_voxon.getHand()->palm)->position)->y) < 150) { fval *= pow(4.00,(double)dtim); }
                    else if ((float)((&(&leap_voxon.getHand()->palm)->position)->y) > 250) { fval *= pow(0.25,(double)dtim); }
                }
            }

#endif

            /* end here */
            /************************************************/

            fval += nav[0].dz*dtim*.005f;
            if (fval != 1.f)
            {
                float rd, u, v;
                fx = gdem[gdemi].r.x; gx = gdem[gdemi].d.x;
                fy = gdem[gdemi].r.y; gy = gdem[gdemi].d.y;
                gz = gdem[gdemi].f.z;

                gdem[gdemi].r.x *= fval; gdem[gdemi].d.x *= fval;
                gdem[gdemi].r.y *= fval; gdem[gdemi].d.y *= fval;
                gdem[gdemi].f.z *= fval;

                //0 = gdem[gdemi].p.x + fx*u + gx*v
                //0 = gdem[gdemi].p.y + fy*u + gy*v
                //0 = gdem[gdemi].p.x'+ gdem[gdemi].r.x*u + gdem[gdemi].d.x*v
                //0 = gdem[gdemi].p.y'+ gdem[gdemi].r.y*u + gdem[gdemi].d.y*v

                //fx*u + fy*v = gdem[gdemi].p.x
                //gx*u + gy*v = gdem[gdemi].p.y
                rd = 1.f/(fx*gy - fy*gx);
                u = (gdem[gdemi].p.x*gy - gdem[gdemi].p.y*gx)*rd;
                v = (gdem[gdemi].p.y*fx - gdem[gdemi].p.x*fy)*rd;
                gdem[gdemi].p.x = gdem[gdemi].r.x*u + gdem[gdemi].d.x*v;
                gdem[gdemi].p.y = gdem[gdemi].r.y*u + gdem[gdemi].d.y*v;

                //gdem[gdemi].p.z_old +              gz*f/256.0 = 0.0
                //gdem[gdemi].p.z     + gdem[gdemi].f.z*f/256.0 = 0.0
                gdem[gdemi].p.z *= gdem[gdemi].f.z/gz;
            }

            if (voxie_keystat(keyremap[1][4]) || voxie_keystat(keyremap[1][5]) || voxie_keystat(0xb5) || voxie_keystat(0x37))
            {
                gz = gdem[gdemi].f.z;

                f = 1.0;
                if (voxie_keystat(keyremap[1][5])) f *= pow(0.25,(double)dtim);
                if (voxie_keystat(keyremap[1][4])) f *= pow(4.00,(double)dtim);
                if (voxie_keystat(0x37)) f *= pow(0.25,(double)dtim);
                if (voxie_keystat(0xb5)) f *= pow(4.00,(double)dtim);
                gdem[gdemi].f.z /= f;

                gdem[gdemi].p.z *= gdem[gdemi].f.z/gz;
            }

            if (voxie_keystat(0x3b) == 1) { gheightmap_flags ^= 1; }
            if (voxie_keystat(0x3c) == 1) { gheightmap_flags ^= 2; }
            if (voxie_keystat(0x3d) == 1) { gheightmap_flags ^= 4; }
            if (voxie_keystat(0x3f) == 1) { gheightmap_flags ^= 16; }
            if (gdem[gdemi].mapzen) gheightmap_flags |= 32; else gheightmap_flags &= ~32;


            voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,+vw.aspx,+vw.aspy,+vw.aspz);
            avghgt = voxie_drawheimap(&vf,gdem[gdemi].file,&gdem[gdemi].p,&gdem[gdemi].r,&gdem[gdemi].d,&gdem[gdemi].f, gdem[gdemi].colorkey,0 /*reserved*/,gheightmap_flags);
            if (avghgt == 0x80000000)
            {
                //Display error if bad image
                point3d pp, rr, dd;
                pp.x = 0.0; rr.x = 20.0; dd.x =   0.0;
                pp.y = 0.0; rr.y =  0.0; dd.y =  60.0;
                pp.z = 0.0; rr.z =  0.0; dd.z =   0.0;
                voxie_setview(&vf,0.0,0.0,-256,gxres,gyres,+256); //old coords
                voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%s not found",gdem[gdemi].file);
            }


                        /************************************************/
                        /* adding Leap motion device, by Yi-Ting, Hsieh */
              #if (USELEAP)
                        // calling Leap_Voxon for setting up leap motion service
                        leap_voxon.getFrame();
                        leap_voxon.track_event();

                        // // Draw hands if there is any hand on Canvas
                        if(leap_voxon.getNumHands()==1)
                        {
                            checkExitGesture(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                        }
                        else if(leap_voxon.getNumHands()==2)
                        {
                            // drawHands(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                            // drawHands(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                            checkExitGesture(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                            getIndexPos(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode(),.2);
                            drawExitBox(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),.3);
                        }


                        // Check Event
                        if (leap_voxon.getEventType() != nullptr)
                        {
                            if (strcmp(leap_voxon.getEventType(),"Grab")==0)
                            {
                                gdem[gdemi].p.x += ((float)(((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->x/20)*(-.01))); //Rig - Lef
                                gdem[gdemi].p.y += ((float)(((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->z/20)*(-.01))); // Up - Dow
                            }
                        }

                        if (checkExitBox == true || checkExitStatus == FIRE)
                        {
                            grendmode = RENDMODE_TITLE; checkExitBox = false;
                            checkExitStatus = NOP;
                            leap_voxon.set_swipemode(false);

                            menubox.timer = 0;
                            menubox.status = 1;
                            menubox.delayTimer = DELAYTIME;
                            menubox.fromTop = false;
                        }

              #endif
        }
            break;

            // --------------------------------------------------------------------------------------------------

        case RENDMODE_CHESS: //Chess
        {
            static const char *chessnam[] = {"pawn.kv6","knight.kv6","bishop.kv6","rook.kv6","queen.kv6","king.kv6"};
            static const float heioff[6] = {+.0758f,+.0375f,+.0250f,+.0475f,+.0138f,-.0113f};
            static const float sizoff[6] = {0.225,0.300,0.325,0.275,0.350,0.400};
            static double movetim = -1e32, timig = -1e32;
            static int inited = -1, cursx, cursy, fromx, fromy, tox, toy, caststat, prevmove, turn, win;
            static int board[8][8], sboard[8][8] =
            {
                4, 2, 3, 5, 6, 3, 2, 4,
                1, 1, 1, 1, 1, 1, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                -1,-1,-1,-1,-1,-1,-1,-1,
                -4,-2,-3,-5,-6,-3,-2,-4,
            };

            f = 1.f/min(min(vw.aspx,vw.aspy),vw.aspz/.2f);
            //voxie_setview(&vf,-vw.aspx*f,-vw.aspy*f,-vw.aspz*f,+vw.aspx*f,+vw.aspy*f,+vw.aspz*f); //preserve cube aspect
            //voxie_setview(&vf,-1.f,-1.f,-.2f,+1.f,+1.f,+.2f); //ignore cube aspect and fill space
            voxie_setview(&vf,(-vw.aspx*f-1.f)*.5f,(-vw.aspy*f-1.f)*.5f,(-vw.aspz*f-.2f)*.5f,
                          (+vw.aspx*f+1.f)*.5f,(+vw.aspy*f+1.f)*.5f,(+vw.aspz*f+.2f)*.5f); //average of above methods

            if ((voxie_keystat(0x0e) == 1) || (ghitkey == 0x0e) || (voxie_keystat(0x1c) == 1) && (voxie_keystat(0x1d)|voxie_keystat(0x9d))) //Backspace,Ctrl+Enter:reset game
            {
                ghitkey = 0;
                inited = 0; voxie_playsound("opendoor.flac",-1,100,100,1.f);
            }
            if (inited <= 0)
            {
                if (inited < 0) { voxie_mountzip("chessdat.zip"); voxie_playsound("opendoor.flac",-1,100,100,1.f); }

                memcpy(board,sboard,sizeof(board));
                cursx = 4; cursy = 1; fromx = -1; tox = -1;

                //if (caststat&1): no white long  castle
                //if (caststat&2): no white short castle
                //if (caststat&4): no black long  castle
                //if (caststat&8): no black short castle
                caststat = 0; prevmove = -1; turn = 0; win = -1;

                inited = 1;
            }

            // #if (USELEAP)
            // 				// frame = leap_getframe();
            // 				// if ((frame) && (frame->nHands > 0))
            // 				// {
            // 				// 	static int onearboard = 0, nearboard = 0;
            // 				//
            // 				// 	hand = &frame->pHands[0];
            // 				// 	digit = &hand->digits[1];
            // 				// 	//vec = &digit->stabilized_tip_position;
            // 				// 	vec = &digit->bones[3].next_joint;
            // 				//
            // 				// 	fx = vec->x*+.01f    ; fx = min(max(fx,-vw.aspx),vw.aspx);
            // 				// 	fy = vec->z*+.01f    ; fy = min(max(fy,-vw.aspy),vw.aspy);
            // 				// 	fz = vec->y*-.01f+2.f; fz = min(max(fz,-vw.aspz),vw.aspz);
            // 				// 	voxie_drawsph(&vf,fx,fy,fz,.05f,1,0xffffff);
            // 				// 	onearboard = nearboard; nearboard = (fz >= min(vw.aspz,0.2f));
            // 				// 	if ((nearboard > onearboard) && (tox < 0))
            // 				// 	{
            // 				// 		cursx = (int)((1.f+fx)*4.f);
            // 				// 		cursy = (int)((1.f-fy)*4.f);
            // 				// 		if ((cursx != fromx) || (cursy != fromy)) goto chesselect;
            // 				// 	}
            // 				// }
            // #endif

            /************************************************/
            /* adding Leap motion device, by Yi-Ting, Hsieh */
#if (USELEAP)
            // calling Leap_Voxon for setting up leap motion service
            leap_voxon.getFrame();
            leap_voxon.track_event();

            // Draw hands if there is any hand on Canvas
            if(leap_voxon.getNumHands()==1)
            {
                drawHands(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                getIndexPos(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode(),.2);
                checkExitGesture(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());

            }
            else if(leap_voxon.getNumHands()==2)
            {
                drawHands(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                drawHands(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                getIndexPos(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode(),.2);
                // drawExitBox(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),.3);
                checkExitGesture(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());

            }

            static int onearboard = 0, nearboard = 0;
            fx = indexPos.x;
            fy = indexPos.y;
            fz = indexPos.z;
            voxie_drawsph(&vf,indexPos.x,indexPos.y,indexPos.z,.05f,1,0xffffff);
            onearboard = nearboard; nearboard = (fz >= min(vw.aspz,0.2f));
            if ((nearboard > onearboard) && (tox < 0))
            {
                cursx = (int)((1.f+fx)*4.f);
                cursy = (int)((1.f-fy)*4.f);
                if ((cursx != fromx) || (cursy != fromy)) goto chesselect;
            }


            // Check if user want to exit the application
            if (checkExitBox == true || checkExitStatus == FIRE)
            {
                grendmode = RENDMODE_TITLE; checkExitBox = false;
                checkExitStatus = NOP;
                leap_voxon.set_swipemode(false);

                menubox.timer = 0;
                menubox.status = 2;
                menubox.delayTimer = DELAYTIME;
                menubox.fromTop = false;
            }

#endif
            /* end here */
            /************************************************/

            if (tim > timig)
            {
                if ((min(vx[0].tx0,vx[0].tx1) < -16384) || (nav[0].dx < -100.0f)) { timig = tim+0.25; cursx = max(cursx-1,  0); }
                if ((max(vx[0].tx0,vx[0].tx1) > +16384) || (nav[0].dx > +100.0f)) { timig = tim+0.25; cursx = min(cursx+1,8-1); }
                if ((max(vx[0].ty0,vx[0].ty1) > +16384) || (nav[0].dy < -100.0f)) { timig = tim+0.25; cursy = min(cursy+1,8-1); }
                if ((min(vx[0].ty0,vx[0].ty1) < -16384) || (nav[0].dy > +100.0f)) { timig = tim+0.25; cursy = max(cursy-1,  0); }
            }
            if ((voxie_keystat(keyremap[0][0]) == 1) || (voxie_keystat(keyremap[2][0]) == 1) || ((vx[0].but&~ovxbut[0])&4)) { cursx = max(cursx-1,  0); }
            if ((voxie_keystat(keyremap[0][1]) == 1) || (voxie_keystat(keyremap[2][1]) == 1) || ((vx[0].but&~ovxbut[0])&8)) { cursx = min(cursx+1,8-1); }
            if ((voxie_keystat(keyremap[0][2]) == 1) || (voxie_keystat(keyremap[2][2]) == 1) || ((vx[0].but&~ovxbut[0])&1)) { cursy = min(cursy+1,8-1); }
            if ((voxie_keystat(keyremap[0][3]) == 1) || (voxie_keystat(keyremap[2][3]) == 1) || ((vx[0].but&~ovxbut[0])&2)) { cursy = max(cursy-1,  0); }
            if (((voxie_keystat(keyremap[0][4]) == 1) || (voxie_keystat(0x1c) == 1) || (voxie_keystat(0x9c) == 1) || ((vx[0].but&~ovxbut[0])&0xf3c0) || (nav[0].but&~onavbut[0]&1)) && (tox < 0)) //ButA, Enter
            {
chesselect:;        if ((fromx == cursx) && (fromy == cursy)) fromx = -1;
                else if (ksgn(board[cursy][cursx]) == 1-turn*2) { fromx = cursx; fromy = cursy; voxie_playsound("c:/windows/media/recycle.wav",-1,500,500,3.f); }
                else if (fromx >= 0)
                {
                    if (isvalmove(board,caststat,prevmove,fromx,fromy,cursx,cursy))
                    {
                        i = labs(board[fromy][fromx]);
                        if (i == 1) voxie_playsound("zipguns.flac",-1,100,100,1.f);
                        if (i == 2) voxie_playsound("shoot2.flac",-1,100,100,1.f);
                        if (i == 3) voxie_playsound("shoot2.flac",-1,100,100,1.f);
                        if (i == 4) voxie_playsound("shoot2.flac",-1,100,100,1.f);
                        if (i == 5) voxie_playsound("shoot3.flac",-1,100,100,1.f);
                        if (i == 6) voxie_playsound("warp.flac",-1,100,100,1.f);
                        tox = cursx; toy = cursy; movetim = tim;
                    }
                    else
                    {
                        int kx, ky, t;
                        t = 1-turn*2;
                        for(ky=0;ky<8;ky++)
                            for(kx=0;kx<8;kx++)
                            {
                                if (board[ky][kx] != t*6) continue;
                                if (ischeck(board,kx,ky,turn))
                                { voxie_playsound("alarm.flac",-1,100,100,1.f); goto break2; }
                            }
                        voxie_playsound("bouncy.flac",-1,100,100,1.f);
break2:;          }
                }
            }

            if ((voxie_keystat(0x23) == 1) && (voxie_keystat(0x1d) || voxie_keystat(0x9d))) gchess_automove = !gchess_automove; //Ctrl+H
            if (((gchessailev[turn]) || (gchess_automove) || ((voxie_keystat(0x23) == 1) || (gchess_hint))) && (fromx < 0) && (win < 0)) //H:hint
            {
                gchess_hint = 0;
                i = gchessailev[turn]; if (!i) i = gchessailev[turn^1];
                if (!getcompmove(board,&caststat,&prevmove,turn,&fromx,&fromy,&tox,&toy,i)) goto dowin;
                movetim = voxie_klock();

                i = labs(board[fromy][fromx]);
                if (i == 1) voxie_playsound("zipguns.flac",-1,100,100,1.f);
                if (i == 2) voxie_playsound("shoot2.flac",-1,100,100,1.f);
                if (i == 3) voxie_playsound("shoot2.flac",-1,100,100,1.f);
                if (i == 4) voxie_playsound("shoot2.flac",-1,100,100,1.f);
                if (i == 5) voxie_playsound("shoot3.flac",-1,100,100,1.f);
                if (i == 6) voxie_playsound("warp.flac",-1,100,100,1.f);
            }
            if ((gchess_automove) && (win >= 0) && (tim >= movetim+10.0)) //restart in auto mode
            { inited = 0; break; }

            n = getvalmoves(board,caststat,prevmove,turn,0);
            if (n == 0)
            {
dowin:;        if (win < 0)
                    for(y=0;y<8;y++)
                        for(x=0;x<8;x++)
                            if (board[y][x] == (1-turn*2)*6)
                            {
                                if (!ischeck(board,x,y,turn)) //stalemate
                                { win = 2; voxie_playsound("shoot4.flac",-1,100,100,1.f); }
                                else { win = 1-turn; voxie_playsound("closdoor.flac",-1,100,100,1.f); }
                                goto foundking1;
                            }
foundking1:;}

            if ((voxie_keystat(keyremap[0][5]) == 1) || ((vx[0].but&~ovxbut[0])&(1<<4))) //Backspace:reset
            {
                if (gmoven >= 2)
                {
                    gmoven -= 2; voxie_playsound("ouch.flac",-1,100,100,1.f);

                    for(y=0;y<8;y++) for(x=0;x<8;x++) board[y][x] = sboard[y][x];
                    turn = 0; win = -1; caststat = 0; prevmove = -1;
                    for(i=0;i<gmoven;i++)
                    {
                        move2xys(gmove[i],&fromx,&fromy,&tox,&toy);
                        domove(board,&caststat,&prevmove,fromx,fromy,tox,toy,0);
                        turn = 1-turn;
                    }
                    fromx = -1; tox = -1;
                }
            }

            //Draw grid lines
            //for(f=-1.f;f<=1.f;f+=.25f)
            //{
            //   voxie_drawlin(&vf,f,-1.f,.195f,f,+1.f,.195f,0xffffff);
            //   voxie_drawlin(&vf,-1.f,f,.195f,+1.f,f,.195f,0xffffff);
            //}

            for(y=0;y<8;y++)
                for(x=0;x<8;x++)
                {
                    fx = ((float)x-3.5f)* .25f;
                    fy = ((float)y-3.5f)*-.25f;

                    //if ((x+y)&1) voxie_drawbox(&vf,fx-.12f,fy-.12f,.195f,fx+.12f,fy+.12f,.199f,2,0x0000ff); //filled squares (old; too bright)
                    //voxie_drawbox(&vf,fx-.12f,fy-.12f,.195f,fx+.12f,fy+.12f,.199f,2,((x+y)&1)*(0xffff00-0x00ffff) + 0x00ffff); //filled squares (old; too bright)
                    //voxie_drawbox(&vf,fx-.11f,fy-.11f,.17f,fx+.11f,fy+.11f,.22f,3,((x+y)&1)*(0x101000-0x001010) + 0x001010); //solid-filled boxes
                    //for(f=-.11f;f<=.11f;f+=.016f) voxie_drawlin(&vf,fx+f,fy-.11f,.199f,fx+f,fy+.11f,.199f,((x+y)&1)*(0xffff00-0x00ffff) + 0x00ffff); //filled squares (new)

                    poltex_t ptx[4];
                    int mesh[5];
                    f = 0.11f;
                    ptx[0].x = fx-f; ptx[0].y = fy-f; ptx[0].z = 0.199f; ptx[0].u = 0.f; ptx[0].v = 0.f; ptx[0].col = 0xffffff;
                    ptx[1].x = fx+f; ptx[1].y = fy-f; ptx[1].z = 0.199f; ptx[1].u = 1.f; ptx[1].v = 0.f; ptx[1].col = 0xffffff;
                    ptx[2].x = fx+f; ptx[2].y = fy+f; ptx[2].z = 0.199f; ptx[2].u = 1.f; ptx[2].v = 1.f; ptx[2].col = 0xffffff;
                    ptx[3].x = fx-f; ptx[3].y = fy+f; ptx[3].z = 0.199f; ptx[3].u = 0.f; ptx[3].v = 1.f; ptx[3].col = 0xffffff;
                    mesh[0] = 0; mesh[1] = 1; mesh[2] = 2; mesh[3] = 3; mesh[4] = -1;
                    voxie_drawmeshtex(&vf,0 /*"klab\\hole.png"*/,ptx,4,mesh,5,2,(((-((x+y)&1))&(0x010100-0x000101)) + 0x000101)*128);

                    //draw chess piece
                    if (board[y][x])
                    {
                        point3d pp, rr, dd, ff;
                        i = labs(board[y][x])-1;
                        pp.x = fx; pp.y = fy; pp.z = heioff[i];
                        rr.x = sizoff[i]; rr.y = 0.0f; rr.z = 0.0f;
                        dd.x = 0.0f; dd.y = sizoff[i]; dd.z = 0.0f; if (board[y][x] == -2) { rr.x *= -1.f; dd.y *= -1.f; }
                        ff.x = 0.0f; ff.y = 0.0f; ff.z = sizoff[i];
                        if ((tox >= 0) && (x == fromx) && (y == fromy))
                        {
                            f = (tim-movetim)/gchesstime;
                            //pp.z -= (cos(f*PI*2.f)*-.5f + .5f)*.1f;
                            //rotate_vex((cos(f*PI*2.f)*-.5f + .5f)*.25f,&dd,&ff);
                            //rotate_vex(f*PI*2.0,&dd,&ff);
                            f = cos(f*PI)*-.5f + .5f;
                            f *= .25f;
                            pp.x += (tox-fromx)*f;
                            pp.y -= (toy-fromy)*f;
                        }
                        if (board[y][x] > 0) j = gchesscol[0];
                        if (board[y][x] < 0) j = gchesscol[1];
                        voxie_drawspr(&vf,chessnam[i],&pp,&rr,&dd,&ff,j);
                        //voxie_drawbox(&vf,pp.x-.04f,pp.y-.04f,.21f,pp.x+.04f,pp.y+.04f,.21f,1,j);
                        //voxie_drawbox(&vf,pp.x-.06f,pp.y-.06f,.21f,pp.x+.06f,pp.y+.06f,.21f,1,j);
                        //voxie_drawbox(&vf,pp.x-.08f,pp.y-.08f,.21f,pp.x+.08f,pp.y+.08f,.21f,1,j);
                    }
                }

            //finish move
            if ((tox >= 0) && (tim >= movetim+gchesstime))
            {
                domove(board,&caststat,&prevmove,fromx,fromy,tox,toy,1);
                turn = 1-turn; fromx = -1; tox = -1;
            }

            if (win < 0)
            {
                if ((!gchessailev[turn]) && (tim >= movetim+gchesstime))
                {
                    if (!gchess_automove)
                    {
                        fx = ((float)cursx-3.5f)*.25f; fy = ((float)cursy-3.5f)*-.25f;
                        drawcube_thickwire(&vf,fx-.125f,fy-.125f,+.17f,fx+.125f,fy+.125f,+.20f,.01f,0xffffff);
                    }
                    if ((fromx >= 0) && (tox < 0))
                    {
                        x0 = ((float)fromx-3.5f)*.25f; y0 = ((float)fromy-3.5f)*-.25f;
                        if ((fromx == cursx) && (fromy == cursy))
                        {
                            voxie_drawsph(&vf,x0,y0,.17f,.03f,1,0xffffff);
                        }
                        else
                        {
                            x1 = ((float)cursx-3.5f)*.25f; y1 = ((float)cursy-3.5f)*-.25f;
                            f = 0.15/sqrt((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));
                            voxie_drawcone(&vf,x0,y0,.17f,.03f,(x0-x1)*f+x1,(y0-y1)*f+y1,.17f,.03f,1,0xffffff);
                            voxie_drawcone(&vf,(x0-x1)*f+x1,(y0-y1)*f+y1,.17f,.05f,x1,y1,.17f,.001f,1,0xffffff);
                        }
                    }
                }
            }
            else
            {
                char buf[256];
                point3d pp, rr, dd;
                pp.x =  0.00f; rr.x = 0.16; dd.x = 0.00;
                pp.y = -0.12f; rr.y = 0.00; dd.y = 0.24;
                f = 1.f/min(min(vw.aspx,vw.aspy),vw.aspz/.2f);
                pp.z = (-vw.aspz*f-.2f)*.5f+.01f; rr.z = 0.00; dd.z = 0.00;
                if (win < 2) strcpy(buf,"Checkmate!");
                else strcpy(buf,"Stalemate");
                f = (float)strlen(buf)*.5; pp.x -= rr.x*f; pp.y -= rr.y*f; //pp.z -= rr.z*f;
                voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%s",buf);
            }

        }
            break;


            //--------------------------------------------------------------------------------------------------

        case RENDMODE_MODELANIM: //KV6/STL/animation display
        {
            static int colval = 0x404040;
            static int inited = 0;
            int xsiz, ysiz;

            if (labs(vx[0].tx0) < 4096) vx[0].tx0 = 0;
            if (labs(vx[0].ty0) < 4096) vx[0].ty0 = 0;
            if (labs(vx[0].tx1) < 4096) vx[0].tx1 = 0;
            if (labs(vx[0].ty1) < 4096) vx[0].ty1 = 0;

            voxie_setview(&vf,0.0,0.0,-256,gxres,gyres,+256); //old coords

            if (!gmodelanim_pauseit)
            {
                i = ganimi;
                //Change animation
                if ((voxie_keystat(0xc9) == 1) || (ghitkey == 0xc9) || (voxie_keystat(0x27) == 1) || ((vx[0].but&~ovxbut[0])&((1<<14)|(1<<8)))) { ghitkey = 0; ganimi--; if (ganimi <       0) ganimi = max(ganimn-1,0); voxie_playsound_update(-1,-1,0,0,1.f);/*kill all sound*/ cycletim = tim+5.0; } //PGUP
                if ((voxie_keystat(0xd1) == 1) || (ghitkey == 0xd1) || (voxie_keystat(0x28) == 1) || ((vx[0].but&~ovxbut[0])&((1<<13)|(1<<9)))) { ghitkey = 0; ganimi++; if (ganimi >= ganimn) ganimi =               0; voxie_playsound_update(-1,-1,0,0,1.f);/*kill all sound*/ cycletim = tim+5.0; } //PGDN
                if ((gautocycle) && (tim > cycletim))
                {
                    ganimi++; if (ganimi >= ganimn) ganimi = 0; voxie_playsound_update(-1,-1,0,0,1.f);/*kill all sound*/
                    cycletim = tim+5.0;
                }

                if ((i != ganimi) && (ganim[ganimi].snd[0]))
                {
                    ganim[ganimi].cnt = 0;
                    voxie_playsound(ganim[ganimi].snd,-1,100,100,1.f);
                }
            }

            if (voxie_keystat(0x34) == 1) { colval = min((colval&255)<<1,255)*0x10101; }
            if (voxie_keystat(0x33) == 1) { colval = max((colval&255)>>1,  1)*0x10101; }
            if (voxie_keystat(0x32) == 1) //M:toggle slicemode
            {
                gslicemode = !gslicemode;
                gslicep.x = 0.f; gslicer.x = .05f; gsliced.x = 0.f; gslicef.x = 0.f;
                gslicep.y = 0.f; gslicer.y = 0.f; gsliced.y = .05f; gslicef.y = 0.f;
                gslicep.z = 0.f; gslicer.z = 0.f; gsliced.z = 0.f; gslicef.z = .05f;
            }

            if (((voxie_keystat(0x1f) == 1) && (voxie_keystat(0x1d) || voxie_keystat(0x9d))) || (ghitkey == 0x1f)) //Ctrl+S
            {
                FILE *fil;
                ghitkey = 0;
                fil = fopen("anim.ini","w");
                if (fil)
                {
                    fprintf(fil,"%d\n",ganimi);
                    for(i=0;i<ganimn;i++)
                        fprintf(fil,"%s,%f,%f,%f,%f,%f,%f,%f\n",ganim[i].file,ganim[i].p.x,ganim[i].p.y,ganim[i].p.z,ganim[i].ang[0],ganim[i].ang[1],ganim[i].ang[2],ganim[i].sc);
                    fclose(fil);
                }
                MessageBeep(48);
            }

            if ((((voxie_keystat(0x26) == 1) && (voxie_keystat(0x1d) || voxie_keystat(0x9d))) || (ghitkey == 0x26)) || (!inited)) //Ctrl+L
            {
                char tbuf[MAX_PATH];
                FILE *fil;
                inited = 1; ghitkey = 0;
                fil = fopen("anim.ini","r");
                if (fil)
                {
                    fscanf(fil,"%d\n",&ganimi); ganimi = max(min(ganimi,ganimn-1),0);
                    while (!feof(fil))
                    {
                        i = 0;
                        do { tbuf[i] = fgetc(fil); if (tbuf[i] == ',') break; i++; } while ((!feof(fil)) && (i < MAX_PATH-1) && (tbuf[i] != 13));
                        tbuf[i] = 0;

                        for(i=ganimn-1;i>=0;i--)
                        {
                            if (stricmp(ganim[i].file,tbuf)) continue;
                            fscanf(fil,"%f,%f,%f,%f,%f,%f,%f",&ganim[i].p.x,&ganim[i].p.y,&ganim[i].p.z,&ganim[i].ang[0],&ganim[i].ang[1],&ganim[i].ang[2],&ganim[i].sc);
                            ganim[i].defscale = ganim[i].sc;
                            ganim[i].p_init.x = ganim[i].p.x;
                            ganim[i].p_init.y = ganim[i].p.y;
                            ganim[i].p_init.z = ganim[i].p.z;
                            ganim[i].ang_init[0] = ganim[i].ang[0];
                            ganim[i].ang_init[1] = ganim[i].ang[1];
                            ganim[i].ang_init[2] = ganim[i].ang[2];
                            break;
                        }
                        do { i = fgetc(fil); } while ((!feof(fil)) && (i != 10));
                    }
                    fclose(fil);
                }
            }

            if ((ganim[ganimi].n <= 0) && (ganim[ganimi].loaddone))
            {
                point3d pp, rr, dd;
                pp.x = 0.0; rr.x = 20.0; dd.x =  0.0;
                pp.y = 0.0; rr.y =  0.0; dd.y = 60.0;
                pp.z = 0.0; rr.z =  0.0; dd.z =  0.0;
                voxie_printalph(&vf,&pp,&rr,&dd,0xffffff,"File not found");
                break;
            }
            if (!ganim[ganimi].loaddone)
            {
                char tbuf[MAX_PATH];

                sprintf(tbuf,ganim[ganimi].file,ganim[ganimi].n+1);
                if ((!ganim[ganimi].n) || (strchr(ganim[ganimi].file,'%')))
                {
                    if (ganim[ganimi].n >= ganim[ganimi].mal)
                    {
                        ganim[ganimi].mal = max(ganim[ganimi].mal<<1,256);
                        ganim[ganimi].tt = (tiletype *)realloc(ganim[ganimi].tt,ganim[ganimi].mal*sizeof(tiletype));
                    }

                    if ((ganim[ganimi].snd[0]) && (!ganim[ganimi].n)) voxie_playsound(ganim[ganimi].snd,-1,100,100,1.f);

                    i = strlen(tbuf);
                    if ((i >= 4) && (tbuf[i-4] == '.') && ((tbuf[i-3] == 'P') || (tbuf[i-3] == 'p')) &&
                            ((tbuf[i-2] == 'N') || (tbuf[i-2] == 'n')) &&
                            ((tbuf[i-1] == 'G') || (tbuf[i-1] == 'g'))) ganim[ganimi].filetyp = 0; else ganim[ganimi].filetyp = 1;
                    if (ganim[ganimi].filetyp == 0)
                    {
                        kpzload(tbuf,      &ganim[ganimi].tt[ganim[ganimi].n].f,(int *)&ganim[ganimi].tt[ganim[ganimi].n].p,
                                (int *)&ganim[ganimi].tt[ganim[ganimi].n].x,(int *)&ganim[ganimi].tt[ganim[ganimi].n].y);
                        ganim[ganimi].tt[ganim[ganimi].n].p &= 0xffffffffI64;
                        ganim[ganimi].tt[ganim[ganimi].n].x &= 0xffffffffI64;
                        ganim[ganimi].tt[ganim[ganimi].n].y &= 0xffffffffI64;
                    }
                    else
                    {
                        ganim[ganimi].tt[ganim[ganimi].n].f = (INT_PTR)malloc(strlen(tbuf)+1);
                        if (ganim[ganimi].tt[ganim[ganimi].n].f) strcpy((char *)ganim[ganimi].tt[ganim[ganimi].n].f,tbuf);

                        if (kzopen((const char *)ganim[ganimi].tt[ganim[ganimi].n].f)) kzclose();
                        else ganim[ganimi].tt[ganim[ganimi].n].f = 0;
                    }
                }
                else
                {
                    ganim[ganimi].tt[ganim[ganimi].n].f = 0;
                }

                if (ganim[ganimi].tt[ganim[ganimi].n].f) { ganim[ganimi].n++; }
                else { ganim[ganimi].loaddone = 1; if (!ganim[ganimi].n) break; }
            }

            if (voxie_keystat(0x19) == 1) { gmodelanim_pauseit ^= 1; } //P
            if (((gmodelanim_pauseit) && ((voxie_keystat(0xc9) == 1) || (voxie_keystat(0x27) == 1) || ((vx[0].but&~ovxbut[0])&((1<<14)|(1<<8))))) || (ghitkey == 0xc8)) { ghitkey = 0; ganim[ganimi].cnt--; if (ganim[ganimi].cnt < 0) ganim[ganimi].cnt = ganim[ganimi].n-1; cycletim = tim+5.0; } //PGUP
            if (((gmodelanim_pauseit) && ((voxie_keystat(0xd1) == 1) || (voxie_keystat(0x28) == 1) || ((vx[0].but&~ovxbut[0])&((1<<13)|(1<<9))))) || (ghitkey == 0xd0)) { ghitkey = 0; ganim[ganimi].cnt++; if (ganim[ganimi].cnt >= ganim[ganimi].n) ganim[ganimi].cnt = 0;  cycletim = tim+5.0; } //PGDN

            if ((!gmodelanim_pauseit) && ((int)(tim*ganim[ganimi].fps) != (int)(otim*ganim[ganimi].fps)))
            {
                if (ganim[ganimi].mode == 0) //forward animation
                {
                    ganim[ganimi].cnt++;
                    if (ganim[ganimi].cnt >= ganim[ganimi].n)
                    {
                        ganim[ganimi].cnt = 0;
                        if ((ganim[ganimi].snd[0]) && (strchr(ganim[ganimi].file,'%'))) voxie_playsound(ganim[ganimi].snd,0,100,100,1.f);
                    }
                }
                else //pingpong animation
                {
                    if (ganim[ganimi].mode > 0) { ganim[ganimi].cnt++; if (ganim[ganimi].cnt >= ganim[ganimi].n) { ganim[ganimi].cnt--; ganim[ganimi].mode *= -1; } }
                    else { ganim[ganimi].cnt--; if (ganim[ganimi].cnt <                0) { ganim[ganimi].cnt++; ganim[ganimi].mode *= -1; } }
                }
            }

            if (ganim[ganimi].filetyp != 0)
            {
                point3d fp, fr, fd, ff;
                char tbuf[MAX_PATH];


                /************************************************/
                /* adding Leap motion device, by Yi-Ting, Hsieh */
#if (USELEAP)

                // calling Leap_Voxon for setting up leap motion service
                leap_voxon.set_swipemode(true);
                leap_voxon.getFrame();
                leap_voxon.track_event();

#endif
                /* end here */
                /************************************************/

                if (!gslicemode)
                {
                    // if (voxie_keystat(keyremap[1][4]) || voxie_keystat(keyremap[1][5]) || (vx[0].lt|vx[0].rt) || (in.dmousz != 0.0) || (nav[0].but) || ((bstatus&3) == 3) || (HandPinched)) //ButA, ButB, XBox, dmousz, nav
                    if ((voxie_keystat(keyremap[1][4]) || voxie_keystat(keyremap[1][5]) || ((bstatus&3) == 3) || HandPinched)) //ButA, ButB, XBox, dmousz, nav
                    {
                        f = 1.f;
                        if (vx[0].lt) f *= pow(1.0+vx[0].lt/256.0,(double)dtim);
                        if (vx[0].rt) f /= pow(1.0+vx[0].rt/256.0,(double)dtim);
                        if (in.dmousz != 0.0) f *= pow(0.5,(double)in.dmousz*.0005);
                        if ((bstatus&3) == 3) f *= pow(0.5,(double)in.dmousy*-.01);
                        if (voxie_keystat(keyremap[1][5])|(nav[0].but&1)) f *= pow(2.0,(double)dtim); //ButA
                        if (voxie_keystat(keyremap[1][4])|(nav[0].but&2)) f *= pow(0.5,(double)dtim); //ButB

                        // TO DO...
                        // Zoom In/Out features
                        /************************************************/
                        /* adding Leap motion device, by Yi-Ting, Hsieh */
#if (USELEAP)
                        // Check Event
                        // Zoom in: Right hand Pinch
                        // Zoom out: Left hand Pinch
                        if (leap_voxon.getEventType() != nullptr)
                        {
                            // if (strcmp(leap_voxon.getEventType(),"Pinch")==0)
                            // {
                            // 	if(leap_voxon.getHand()->type==eLeapHandType_Right){ f *= pow(2.0,(double)dtim); }
                            //   else if(leap_voxon.getHand()->type==eLeapHandType_Left){ f*= pow(0.5,(double)dtim); }
                            // }

                            /* 08/18/2018 : new Update:
                                        Tracking the palm position to zoom in/out*/
                            if (strcmp(leap_voxon.getEventType(),"Grab")==0)
                            {
                                if ((float)((&(&leap_voxon.getHand()->palm)->position)->y) < 150) {  f*= pow(2.0,(double)dtim); }
                                else if ((float)((&(&leap_voxon.getHand()->palm)->position)->y) > 250) { f *= pow(0.5,(double)dtim); }
                            }
                        }
#endif
                        /* end here */
                        /************************************************/


                        f *= ganim[ganimi].sc;

                        fr.x = 1.f; fr.y = 0.f; fr.z = 0.f;
                        fd.x = 0.f; fd.y = 1.f; fd.z = 0.f;
                        ff.x = 0.f; ff.y = 0.f; ff.z = 1.f;
                        rotate_vex(ganim[ganimi].ang[0],&fr,&fd);
                        rotate_vex(ganim[ganimi].ang[1],&fd,&ff);
                        rotate_vex(ganim[ganimi].ang[2],&ff,&fr);

                        g = f/ganim[ganimi].sc; ganim[ganimi].p.x *= g; ganim[ganimi].p.y *= g; ganim[ganimi].p.z *= g;

                        ganim[ganimi].sc = f;
                    }

                    f = dtim*.5f;
                    ganim[ganimi].p.x += vx[0].tx0*dtim*+.00002f + nav[0].dx*dtim*.005f;
                    ganim[ganimi].p.y += vx[0].ty0*dtim*-.00002f + nav[0].dy*dtim*.005f;
                    ganim[ganimi].p.z += (((vx[0].but>>0)&1)-((vx[0].but>>1)&1))*dtim*-.5f + nav[0].dz*dtim*.0025f;
                    //if (voxie_keystat(0x2a)) { f *= 1.f/4.f; }
                    //if (voxie_keystat(0x36)) { f *= 4.f/1.f; }
                    if (voxie_keystat(keyremap[0][0])) { ganim[ganimi].p.x -= f; } //Left
                    if (voxie_keystat(keyremap[0][1])) { ganim[ganimi].p.x += f; } //Right
                    if (voxie_keystat(keyremap[0][2])) { ganim[ganimi].p.y -= f; } //Up
                    if (voxie_keystat(keyremap[0][3])) { ganim[ganimi].p.y += f; } //Down
                    if (voxie_keystat(keyremap[0][4])) { ganim[ganimi].p.z += f; } //ButA
                    if (voxie_keystat(keyremap[0][5])) { ganim[ganimi].p.z -= f; } //ButB

                    /************************************************/
                    /* adding Leap motion device, by Yi-Ting, Hsieh */
#if (USELEAP)


                    // Draw hands if there is any hand on Canvas
                    if(leap_voxon.getNumHands()==1)
                    {
                        getIndexPos(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode(),.2);
                        checkExitGesture(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                    }
                    else if(leap_voxon.getNumHands()==2)
                    {
                        //drawHands(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                        //drawHands(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                        getIndexPos(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode(),.2);
                        // drawExitBox(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),.3);
                        checkExitGesture(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());

                    }

                    f = 0.10f;
                    // Grab + Swipe as pressing arrow buttons moving objects around
                    if (leap_voxon.getEventType() != nullptr)
                    {
                        // if (strcmp(leap_voxon.getEventType(),"Swipe+Grab")==0 && leap_voxon.getSwipeType() == 0)
                        // {
                        //   ganim[ganimi].p.z -= f;
                        // }
                        // else if (strcmp(leap_voxon.getEventType(),"Swipe+Grab")==0 && leap_voxon.getSwipeType() == 1)
                        // {
                        //   ganim[ganimi].p.z += f;
                        // }
                        // else if (strcmp(leap_voxon.getEventType(),"Swipe+Grab")==0 && leap_voxon.getSwipeType() == 2)
                        // {
                        //   ganim[ganimi].p.x -= f;
                        // }
                        // else if (strcmp(leap_voxon.getEventType(),"Swipe+Grab")==0 && leap_voxon.getSwipeType() == 3)
                        // {
                        //   ganim[ganimi].p.x += f;
                        // }
                        // else if (strcmp(leap_voxon.getEventType(),"Swipe+Grab")==0 && leap_voxon.getSwipeType() == 4)
                        // {
                        //   ganim[ganimi].p.y -= f;
                        // }
                        // else if (strcmp(leap_voxon.getEventType(),"Swipe+Grab")==0 && leap_voxon.getSwipeType() == 5)
                        // {
                        //   ganim[ganimi].p.y += f;
                        // }

                        // if (strcmp(leap_voxon.getEventType(),"Grab")==0 )
                        // {
                        // 	ganim[ganimi].p.x = indexPos.x;
                        // 	ganim[ganimi].p.y = indexPos.y;
                        // 	ganim[ganimi].p.z = indexPos.z;
                        // }

                        // rotate objects by swiping
                        if (leap_voxon.getNumHands() == 1)
                        {

                            if (strcmp(leap_voxon.getEventType(),"Pinch")==0)
                            {
                                ganim[ganimi].p.x = indexPos.x;
                                ganim[ganimi].p.y = indexPos.y;
                                ganim[ganimi].p.z = indexPos.z;
                            }


                            if (strcmp(leap_voxon.getEventType(),"Grab")==0 && leap_voxon.getSwipeType()==-1)
                            {
                                // if (countGrab < 200) { countGrab++; }
                                // else
                                // {
                                //ganim[ganimi].ang[0] += indexPos.x*+.05;
                                int tempX = (int) ((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->x);

                                if (tempX >= 80 || tempX <= -80) { ganim[ganimi].ang[0] += ((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->x/30)*(+.01); }
                                // ganim[ganimi].ang[0] += ((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->x/20)*(+.01);
                                // if (tempY >= 20 || tempY <= -20) { ganim[ganimi].ang[2] += ((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->y/20)*(+.01); }
                                // if (tempZ >= 80 || tempZ <= -80) { ganim[ganimi].ang[1] += ((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->z/30)*(+.01); }
                                if (((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->z) > 80 || ((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->z) < -80)
                                {
                                    ganim[ganimi].ang[1] += ((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->z/30)*(+.01);
                                }
                                // if (fabs((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->y-120)>=0){
                                // 	ganim[ganimi].ang[2] += (((&(&(&leap_voxon.getHand()->digits[1])->bones[3])->next_joint)->y-120)/30)*(+.01);
                                // }
                                // }

                                if (!HandPinched) HandPinched = true;


                            } else {
                                if (countGrab != 0) countGrab = 0;
                                if(HandPinched) HandPinched = false;
                            }

                            // Swipe: Left: go to prev image;   Right: go to next image
                            if (strcmp(leap_voxon.getEventType(),"Swipe")==0 && leap_voxon.getSwipeType() == 2 && leap_voxon.getHand()->type == eLeapHandType_Right)
                            {
                                if (leap_voxon.ifEventFinished())
                                {
                                    gmodelanim_pauseit ^= 1;
                                    ghitkey = 0; ganimi--; if (ganimi < 0) ganimi = max(ganimn-1,0); voxie_playsound_update(-1,-1,0,0,1.f);/*kill all sound*/ cycletim = tim+5.0;
                                    break;
                                }
                            }
                            else if (strcmp(leap_voxon.getEventType(),"Swipe")==0 && leap_voxon.getSwipeType() == 3 && 2 && leap_voxon.getHand()->type == eLeapHandType_Left)
                            {
                                if (leap_voxon.ifEventFinished())
                                {
                                    gmodelanim_pauseit ^= 1;
                                    ghitkey = 0; ganimi++; if (ganimi >= ganimn) ganimi = 0; voxie_playsound_update(-1,-1,0,0,1.f);/*kill all sound*/ cycletim = tim+5.0;
                                    break;
                                }
                            }

                            // Zoom out: Left hand Pinch

                            // if (strcmp(leap_voxon.getEventType(),"Pinch")==0)
                            // {
                            // 	if (!HandPinched) HandPinched = true;
                            // }
                            // else { if(HandPinched) HandPinched = false; }

                        }
                    }

                    if (checkExitBox == true || checkExitStatus == FIRE)
                    {
                        grendmode = RENDMODE_TITLE; checkExitBox = false;
                        checkExitStatus = NOP;
                        leap_voxon.set_swipemode(false);

                        menubox.timer = 0;
                        menubox.status = 1;
                        menubox.delayTimer = DELAYTIME;
                        menubox.fromTop = false;
                    }

#endif
                    /* end here */
                    /************************************************/

                    if (!(bstatus&3))
                    {
                        ganim[ganimi].ang[0] += in.dmousx*+.01;
                        ganim[ganimi].ang[1] += in.dmousy*+.01;
                        //if (gautorotateax < 0) ganim[ganimi].ang[1] = min(max(ganim[ganimi].ang[1],PI*-.5),PI*.5);
                        if (!(gautorotatespd[0]|gautorotatespd[1]|gautorotatespd[2])) ganim[ganimi].ang[1] = min(max(ganim[ganimi].ang[1],PI*-.5),PI*.5);
                    }
                    ganim[ganimi].ang[0] += vx[0].tx1*dtim*.0001f;
                    ganim[ganimi].ang[1] += vx[0].ty1*dtim*.0001f;
                    fr.x = 1.f; fr.y = 0.f; fr.z = 0.f;
                    fd.x = 0.f; fd.y = 1.f; fd.z = 0.f;
                    ff.x = 0.f; ff.y = 0.f; ff.z = 1.f;
                    rotate_vex(ganim[ganimi].ang[0],&fr,&fd);
                    rotate_vex(ganim[ganimi].ang[1],&fd,&ff);
                    rotate_vex(ganim[ganimi].ang[2],&ff,&fr);

                    //Hacks for nice matrix movement with Space Navigator
                    f = fr.y; fr.y = fd.x; fd.x = f;
                    f = fr.z; fr.z = ff.x; ff.x = f;
                    f = fd.z; fd.z = ff.y; ff.y = f;

                    //fr.x*fx + fd.x*fy + ff.x*fz = -ganim[ganimi].p.x
                    //fr.y*fx + fd.y*fy + ff.y*fz = -ganim[ganimi].p.y
                    //fr.z*fx + fd.z*fy + ff.z*fz = -ganim[ganimi].p.z
                    fx = (ganim[ganimi].p.x*fr.x + ganim[ganimi].p.y*fd.x + ganim[ganimi].p.z*ff.x); //rotate around center of volume (backup)
                    fy = (ganim[ganimi].p.x*fr.y + ganim[ganimi].p.y*fd.y + ganim[ganimi].p.z*ff.y);
                    fz = (ganim[ganimi].p.x*fr.z + ganim[ganimi].p.y*fd.z + ganim[ganimi].p.z*ff.z);
                    //rotate_vex(nav[0].az*dtim*-.005f + in.dmousx*+.01*(float)((bstatus&3)>=2) + (gautorotateax==2)*(gautorotatespd!=0)*pow(2.0,gautorotatespd-7.0)*dtim,&fr,&fd);
                    //rotate_vex(nav[0].ay*dtim*-.005f - in.dmousy*+.01*(float)((bstatus&3)==1) + (gautorotateax==0)*(gautorotatespd!=0)*pow(2.0,gautorotatespd-7.0)*dtim,&fd,&ff);
                    //rotate_vex(nav[0].ax*dtim*+.005f + in.dmousx*+.01*(float)((bstatus&3)==1) + (gautorotateax==1)*(gautorotatespd!=0)*pow(2.0,gautorotatespd-7.0)*dtim,&ff,&fr);
                    rotate_vex(nav[0].az*dtim*-.005f + in.dmousx*+.01*(float)((bstatus&3)>=2) + (gautorotatespd[2]!=0)*pow(2.0,gautorotatespd[2]-7.0)*dtim,&fr,&fd);
                    rotate_vex(nav[0].ay*dtim*-.005f - in.dmousy*+.01*(float)((bstatus&3)==1) + (gautorotatespd[0]!=0)*pow(2.0,gautorotatespd[0]-7.0)*dtim,&fd,&ff);
                    rotate_vex(nav[0].ax*dtim*+.005f + in.dmousx*+.01*(float)((bstatus&3)==1) + (gautorotatespd[1]!=0)*pow(2.0,gautorotatespd[1]-7.0)*dtim,&ff,&fr);
                    ganim[ganimi].p.x = (fr.x*fx + fr.y*fy + fr.z*fz); //rotate around center of volume instead of center of object
                    ganim[ganimi].p.y = (fd.x*fx + fd.y*fy + fd.z*fz);
                    ganim[ganimi].p.z = (ff.x*fx + ff.y*fy + ff.z*fz);

                    f = fr.y; fr.y = fd.x; fd.x = f;
                    f = fr.z; fr.z = ff.x; ff.x = f;
                    f = fd.z; fd.z = ff.y; ff.y = f;
                    //See MAT2ANG.KC for derivation
                    ganim[ganimi].ang[0] = atan2(-fd.x,fd.y);
                    ganim[ganimi].ang[1] = atan2(fd.z,sqrt(fd.x*fd.x + fd.y*fd.y));
                    ganim[ganimi].ang[2] = atan2(-fr.z,ff.z);

                    fr.x *= ganim[ganimi].sc; fr.y *= ganim[ganimi].sc; fr.z *= ganim[ganimi].sc;
                    fd.x *= ganim[ganimi].sc; fd.y *= ganim[ganimi].sc; fd.z *= ganim[ganimi].sc;
                    ff.x *= ganim[ganimi].sc; ff.y *= ganim[ganimi].sc; ff.z *= ganim[ganimi].sc;

#if (USELEAP)
                    // frame = leap_getframe();
                    // for(i=min(frame->nHands,1)-1;i>=0;i--)
                    // {
                    // 	hand = &frame->pHands[i];
                    // 	//if (hand->type == eLeapHandType_Left) ..
                    //
                    // 	palm = &hand->palm;
                    // 	memcpy(&palmp,&palm->position ,sizeof(point3d));
                    // 	memcpy(&palmd,&palm->normal   ,sizeof(point3d));
                    // 	memcpy(&palmf,&palm->direction,sizeof(point3d));
                    // 	palmr.x = palmd.y*palmf.z - palmd.z*palmf.y;
                    // 	palmr.y = palmd.z*palmf.x - palmd.x*palmf.z;
                    // 	palmr.z = palmd.x*palmf.y - palmd.y*palmf.x;
                    // 	//use palmp/r/d/f
                    //
                    // 	ganim[ganimi].p.x = palmp.x*+.01f;
                    // 	ganim[ganimi].p.y = palmp.z*+.01f;
                    // 	ganim[ganimi].sc = exp(palmp.y*.01f)*.125f;
                    //
                    // 	fr.x =-palmr.x*ganim[ganimi].sc; fr.y =-palmr.z*ganim[ganimi].sc; fr.z =+palmr.y*ganim[ganimi].sc;
                    // 	fd.x =-palmd.x*ganim[ganimi].sc; fd.y =-palmd.z*ganim[ganimi].sc; fd.z =+palmd.y*ganim[ganimi].sc;
                    // 	ff.x =-palmf.x*ganim[ganimi].sc; ff.y =-palmf.z*ganim[ganimi].sc; ff.z =+palmf.y*ganim[ganimi].sc;
                    // 	rotate_vex(PI*.5f,&fd,&ff);
                    // }
#endif



                    if ((voxie_keystat(0x35)) || (ghitkey == 0x0e)) // /
                    {
                        ghitkey = 0;
                        ganim[ganimi].ang[0] = ganim[ganimi].ang_init[0];
                        ganim[ganimi].ang[1] = ganim[ganimi].ang_init[1];
                        ganim[ganimi].ang[2] = ganim[ganimi].ang_init[2];
                        ganim[ganimi].sc = ganim[ganimi].defscale;
                        ganim[ganimi].p.x = ganim[ganimi].p_init.x;
                        ganim[ganimi].p.y = ganim[ganimi].p_init.y;
                        ganim[ganimi].p.z = ganim[ganimi].p_init.z;
                    }
                }
                else
                {
                    if (voxie_keystat(keyremap[1][4]) || voxie_keystat(keyremap[1][5]) || (vx[0].lt|vx[0].rt) || (in.dmousz != 0.0) || (nav[0].but) || ((bstatus&3) == 3)) //ButA, ButB, XBox, dmousz, nav
                    {
                        f = 1.f;
                        if (vx[0].lt) f *= pow(1.0+vx[0].lt/256.0,(double)dtim);
                        if (vx[0].rt) f /= pow(1.0+vx[0].rt/256.0,(double)dtim);
                        if (in.dmousz != 0.0) f *= pow(0.5,(double)in.dmousz*.0005);
                        if ((bstatus&3) == 3) f *= pow(0.5,(double)in.dmousy*-.01);
                        if (voxie_keystat(keyremap[1][5])|(nav[0].but&1)) f *= pow(2.0,(double)dtim); //ButA
                        if (voxie_keystat(keyremap[1][4])|(nav[0].but&2)) f *= pow(0.5,(double)dtim); //ButB

                        gslicer.x *= f; gslicer.y *= f; gslicer.z *= f;
                        gsliced.x *= f; gsliced.y *= f; gsliced.z *= f;
                        gslicef.x *= f; gslicef.y *= f; gslicef.z *= f;
                    }

                    f = dtim*.5f;
                    gslicep.x += vx[0].tx0*dtim*+.00002f + nav[0].dx*dtim*.005f;
                    gslicep.y += vx[0].ty0*dtim*-.00002f + nav[0].dy*dtim*.005f;
                    gslicep.z += (((vx[0].but>>0)&1)-((vx[0].but>>1)&1))*dtim*-.5f + nav[0].dz*dtim*.0025f;
                    //if (voxie_keystat(0x2a)) { f *= 1.f/4.f; }
                    //if (voxie_keystat(0x36)) { f *= 4.f/1.f; }
                    if (voxie_keystat(keyremap[0][0])) { gslicep.x -= f; } //Left
                    if (voxie_keystat(keyremap[0][1])) { gslicep.x += f; } //Right
                    if (voxie_keystat(keyremap[0][2])) { gslicep.y -= f; } //Up
                    if (voxie_keystat(keyremap[0][3])) { gslicep.y += f; } //Down
                    if (voxie_keystat(keyremap[0][4])) { gslicep.z += f; } //ButA
                    if (voxie_keystat(keyremap[0][5])) { gslicep.z -= f; } //ButB

                    f = gslicer.y; gslicer.y = gsliced.x; gsliced.x = f;
                    f = gslicer.z; gslicer.z = gslicef.x; gslicef.x = f;
                    f = gsliced.z; gsliced.z = gslicef.y; gslicef.y = f;
                    rotate_vex(nav[0].az*dtim*-.005f + in.dmousx*+.01*(float)((bstatus&3)>=2),&gslicer,&gsliced);
                    rotate_vex(nav[0].ay*dtim*-.005f - in.dmousy*+.01*(float)((bstatus&3)==1),&gsliced,&gslicef);
                    rotate_vex(nav[0].ax*dtim*+.005f + in.dmousx*+.01*(float)((bstatus&3)==1),&gslicef,&gslicer);
                    f = gslicer.y; gslicer.y = gsliced.x; gsliced.x = f;
                    f = gslicer.z; gslicer.z = gslicef.x; gslicef.x = f;
                    f = gsliced.z; gsliced.z = gslicef.y; gslicef.y = f;

                    //Keep slice near origin by subtracting dot of unused axes - works great!
                    f = (gslicer.x*gslicep.x + gslicer.y*gslicep.y + gslicer.z*gslicep.z)
                            / (gslicer.x*gslicer.x + gslicer.y*gslicer.y + gslicer.z*gslicer.z); gslicep.x -= gslicer.x*f; gslicep.y -= gslicer.y*f; gslicep.z -= gslicer.z*f;
                    f = (gslicef.x*gslicep.x + gslicef.y*gslicep.y + gslicef.z*gslicep.z)
                            / (gslicef.x*gslicef.x + gslicef.y*gslicef.y + gslicef.z*gslicef.z); gslicep.x -= gslicef.x*f; gslicep.y -= gslicef.y*f; gslicep.z -= gslicef.z*f;

                    if ((voxie_keystat(0x35)) || (ghitkey == 0x0e)) // /
                    {
                        ghitkey = 0;
                        gslicep.x = 0.f; gslicer.x = .05f; gsliced.x = 0.f; gslicef.x = 0.f;
                        gslicep.y = 0.f; gslicer.y = 0.f; gsliced.y = .05f; gslicef.y = 0.f;
                        gslicep.z = 0.f; gslicer.z = 0.f; gsliced.z = 0.f; gslicef.z = .05f;
                    }
                }

                f = 1.f/min(min(vw.aspx,vw.aspy),vw.aspz/.2f);
                voxie_setview(&vf,-vw.aspx*f,-vw.aspy*f,-vw.aspz*f,+vw.aspx*f,+vw.aspy*f,+vw.aspz*f);

                if (gslicemode) { voxie_setmaskplane(&vf, gslicep.x,gslicep.y,gslicep.z,gsliced.x,gsliced.y,gsliced.z); }

                if (!voxie_drawspr(&vf,(const char *)ganim[ganimi].tt[ganim[ganimi].cnt].f,&ganim[ganimi].p,&fr,&fd,&ff,colval))
                {
                    point3d pp, rr, dd;
                    pp.x = 0.0; rr.x = 20.0; dd.x =   0.0;
                    pp.y = 0.0; rr.y =  0.0; dd.y =  60.0;
                    pp.z = 0.0; rr.z =  0.0; dd.z =   0.0;
                    voxie_setview(&vf,0.0,0.0,-256,gxres,gyres,+256); //old coords
                    voxie_printalph(&vf,&pp,&rr,&dd,0xffffff,"File not found");
                    voxie_setview(&vf,-vw.aspx*f,-vw.aspy*f,-vw.aspz*f,+vw.aspx*f,+vw.aspy*f,+vw.aspz*f);
                }

                if ((bstatus&4) || (gdrawstats))
                {
                    fp.x = ganim[ganimi].p.x - (fr.x + fd.x + ff.x)*.5f;
                    fp.y = ganim[ganimi].p.y - (fr.y + fd.y + ff.y)*.5f;
                    fp.z = ganim[ganimi].p.z - (fr.z + fd.z + ff.z)*.5f;
                    voxie_drawcube(&vf,&fp,&fr,&fd,&ff,1,0xffffff);
                }

                //Don't allow model to fall out of view too far (bad for Dejarik animation, which is not centered well)
                //g = (-vw.aspx*f) - (ganim[ganimi].p.x + (fabs(fr.x) + fabs(fd.x) + fabs(ff.x))*.5f); if (g > 0.f) ganim[ganimi].p.x += g; //(xmin view) - (xmax model)
                //g = (+vw.aspx*f) - (ganim[ganimi].p.x - (fabs(fr.x) + fabs(fd.x) + fabs(ff.x))*.5f); if (g < 0.f) ganim[ganimi].p.x += g; //(xmax view) - (xmin model)
                //g = (-vw.aspy*f) - (ganim[ganimi].p.y + (fabs(fr.y) + fabs(fd.y) + fabs(ff.y))*.5f); if (g > 0.f) ganim[ganimi].p.y += g; //(ymin view) - (ymax model)
                //g = (+vw.aspy*f) - (ganim[ganimi].p.y - (fabs(fr.y) + fabs(fd.y) + fabs(ff.y))*.5f); if (g < 0.f) ganim[ganimi].p.y += g; //(ymax view) - (ymin model)
                //g = (-vw.aspz*f) - (ganim[ganimi].p.z + (fabs(fr.z) + fabs(fd.z) + fabs(ff.z))*.5f); if (g > 0.f) ganim[ganimi].p.z += g; //(zmin view) - (zmax model)
                //g = (+vw.aspz*f) - (ganim[ganimi].p.z - (fabs(fr.z) + fabs(fd.z) + fabs(ff.z))*.5f); if (g < 0.f) ganim[ganimi].p.z += g; //(zmax view) - (zmin model)
            }
            //else
            //{
            //   INT_PTR qwptr, qrptr;
            //   if ((vf.x >  gxres) || (vf.y > gyres)) { xx =  vf.x; yy =  vf.y; qwptr = vf.f; }
            //                                     else { xx = gxres; yy = gyres; qwptr = vf.f; }
            //   qrptr = ganim[ganimi].tt[ganim[ganimi].cnt].f;
            //
            //   j = vw.framepervol;
            //   xsiz = ganim[ganimi].tt[ganim[ganimi].cnt].x/j;
            //   for(y=0;y<yy;y++,qwptr+=vf.p,qrptr+=ganim[ganimi].tt[ganim[ganimi].cnt].p) //z_arccos to z_arccos (direct copy)
            //      for(i=0;i<j;i++)
            //         memcpy((void *)(vf.fp*i + qwptr),(void *)(xsiz*i*4 + qrptr),xx<<2);
            //}

            if (((voxie_keystat(0xb8)) && (ganim[ganimi].n > 0)) || (gdrawstats)) //R.Alt
            {
                char tbuf[MAX_PATH];

                point3d pp, rr, dd;
                voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,+vw.aspx,+vw.aspy,+vw.aspz);

                rr.x = 0.125f; dd.x = 0.00f; pp.x = vw.aspx*.1f;
                rr.y = 0.00f;  dd.y = 0.25f; pp.y = vw.aspy*-.99f;
                rr.z = 0.00f;  dd.z = 0.00f; pp.z = -vw.aspz+0.01f;
                voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%3d/%3d",ganim[ganimi].cnt+1,ganim[ganimi].n);

                sprintf(tbuf,ganim[ganimi].file,ganim[ganimi].cnt);
                i = strlen(tbuf);
                rr.x = vw.aspx*2.f/((float)(max(i,16)+2)); rr.y = 0.0f; rr.z = 0.0f;
                dd.x = 0.0f; dd.y = rr.x*1.5f; dd.z = 0.0f;
                pp.x = -vw.aspx + rr.x*(float)(max(i,16)+2 - i)*.5f; pp.y = vw.aspy*+.99-dd.y; pp.z = vw.aspz*-.99f;
                voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%s",tbuf);
            }

        }
            break;


#if 1
        case RENDMODE_DOTMUNCH: //Dot Munch
        {
            static unsigned char *got = 0;
#define GHOSTFRAMES 2
#define MUNCHFRAMES 9
            static const char *munnam[] =
            {
                "ghost_0.kv6","ghost_1.kv6",
                "munman_0.kv6","munman_1.kv6","munman_2.kv6","munman_3.kv6","munman_4.kv6","munman_5.kv6","munman_6.kv6","munman_7.kv6","munman_8.kv6"
            };
            static point3d smun, mun, nmun, vel, nvel, viewcent;
#define MONMAX 16
            static point3d smpos[MONMAX], mpos[MONMAX], mgoal[MONMAX], mvel[MONMAX];
            static int nmon = 0;
            static int inited = -1, numdots = 0, mcol[MONMAX] = {0x200808,0x20181c,0x08201c,0x201808}; //mcol[4] = {0x802020,0x806070,0x208070,0x806020};
            static double timpower = -1e32, timdeath = 0.0, timnextlev = -1e32;
            static float fsc = .33f, fscgoal = .33f;
            float fx, fy, fz, fscale;
            int fakebut;
            char ch;

            if (labs(vx[0].tx0) + labs(vx[0].ty0) > 16384)
            {
                if (labs(vx[0].tx0) > labs(vx[0].ty0)) { if (vx[0].tx0 < 0) vx[0].but |= (1<<2); else vx[0].but |= (1<<3); }
                else { if (vx[0].ty0 > 0) vx[0].but |= (1<<0); else vx[0].but |= (1<<1); }
            }
            if (labs(vx[0].tx1) + labs(vx[0].ty1) > 16384)
            {
                if (labs(vx[0].tx1) > labs(vx[0].ty1)) { if (vx[0].tx1 < 0) vx[0].but |= (1<<2); else vx[0].but |= (1<<3); }
                else { if (vx[0].ty1 > 0) vx[0].but |= (1<<0); else vx[0].but |= (1<<1); }
            }

            fakebut = 0;
            if (cmun->zsiz == 1)
            {
                if (fabs(nav[0].ax) + fabs(nav[0].ay) > 100)
                {
                    if (fabs(nav[0].ax) > fabs(nav[0].ay)) { if (nav[0].ax < 0.f) fakebut |= (1<<2); else fakebut |= (1<<3); }
                    else { if (nav[0].ay < 0.f) fakebut |= (1<<0); else fakebut |= (1<<1); }
                }
            }
            else
            {
                if (fabs(nav[0].dx) > max(fabs(nav[0].dy),fabs(nav[0].dz)))
                { if (fabs(nav[0].dx) > 100) { if (nav[0].dx < 0.f) fakebut |= (1<<2); else fakebut |= (1<<3); } }
                else if (fabs(nav[0].dy) > fabs(nav[0].dz))
                { if (fabs(nav[0].dy) > 100) { if (nav[0].dy < 0.f) fakebut |= (1<<0); else fakebut |= (1<<1); } }
                else
                { if (fabs(nav[0].dz) > 100) { if (nav[0].dz < 0.f) fakebut |= (1<<4); else fakebut |= (1<<5); } }
            }

            if (voxie_keystat(keyremap[0][6]) == 1)                            { inited = 0; munlev = 0;                    cmun = &gmun[munlev]; break; } //ButM:reset&title
            if ((voxie_keystat(0x0e) == 1) || (ghitkey == 0x0e) || ((vx[0].but&~ovxbut[0])&(1<<4))) { ghitkey = 0; inited = 0; munlev = 0; cmun = &gmun[munlev];        } //Backspace:reset
            if ((otim < timnextlev) && (tim >= timnextlev))                    { inited = 0; munlev = (munlev+1)%munn;      cmun = &gmun[munlev];        }
            if ((voxie_keystat(0xc9) == 1) || (ghitkey == 0xc9))  { ghitkey = 0; inited = 0; munlev = (munlev+munn-1)%munn; cmun = &gmun[munlev];        } //PGUP
            if ((voxie_keystat(0xd1) == 1) || (ghitkey == 0xd1))  { ghitkey = 0; inited = 0; munlev = (munlev+1)%munn;      cmun = &gmun[munlev];        } //PGDN

            if (inited <= 0)
            {
                if (inited < 0) { voxie_mountzip("mun_data.zip"); }

                inited = 1; timdeath = dtim*.5f; fsc = .15f; fscgoal = .33f;

                i = min(max(cmun->xsiz*cmun->ysiz*cmun->zsiz,1),1048576);
                got = (unsigned char *)realloc(got,i);

                numdots = 0; nmon = 0;
                for(z=0;z<cmun->zsiz;z++)
                    for(y=0;y<cmun->ysiz;y++)
                        for(x=0;x<cmun->xsiz;x++)
                        {
                            i = (z*cmun->ysiz+y)*cmun->xsiz+x; ch = cmun->board[i];
                            if (ch == 'S') { smun.x = (float)x; smun.y = (float)y; smun.z = (float)z; }
                            if (ch == 'M') { if (nmon < MONMAX) { smpos[nmon].x = (float)x; smpos[nmon].y = (float)y; smpos[nmon].z = (float)z; nmon++; } }
                            if (ch != '.') { got[i] = 1; numdots++; }
                        }
            }

            if (timdeath > 0.0)
            {
                timdeath -= dtim;
                if (timdeath <= 0.0)
                {
                    fscgoal = .33f; mun = smun; nmun = mun;
                    vel.x = -1.f; vel.y = 0.f; vel.z = 0.f;
                    nvel.x = 0.f; nvel.y = 0.f; nvel.z = 0.f;
                    for(i=0;i<nmon;i++) { mpos[i] = smpos[i]; mgoal[i] = mpos[i]; mvel[i].x = 0.f; mvel[i].y = 0.f; mvel[i].z = 0.f; }
                }
            }

            if (fscgoal < fsc) fsc = max(fsc - dtim*.2f,fscgoal);
            if (fscgoal > fsc) fsc = min(fsc + dtim*.2f,fscgoal);

            if (voxie_keystat(keyremap[0][0]) || (voxie_keystat(keyremap[2][0]) == 1) || (vx[0].but&(1<<2)) || (fakebut&(1<<2))) { if (nmun.x > mun.x) { nmun.x -= 1.f; vel.x = -1.f; } nvel.x =-1.f; nvel.y = 0.f; nvel.z = 0.f; }
            else if (voxie_keystat(keyremap[0][1]) || (voxie_keystat(keyremap[2][1]) == 1) || (vx[0].but&(1<<3)) || (fakebut&(1<<3))) { if (nmun.x < mun.x) { nmun.x += 1.f; vel.x = +1.f; } nvel.x =+1.f; nvel.y = 0.f; nvel.z = 0.f; }
            else if (voxie_keystat(keyremap[0][2]) || (voxie_keystat(keyremap[2][2]) == 1) || (vx[0].but&(1<<0)) || (fakebut&(1<<0))) { if (nmun.y > mun.y) { nmun.y -= 1.f; vel.y = -1.f; } nvel.x = 0.f; nvel.y =-1.f; nvel.z = 0.f; }
            else if (voxie_keystat(keyremap[0][3]) || (voxie_keystat(keyremap[2][3]) == 1) || (vx[0].but&(1<<1)) || (fakebut&(1<<1))) { if (nmun.y < mun.y) { nmun.y += 1.f; vel.y = +1.f; } nvel.x = 0.f; nvel.y =+1.f; nvel.z = 0.f; }
            else if (voxie_keystat(keyremap[0][5]) || (voxie_keystat(keyremap[2][5]) == 1) ||                       (fakebut&(1<<4))) { if (nmun.z > mun.z) { nmun.z -= 1.f; vel.z = -1.f; } nvel.x = 0.f; nvel.y = 0.f; nvel.z =-1.f; }
            else if (voxie_keystat(keyremap[0][4]) || (voxie_keystat(keyremap[2][4]) == 1) ||                       (fakebut&(1<<5))) { if (nmun.z < mun.z) { nmun.z += 1.f; vel.z = +1.f; } nvel.x = 0.f; nvel.y = 0.f; nvel.z =+1.f; }
            if ((nmun.x == mun.x) && (nmun.y == mun.y) && (nmun.z == mun.z))
            {
                x = (int)mun.x; y = (int)mun.y; z = (int)mun.z;
                if ((nvel.x < 0.f) && (mungetboard(cmun,x-1,y,z,'.') != '.')) { nmun.x = mun.x-1.f; vel.x =-1.f; vel.y = 0.f; vel.z = 0.f; nvel.x = 0.f; }
                if ((nvel.x > 0.f) && (mungetboard(cmun,x+1,y,z,'.') != '.')) { nmun.x = mun.x+1.f; vel.x =+1.f; vel.y = 0.f; vel.z = 0.f; nvel.x = 0.f; }
                if ((nvel.y < 0.f) && (mungetboard(cmun,x,y-1,z,'.') != '.')) { nmun.y = mun.y-1.f; vel.x = 0.f; vel.y =-1.f; vel.z = 0.f; nvel.y = 0.f; }
                if ((nvel.y > 0.f) && (mungetboard(cmun,x,y+1,z,'.') != '.')) { nmun.y = mun.y+1.f; vel.x = 0.f; vel.y =+1.f; vel.z = 0.f; nvel.y = 0.f; }
                if ((nvel.z < 0.f) && (mungetboard(cmun,x,y,z-1,'.') != '.')) { nmun.z = mun.z-1.f; vel.x = 0.f; vel.y = 0.f; vel.z =-1.f; nvel.z = 0.f; }
                if ((nvel.z > 0.f) && (mungetboard(cmun,x,y,z+1,'.') != '.')) { nmun.z = mun.z+1.f; vel.x = 0.f; vel.y = 0.f; vel.z =+1.f; nvel.z = 0.f; }
            }
            if ((nmun.x == mun.x) && (nmun.y == mun.y) && (nmun.z == mun.z))
            {
                x = (int)mun.x; y = (int)mun.y; z = (int)mun.z;
                if (( vel.x < 0.f) && (mungetboard(cmun,x-1,y,z,'.') != '.')) { nmun.x = mun.x-1.f; vel.x =-1.f; vel.y = 0.f; vel.z = 0.f; }
                if (( vel.x > 0.f) && (mungetboard(cmun,x+1,y,z,'.') != '.')) { nmun.x = mun.x+1.f; vel.x =+1.f; vel.y = 0.f; vel.z = 0.f; }
                if (( vel.y < 0.f) && (mungetboard(cmun,x,y-1,z,'.') != '.')) { nmun.y = mun.y-1.f; vel.x = 0.f; vel.y =-1.f; vel.z = 0.f; }
                if (( vel.y > 0.f) && (mungetboard(cmun,x,y+1,z,'.') != '.')) { nmun.y = mun.y+1.f; vel.x = 0.f; vel.y =+1.f; vel.z = 0.f; }
                if (( vel.z < 0.f) && (mungetboard(cmun,x,y,z-1,'.') != '.')) { nmun.z = mun.z-1.f; vel.x = 0.f; vel.y = 0.f; vel.z =-1.f; }
                if (( vel.z > 0.f) && (mungetboard(cmun,x,y,z+1,'.') != '.')) { nmun.z = mun.z+1.f; vel.x = 0.f; vel.y = 0.f; vel.z =+1.f; }
            }
            if ((timdeath <= 0.0) && (tim >= timnextlev))
            {
                if (nmun.x < mun.x) { mun.x -= dtim*4.f; if (mun.x <= nmun.x) { mun.x = nmun.x; if ((cmun->xwrap) && (mun.x <        0.f)) { mun.x += cmun->xsiz; nmun = mun; } } }
                if (nmun.x > mun.x) { mun.x += dtim*4.f; if (mun.x >= nmun.x) { mun.x = nmun.x; if ((cmun->xwrap) && (mun.x >=cmun->xsiz)) { mun.x -= cmun->xsiz; nmun = mun; } } }
                if (nmun.y < mun.y) { mun.y -= dtim*4.f; if (mun.y <= nmun.y) { mun.y = nmun.y; if ((cmun->ywrap) && (mun.y <        0.f)) { mun.y += cmun->ysiz; nmun = mun; } } }
                if (nmun.y > mun.y) { mun.y += dtim*4.f; if (mun.y >= nmun.y) { mun.y = nmun.y; if ((cmun->ywrap) && (mun.y >=cmun->ysiz)) { mun.y -= cmun->ysiz; nmun = mun; } } }
                if (nmun.z < mun.z) { mun.z -= dtim*4.f; if (mun.z <= nmun.z) { mun.z = nmun.z; if ((cmun->zwrap) && (mun.z <        0.f)) { mun.z += cmun->zsiz; nmun = mun; } } }
                if (nmun.z > mun.z) { mun.z += dtim*4.f; if (mun.z >= nmun.z) { mun.z = nmun.z; if ((cmun->zwrap) && (mun.z >=cmun->zsiz)) { mun.z -= cmun->zsiz; nmun = mun; } } }
            }

            for(i=nmon-1;i>=0;i--)
            {
                if ((mgoal[i].x == mpos[i].x) && (mgoal[i].y == mpos[i].y) && (mgoal[i].z == mpos[i].z))
                {
                    x = (int)(mpos[i].x); y = (int)(mpos[i].y); z = (int)(mpos[i].z);
                    if ((mvel[i].x < 0.f) && (mungetboard(cmun,x-1,y,z,'.') != '.')) { mgoal[i].x = mpos[i].x-1.f; mvel[i].x =-1.f; mvel[i].y = 0.f; mvel[i].z = 0.f; }
                    if ((mvel[i].x > 0.f) && (mungetboard(cmun,x+1,y,z,'.') != '.')) { mgoal[i].x = mpos[i].x+1.f; mvel[i].x =+1.f; mvel[i].y = 0.f; mvel[i].z = 0.f; }
                    if ((mvel[i].y < 0.f) && (mungetboard(cmun,x,y-1,z,'.') != '.')) { mgoal[i].y = mpos[i].y-1.f; mvel[i].x = 0.f; mvel[i].y =-1.f; mvel[i].z = 0.f; }
                    if ((mvel[i].y > 0.f) && (mungetboard(cmun,x,y+1,z,'.') != '.')) { mgoal[i].y = mpos[i].y+1.f; mvel[i].x = 0.f; mvel[i].y =+1.f; mvel[i].z = 0.f; }
                    if ((mvel[i].z < 0.f) && (mungetboard(cmun,x,y,z-1,'.') != '.')) { mgoal[i].z = mpos[i].z-1.f; mvel[i].x = 0.f; mvel[i].y = 0.f; mvel[i].z =-1.f; }
                    if ((mvel[i].z > 0.f) && (mungetboard(cmun,x,y,z+1,'.') != '.')) { mgoal[i].z = mpos[i].z+1.f; mvel[i].x = 0.f; mvel[i].y = 0.f; mvel[i].z =+1.f; }
                }
                if ((mgoal[i].x == mpos[i].x) && (mgoal[i].y == mpos[i].y) && (mgoal[i].z == mpos[i].z))
                {
                    x = (int)(mpos[i].x); y = (int)(mpos[i].y); z = (int)(mpos[i].z);
                    j = (z*cmun->ysiz + y)*cmun->xsiz + x;
                    switch(rand()%6)
                    {
                    case 0: if ((mpos[i].x >            0) && (mungetboard(cmun,x-1,y,z,'.') != '.')) { mgoal[i].x = mpos[i].x-1.f; mvel[i].x =-1.f; mvel[i].y = 0.f; mvel[i].z = 0.f; } break;
                    case 1: if ((mpos[i].x < cmun->xsiz-1) && (mungetboard(cmun,x+1,y,z,'.') != '.')) { mgoal[i].x = mpos[i].x+1.f; mvel[i].x =+1.f; mvel[i].y = 0.f; mvel[i].z = 0.f; } break;
                    case 2: if ((mpos[i].y >            0) && (mungetboard(cmun,x,y-1,z,'.') != '.')) { mgoal[i].y = mpos[i].y-1.f; mvel[i].x = 0.f; mvel[i].y =-1.f; mvel[i].z = 0.f; } break;
                    case 3: if ((mpos[i].y < cmun->ysiz-1) && (mungetboard(cmun,x,y+1,z,'.') != '.')) { mgoal[i].y = mpos[i].y+1.f; mvel[i].x = 0.f; mvel[i].y =+1.f; mvel[i].z = 0.f; } break;
                    case 4: if ((mpos[i].z >            0) && (mungetboard(cmun,x,y,z-1,'.') != '.')) { mgoal[i].z = mpos[i].z-1.f; mvel[i].x = 0.f; mvel[i].y = 0.f; mvel[i].z =-1.f; } break;
                    case 5: if ((mpos[i].z < cmun->zsiz-1) && (mungetboard(cmun,x,y,z+1,'.') != '.')) { mgoal[i].z = mpos[i].z+1.f; mvel[i].x = 0.f; mvel[i].y = 0.f; mvel[i].z =+1.f; } break;
                    }
                }
                if ((timdeath <= 0.0) && (tim >= timnextlev))
                {
                    if (mgoal[i].x < mpos[i].x) { mpos[i].x -= (float)(munlev+3)*dtim; if (mpos[i].x <= mgoal[i].x) mpos[i].x = mgoal[i].x; }
                    if (mgoal[i].x > mpos[i].x) { mpos[i].x += (float)(munlev+3)*dtim; if (mpos[i].x >= mgoal[i].x) mpos[i].x = mgoal[i].x; }
                    if (mgoal[i].y < mpos[i].y) { mpos[i].y -= (float)(munlev+3)*dtim; if (mpos[i].y <= mgoal[i].y) mpos[i].y = mgoal[i].y; }
                    if (mgoal[i].y > mpos[i].y) { mpos[i].y += (float)(munlev+3)*dtim; if (mpos[i].y >= mgoal[i].y) mpos[i].y = mgoal[i].y; }
                    if (mgoal[i].z < mpos[i].z) { mpos[i].z -= (float)(munlev+3)*dtim; if (mpos[i].z <= mgoal[i].z) mpos[i].z = mgoal[i].z; }
                    if (mgoal[i].z > mpos[i].z) { mpos[i].z += (float)(munlev+3)*dtim; if (mpos[i].z >= mgoal[i].z) mpos[i].z = mgoal[i].z; }
                }
                if (fabs(mpos[i].x-mun.x) + fabs(mpos[i].y-mun.y) + fabs(mpos[i].z-mun.z) < .99f)
                {
                    if (timpower >= tim)
                    {
                        //player kills ghost
                        for(j=256;j>0;j--)
                        {
                            x = rand()%cmun->xsiz;
                            y = rand()%cmun->ysiz;
                            z = rand()%cmun->zsiz;
                            if ((mungetboard(cmun,x,y,z,'.') != '.') && (fabs((float)x-mun.x) + fabs((float)y-mun.y) + fabs((float)z-mun.z) >= 4.f)) break;
                        }
                        mpos[i].x = x; mpos[i].y = y; mpos[i].z = 0.f; mgoal[i] = mpos[i];
                        mvel[i].x = 0.f; mvel[i].y = 0.f; mvel[i].z = 0.f;

                        voxie_playsound("c:/windows/media/chimes.wav",-1,100,100,8.f);
                    }
                    else if (timdeath <= 0.0)
                    {
                        //ghost kills player
                        voxie_playsound("c:/windows/media/recycle.wav",-1,500,500,0.3f);
                        voxie_playsound("c:/windows/media/recycle.wav",-1,300,300,0.5f);
                        voxie_playsound("c:/windows/media/recycle.wav",-1,100,100,0.7f);
                        timdeath = 3.0; fscgoal = .6f;
                    }
                }
            }

            x = (int)(mun.x+cmun->xsiz+.5f)%cmun->xsiz;
            y = (int)(mun.y+cmun->ysiz+.5f)%cmun->ysiz;
            z = (int)(mun.z+cmun->zsiz+.5f)%cmun->zsiz;
            i = (z*cmun->ysiz+y)*cmun->xsiz+x;
            if (got[i])
            {
                got[i] = 0;
                if (cmun->board[i] == 'P') { voxie_playsound("c:/windows/media/tada.wav",-1,100,100,1.f); timpower = tim+8.f; } //power pellet
                else { voxie_playsound("c:/windows/media/notify.wav",-1,100,100,5.f); }
                numdots--;
                if (numdots <= 0)
                {
                    voxie_playsound("c:/windows/media/tada.wav",-1,100,100,1.f); timnextlev = tim+3.f;
                }
            }

            if (cmun->zsiz == 1) //2D
            {
                fscale = 1.f/min(min(vw.aspx,vw.aspy),vw.aspz/.2f);
                viewcent = mun;
            }
            else //3D
            {
                fsc = 1.f;
                fscale = (float)cmun->zsiz*.5f/vw.aspz;
                viewcent.x = mun.x;
                viewcent.y = mun.y;
                viewcent.z = (cmun->zsiz-1)*.5f;
            }
            voxie_setview(&vf,-vw.aspx*fscale,-vw.aspy*fscale,-vw.aspz*fscale,+vw.aspx*fscale,+vw.aspy*fscale,+vw.aspz*fscale);

            if (tim >= timnextlev)
            {     //draw muncher
                point3d pp, rr, dd, ff;
                pp.x = (mun.x-viewcent.x)*fsc;
                pp.y = (mun.y-viewcent.y)*fsc;
                pp.z = (mun.z-viewcent.z)*fsc;
                if (vel.z == 0.f)
                {
                    rr.x = vel.y*fsc; rr.y = vel.x*-fsc; rr.z = 0.00f;
                    dd.x = vel.x*fsc; dd.y = vel.y*fsc; dd.z = 0.00f;
                    ff.x = 0.00f; ff.y = 0.00f; ff.z = fsc;
                }
                else
                {
                    rr.x = fsc; rr.y = 0.00f; rr.z = 0.00f;
                    dd.x = 0.00f; dd.y = 0.00f; dd.z = vel.z*fsc;
                    ff.x = 0.00f; ff.y = -vel.z*fsc; ff.z = 0.00f;
                }
                if (timdeath > 0.0)
                {
                    rotate_vex(timdeath*12.f,&rr,&dd);
                    f = timdeath/3.0;
                    rr.x *= f; rr.y *= f;
                    dd.x *= f; dd.y *= f;
                    ff.z *= f;
                }
                i = min(max((int)((fabs(fabs(nmun.x-mun.x)+fabs(nmun.y-mun.y)+fabs(nmun.z-mun.z)-.5f))*16.f+0.5f),0),MUNCHFRAMES-1)+GHOSTFRAMES;
                voxie_drawspr(&vf,munnam[i],&pp,&rr,&dd,&ff,0x101010);
            }

            for(i=0;i<nmon;i++)
            {     //draw ghost
                point3d pp, rr, dd, ff;
                pp.x = (mpos[i].x-viewcent.x)*fsc;
                pp.y = (mpos[i].y-viewcent.y)*fsc;
                pp.z = (mpos[i].z-viewcent.z)*fsc;
                if (mvel[i].z == 0.f)
                {
                    rr.x = mvel[i].y*fsc; rr.y = mvel[i].x*-fsc; rr.z = 0.00f;
                    dd.x = mvel[i].x*fsc; dd.y = mvel[i].y*fsc; dd.z = 0.00f;
                    ff.x = 0.0f; ff.y = 0.0f; ff.z = fsc;
                }
                else
                {
                    rr.x = fsc; rr.y = 0.00f; rr.z = 0.00f;
                    dd.x = 0.00f; dd.y = 0.00f; dd.z = mvel[i].z*fsc;
                    ff.x = 0.00f; ff.y = -mvel[i].z*fsc; ff.z = 0.00f;
                }
                switch((int)(fmod(tim*4.0,1.0)*4.f)&3)
                {
                case 0: j = 0; break;
                case 1: j = 1; break;
                case 2: j = 1; rr.x = -rr.x; rr.y = -rr.y; rr.z = -rr.z; break;
                case 3: j = 0; rr.x = -rr.x; rr.y = -rr.y; rr.z = -rr.z; break;
                }
                voxie_drawspr(&vf,munnam[j],&pp,&rr,&dd,&ff,mcol[i]);
            }

            for(z=cmun->zwrap*-8;z<cmun->zsiz+cmun->zwrap*8;z++)
                for(y=cmun->ywrap*-8;y<cmun->ysiz+cmun->ywrap*8;y++)
                    for(x=cmun->xwrap*-8;x<cmun->xsiz+cmun->xwrap*8;x++)
                    {
                        if (mungetboard(cmun,x,y,z,'.') == '.') continue;

                        fx = ((float)x-viewcent.x)*fsc;
                        fy = ((float)y-viewcent.y)*fsc;
                        fz = ((float)z-viewcent.z)*fsc;

                        i = ((z+cmun->zsiz)%cmun->zsiz)*cmun->xsiz*cmun->ysiz
                                + ((y+cmun->ysiz)%cmun->ysiz)*cmun->xsiz
                                + ((x+cmun->xsiz)%cmun->xsiz);
                        if (got[i])
                        {
                            if (cmun->board[i] == 'P') //power pellet
                                voxie_drawsph(&vf,fx,fy,fz,fsc*.2f,1,0xffffff); //power pellet
                            else voxie_drawsph(&vf,fx,fy,fz,fsc*.1f,0,0xffff00); //regular dot
                        }

                        if (fx+fsc*.5f < -vw.aspx*fscale) continue;
                        if (fy+fsc*.5f < -vw.aspy*fscale) continue;
                        if (fz+fsc*.5f < -vw.aspz*fscale) continue;
                        if (fx-fsc*.5f > +vw.aspx*fscale) continue;
                        if (fy-fsc*.5f > +vw.aspy*fscale) continue;
                        if (fz-fsc*.5f > +vw.aspz*fscale) continue;

                        j = 0;
                        if (mungetboard(cmun,x-1,y,z,'.') != '.') j += (1<<0);
                        if (mungetboard(cmun,x+1,y,z,'.') != '.') j += (1<<1);
                        if (mungetboard(cmun,x,y-1,z,'.') != '.') j += (1<<2);
                        if (mungetboard(cmun,x,y+1,z,'.') != '.') j += (1<<3);
                        if (mungetboard(cmun,x,y,z-1,'.') != '.') j += (1<<4);
                        if (mungetboard(cmun,x,y,z+1,'.') != '.') j += (1<<5);
#if 0
                        i = 0x0000ff;
                        if (timpower >= tim)
                        {
                            f = timpower-tim;
                            if ((f > 1.0f) || (fabs(f-0.625) < .125) || (fabs(f-.125) < .125)) i = 0x00ffff;
                        }

                        switch (j)
                        {
                        //             11 2D cases (1 4 6|4 1): |  57 3D cases! (1 6 15 20 15|6 1)
                        // �������Ŀ 0xa 0x3 0xb 0x3 0x9        |  1 6-way path: axes
                        // �   �   � 0xc     0xc     0xc        |  6 5-way paths: Rubix centers
                        // �������Ĵ 0xe 0x3 0xf 0x3 0xd        | 15 4-way paths: 3 plus signs + 12 corners with 1 full axis
                        // �   �   � 0xc     0xc     0xc        | 20 3-way paths: 8 corners + 12 half plus sign Packer pieces
                        // ��������� 0x6 0x3 0x7 0x3 0x5        | 15 2-way paths: 3 straight lines + 12 Rubix edges
                        case 0x3: drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz,  0.f, 0.f,0.f,  0.f, 0.f,0.f, i); break;
                        case 0xc: drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz,  0.f, 0.f,0.f,  0.f, 0.f,0.f, i); break;

                        case 0xa: drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, -1.f,+1.f,0.f,  0.f, 0.f,0.f, i);
                            drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, +1.f,-1.f,0.f,  0.f, 0.f,0.f, i); break;
                        case 0x9: drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, +1.f,+1.f,0.f,  0.f, 0.f,0.f, i);
                            drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, -1.f,-1.f,0.f,  0.f, 0.f,0.f, i); break;
                        case 0x6: drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, -1.f,-1.f,0.f,  0.f, 0.f,0.f, i);
                            drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, +1.f,+1.f,0.f,  0.f, 0.f,0.f, i); break;
                        case 0x5: drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, +1.f,-1.f,0.f,  0.f, 0.f,0.f, i);
                            drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, -1.f,+1.f,0.f,  0.f, 0.f,0.f, i); break;

                        case 0xe: drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, -1.f,-1.f,0.f,  0.f,-1.f,0.f, i);
                            drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, -1.f,+1.f,0.f,  0.f,+1.f,0.f, i);
                            drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, +1.f,-1.f,0.f, +1.f,+1.f,0.f, i); break;
                        case 0xd: drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, +1.f,-1.f,0.f,  0.f,-1.f,0.f, i);
                            drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, +1.f,+1.f,0.f,  0.f,+1.f,0.f, i);
                            drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, -1.f,-1.f,0.f, -1.f,+1.f,0.f, i); break;
                        case 0xb: drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, -1.f,-1.f,0.f, -1.f, 0.f,0.f, i);
                            drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, +1.f,-1.f,0.f, +1.f, 0.f,0.f, i);
                            drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, -1.f,+1.f,0.f, +1.f,+1.f,0.f, i); break;
                        case 0x7: drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, -1.f,+1.f,0.f, -1.f, 0.f,0.f, i);
                            drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, +1.f,+1.f,0.f, +1.f, 0.f,0.f, i);
                            drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, -1.f,-1.f,0.f, +1.f,-1.f,0.f, i); break;

                        case 0xf: drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, +1.f,-1.f,0.f, +1.f,+1.f,0.f, i);
                            drawcone_bot(fx-fsc*.5f,fy,fz,fsc*.5f, fx+fsc*.5f,fy,fz,fsc*.5f, fx,fy,fz, -1.f,-1.f,0.f, -1.f,+1.f,0.f, i);
                            drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, -1.f,+1.f,0.f, +1.f,+1.f,0.f, i);
                            drawcone_bot(fx,fy-fsc*.5f,fz,fsc*.5f, fx,fy+fsc*.5f,fz,fsc*.5f, fx,fy,fz, -1.f,-1.f,0.f, +1.f,-1.f,0.f, i); break;
                        }
#elif 0
                        int m;
                        m = 0x000001;
                        if (timpower >= tim)
                        {
                            f = timpower-tim;
                            if ((f > 1.0f) || (fabs(f-0.625) < .125) || (fabs(f-.125) < .125)) m = 0x010000;
                        }

                        for(i=0;i<6;i++)
                        {
                            float fx2, fy2, fz2;
                            if (!(j&(1<<i))) continue;
                            fx2 = fx; fy2 = fy; fz2 = fz;
                            if (i == 0) fx2 -= fsc*.5f;
                            if (i == 1) fx2 += fsc*.5f;
                            if (i == 2) fy2 -= fsc*.5f;
                            if (i == 3) fy2 += fsc*.5f;
                            if (i == 4) fz2 -= fsc*.5f;
                            if (i == 5) fz2 += fsc*.5f;
                            //voxie_drawcone(&vf, fx,fy,fz,fsc*.25f, fx2,fy2,fz2,fsc*.25f, 1,0x000004);
                            for(k=0;k<4;k++)
                            {
                                voxie_drawsph(&vf, (fx2-fx)*((float)k+.5f)/4.f + fx,
                                              (fy2-fy)*((float)k+.5f)/4.f + fy,
                                              (fz2-fz)*((float)k+.5f)/4.f + fz, fsc*.6f, 0,m);

                                //draw_platonic(2,   (fx2-fx)*((float)k+.5f)/4.f + fx,
                                //                   (fy2-fy)*((float)k+.5f)/4.f + fy,
                                //                   (fz2-fz)*((float)k+.5f)/4.f + fz, fsc*1.f, 0.f,1,m);
                            }
                        }
#else
                        int m;
                        m = 0x80c0ff; i = 28;
                        if (timpower >= tim)
                        {
                            f = timpower-tim;
                            if ((f > 1.0f) || (fabs(f-0.625) < .125) || (fabs(f-.125) < .125)) { m = 0xffc080; i = 20; }
                        }
                        drawcyl(fx,fy,fz,fsc*.5f,m,j,6,i);
#endif
                    }

            {
                point3d pp, rr, dd;
                char buf[64];
                pp.x = 0.0f; pp.y = vw.aspy*fscale*-.99f; pp.z = -.18f*vw.aspz*fscale;
                rr.x = 0.2f*fscale; rr.y = 0.0f; rr.z = 0.0f;
                dd.x = 0.0f; dd.y = 0.0f; dd.z = 0.36f*fscale;
                sprintf(buf,"%d 2GO",numdots);
                f = (float)strlen(buf)*.5; pp.x -= rr.x*f; pp.y -= rr.y*f; pp.z -= rr.z*f;
                voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%s",buf);

                pp.x = 0.0f; pp.y = vw.aspy*fscale*+.99f; pp.z = -.18f*vw.aspz*fscale;
                rr.x = 0.2f*vw.aspy*fscale; rr.y = 0.0f; rr.z = 0.0f;
                dd.x = 0.0f; dd.y = 0.0f; dd.z = 0.36f*vw.aspy*fscale;
                sprintf(buf,"Level %d",munlev+1);
                f = (float)strlen(buf)*.5; pp.x -= rr.x*f; pp.y -= rr.y*f; pp.z -= rr.z*f;
                voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%s",buf);
            }

            //if (voxie_keystat(0x9c) == 1) timnextlev = tim+3.0; //debug only!

            if (tim < timnextlev)
            {
                point3d pp, rr, dd;
                pp.x =-1.00*fscale; rr.x = 0.15*fscale; dd.x = 0.00;
                pp.y = cos(tim*4.0)*-0.15*fscale; rr.y = 0.00; dd.y = cos(tim*4.0)*0.30*fscale;
                pp.z = sin(tim*4.0)*-0.15*fscale; rr.z = 0.00; dd.z = sin(tim*4.0)*0.30*fscale;
                voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"Level %d clear",munlev+1);
            }

        }
            break;
#endif


        case RENDMODE_SNAKETRON: //SnakeTron
        {
#define SNAKEMAX 4096
            typedef struct { point3d p[SNAKEMAX], dir, odir; float rad, spd; int i0, i1, score, reset; } snake_t;
            static snake_t snake[4];

#define PEL2MAX 16
            typedef struct { point3d p, v; int issph; } pel_t;
            static pel_t pel[PEL2MAX];
            static int peln;

#define SPLODEMAX 16
            typedef struct { point3d p; double tim; } splode_t;
            static splode_t splode[SPLODEMAX];
            static int sploden = 0;

            static int inited = 0, gotx, numplays = 2, winner;
            static double timend = 0.0;
            static const int shcol[4] = {0xffff00,0x00ffff,0xff00ff,0x00ff00};
            static const char *playst[4] = {"Yellow","Cyan","Magenta","Green"};
            int p;

            if (voxie_keystat(keyremap[0][6]) == 1) { inited = 0; break; } //ButM:reset&title
            if ((voxie_keystat(0x0e) == 1) || (ghitkey == 0x0e) || ((vx[0].but&~ovxbut[0])&(1<<4))) { ghitkey = 0; inited = 0; } //Backspace:reset
            if (!inited)
            {
                inited = 1; peln = gsnakenumpels+gsnakenumtetras;
                for(i=peln-1;i>=0;i--)
                {
                    pel[i].p.x = (float)((rand()&32767)-16384)/16384.f*vw.aspx;
                    pel[i].p.y = (float)((rand()&32767)-16384)/16384.f*vw.aspy;
                    pel[i].p.z = (float)((rand()&32767)-16384)/16384.f*vw.aspz;
                    vecrand(&pel[i].v,gsnakepelspeed);
                    pel[i].issph = (i < gsnakenumpels);

                }
                for(i=0;i<4;i++) { snake[i].reset = 1; snake[i].score = 0; snake[i].odir.x = 0.f; snake[i].odir.y = 0.f; snake[i].odir.z = 0.f; }

                timend = (double)gsnaketime + tim;
                winner = -2;
            }

            i = 0;
            if (voxie_keystat(0x27) == 1) { gsnakepelspeed = max(gsnakepelspeed*0.5f, 0.f); if (gsnakepelspeed <= 1.f/16.f) gsnakepelspeed = 0.f; i = 1; } //;
            if (voxie_keystat(0x28) == 1) { if (gsnakepelspeed < 1.f/16.f) gsnakepelspeed = 1.f/16.f; gsnakepelspeed = min(gsnakepelspeed*2.0f,2.f); i = 1; } //'
            if ((i) || (ghitkey == 0x27))
            {
                for(i=peln-1;i>=0;i--)
                {
                    f = pel[i].v.x*pel[i].v.x + pel[i].v.y*pel[i].v.y + pel[i].v.z*pel[i].v.z;
                    if (f > 0.f)
                    {
                        f = gsnakepelspeed/sqrt(f);
                        pel[i].v.x *= f;
                        pel[i].v.y *= f;
                        pel[i].v.z *= f;
                    }
                    else { vecrand(&pel[i].v,gsnakepelspeed); }
                }
            }

            voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,+vw.aspx,+vw.aspy,+vw.aspz);

            if ((gsnaketime <= 0) || (tim < timend))
            {
                numplays = max(vxnplays,gminplays);
                for(p=0;p<numplays;p++)
                {
                    if (snake[p].reset)
                    {
                        snake[p].reset = 0;
                        snake[p].i0 = 0; snake[p].i1 = 16; snake[p].spd = .02f; snake[p].rad = .05f;
                        if (p == 0) { snake[p].dir.x = 0.f; snake[p].dir.y =-1.f; snake[p].dir.z = 0.f; fx = +0.1f; fy = +vw.aspy; fz = -0.1f; }
                        if (p == 1) { snake[p].dir.x = 0.f; snake[p].dir.y =+1.f; snake[p].dir.z = 0.f; fx = -0.1f; fy = -vw.aspy; fz = -0.1f; }
                        if (p == 2) { snake[p].dir.x =-1.f; snake[p].dir.y = 0.f; snake[p].dir.z = 0.f; fx = +vw.aspx; fy = -0.1f; fz = +0.1f; }
                        if (p == 3) { snake[p].dir.x =+1.f; snake[p].dir.y = 0.f; snake[p].dir.z = 0.f; fx = -vw.aspx; fy = +0.1f; fz = +0.1f; }
                        snake[p].odir.x = 1.f; snake[p].odir.y = 1.f; snake[p].odir.z = 1.f;
                        for(i=snake[p].i0;i<snake[p].i1;i++)
                        {
                            snake[p].p[i].x = (float)i*snake[p].dir.x*snake[p].spd + fx;
                            snake[p].p[i].y = (float)i*snake[p].dir.y*snake[p].spd + fy;
                            snake[p].p[i].z = (float)i*snake[p].dir.z*snake[p].spd + fz;
                        }
                    }

                    snake[p].spd = max(snake[p].spd-dtim*.01f,.02f);
                    snake[p].rad = max(snake[p].rad-dtim*.01f,.05f);

                    if ((int)(otim*20.0) != (int)(tim*20.0))
                    {
                        fx = nav[p].dx*.001f;
                        fy = nav[p].dy*.001f;
                        fz = nav[p].dz*.001f;

                        if (gdualnav && (p)) //Super crappy AI
                        {
                            fx = cos(tim*1.1+(float)p);
                            fy = cos(tim*1.3+(float)p);
                            fz = cos(tim*1.7+(float)p);
                        }

                        if (voxie_keystat(keyremap[(p&3)*2][0])) { fx -= 1.f; } //Left
                        if (voxie_keystat(keyremap[(p&3)*2][1])) { fx += 1.f; } //Right
                        if (voxie_keystat(keyremap[(p&3)*2][2])) { fy -= 1.f; } //Up
                        if (voxie_keystat(keyremap[(p&3)*2][3])) { fy += 1.f; } //Down
                        if (voxie_keystat(keyremap[(p&3)*2][4])) { fz += 1.f; } //ButA
                        if (voxie_keystat(keyremap[(p&3)*2][5])) { fz -= 1.f; } //ButB

                        /************************************************/
                        /* adding Leap motion device, by Yi-Ting, Hsieh */
#if (USELEAP)

                        // calling Leap_Voxon for setting up leap motion service
                        leap_voxon.set_swipemode(true);
                        leap_voxon.getFrame();
                        leap_voxon.track_event();

                        // Draw hands if there is any hand on Canvas
                        if(leap_voxon.getNumHands()==1)
                        {
                            drawHands(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                            getIndexPos(leap_voxon.getHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode(),.2);
                        }
                        else if(leap_voxon.getNumHands()==2)
                        {
                            drawHands(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                            drawHands(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode());
                            getIndexPos(leap_voxon.getRightHand(),leap_voxon.getLeapScale(),leap_voxon.getRendmode(),.2);
                            // drawExitBox(leap_voxon.getLeftHand(),leap_voxon.getLeapScale(),.3);
                        }


                        if (checkExitBox == true)
                        {
                            grendmode = RENDMODE_TITLE; checkExitBox = false;
                            leap_voxon.set_swipemode(false);

                            menubox.timer = 0;
                            menubox.status = 2;
                            menubox.delayTimer = DELAYTIME;
                            menubox.fromTop = false;
                        }

#endif
                        /* end here */
                        /************************************************/

                        if (fabs(fx) + fabs(fy) + fabs(fz) > 0.f) { snake[p].dir.x = fx; snake[p].dir.y = fy; snake[p].dir.z = fz; }
                        if (p < vxnplays)
                        {
                            if (p == 0) { fx = vx[p].tx0; fy =-vx[p].ty0; fz =-vx[p].ty1; }
                            if (p == 1) { fx =-vx[p].tx0; fy = vx[p].ty0; fz =-vx[p].ty1; }
                            if (p == 2) { fx =-vx[p].ty0; fy =-vx[p].tx0; fz =-vx[p].ty1; }
                            if (p == 3) { fx = vx[p].ty0; fy = vx[p].tx0; fz =-vx[p].ty1; }
                            fz += (vx[p].rt - vx[p].lt)*128.0 - (vx[p].but&(1<<9))*128;
                            f = fx*fx + fy*fy + fz*fz;
                            if (f > 4096.0*4096.0) { f = 1.0/sqrt(f); snake[p].dir.x = fx*f; snake[p].dir.y = fy*f; snake[p].dir.z = fz*f; }
                        }

                        j = (snake[p].i1-1)&(SNAKEMAX-1);
                        if (fabs(snake[p].p[j].z + snake[p].dir.z*snake[p].spd) > vw.aspz) snake[p].dir.z = 0.f;

                        //Normalize
                        f = snake[p].dir.x*snake[p].dir.x + snake[p].dir.y*snake[p].dir.y + snake[p].dir.z*snake[p].dir.z;
                        if (f == 0.f)
                        {
                            snake[p].dir = snake[p].odir;
                            f = snake[p].dir.x*snake[p].dir.x + snake[p].dir.y*snake[p].dir.y + snake[p].dir.z*snake[p].dir.z;
                        }
                        if (f != 0.f)
                        {
                            f = 1.0/sqrt(f);
                            snake[p].dir.x *= f; if (snake[p].dir.x != 0.f) snake[p].odir.x = snake[p].dir.x;
                            snake[p].dir.y *= f; if (snake[p].dir.y != 0.f) snake[p].odir.y = snake[p].dir.y;
                            snake[p].dir.z *= f; if (snake[p].dir.z != 0.f) snake[p].odir.z = snake[p].dir.z;
                        }
                        else
                        {
                            voxie_playsound("c:/windows/media/chord.wav",-1,100,100,1.f); snake[p].reset = 1;
                            if (sploden < SPLODEMAX-1)
                            {
                                splode[sploden].p = snake[p].p[(snake[p].i1-1)&(SNAKEMAX-1)];
                                splode[sploden].tim = tim;
                                sploden++;
                            }
                        }

                        snake[p].i0++;
                        i = snake[p].i1&(SNAKEMAX-1); j = (snake[p].i1-1)&(SNAKEMAX-1);
                        snake[p].p[i].x = snake[p].p[j].x + snake[p].dir.x*snake[p].spd; if (snake[p].p[i].x < -vw.aspx) snake[p].p[i].x += vw.aspx*2.f; if (snake[p].p[i].x > +vw.aspx) snake[p].p[i].x -= vw.aspx*2.f;
                        snake[p].p[i].y = snake[p].p[j].y + snake[p].dir.y*snake[p].spd; if (snake[p].p[i].y < -vw.aspy) snake[p].p[i].y += vw.aspy*2.f; if (snake[p].p[i].y > +vw.aspy) snake[p].p[i].y -= vw.aspy*2.f;
                        snake[p].p[i].z = min(max(snake[p].p[j].z + snake[p].dir.z*snake[p].spd,-vw.aspz),+vw.aspz);
                        snake[p].i1++;
                        snake[p].score += (snake[p].i1-snake[p].i0)/16-1;

                        for(j=numplays-1;j>=0;j--)
                        {
                            if (j == p) i = snake[j].i1-12; else i = snake[j].i1-1;
                            for(;i>=snake[j].i0;i--)
                            {
                                fx = snake[p].p[(snake[p].i1-1)&(SNAKEMAX-1)].x-snake[j].p[i&(SNAKEMAX-1)].x;
                                fy = snake[p].p[(snake[p].i1-1)&(SNAKEMAX-1)].y-snake[j].p[i&(SNAKEMAX-1)].y;
                                fz = snake[p].p[(snake[p].i1-1)&(SNAKEMAX-1)].z-snake[j].p[i&(SNAKEMAX-1)].z;
                                if (fx*fx + fy*fy + fz*fz >= snake[p].rad*1.5*snake[j].rad*1.5) continue;
                                voxie_playsound("c:/windows/media/chord.wav",-1,100,100,1.f); snake[p].reset = 1;
                                if (sploden < SPLODEMAX-1)
                                {
                                    splode[sploden].p = snake[p].p[(snake[p].i1-1)&(SNAKEMAX-1)];
                                    splode[sploden].tim = tim;
                                    sploden++;
                                }
                                break;
                            }
                        }

                        for(i=peln-1;i>=0;i--)
                        {
                            pel[i].p.x += pel[i].v.x*dtim; if (pel[i].p.x < -vw.aspx) pel[i].p.x += vw.aspx*2.f; if (pel[i].p.x > +vw.aspx) pel[i].p.x -= vw.aspx*2.f;
                            pel[i].p.y += pel[i].v.y*dtim; if (pel[i].p.y < -vw.aspy) pel[i].p.y += vw.aspy*2.f; if (pel[i].p.y > +vw.aspy) pel[i].p.y -= vw.aspy*2.f;
                            pel[i].p.z += pel[i].v.z*dtim; if (fabs(pel[i].p.z) > vw.aspz) pel[i].v.z = fabs(pel[i].v.z)*(float)(1-(pel[i].p.z > 0.f)*2);

                            fx = snake[p].p[(snake[p].i1-1)&(SNAKEMAX-1)].x-pel[i].p.x;
                            fy = snake[p].p[(snake[p].i1-1)&(SNAKEMAX-1)].y-pel[i].p.y;
                            fz = snake[p].p[(snake[p].i1-1)&(SNAKEMAX-1)].z-pel[i].p.z;
                            if (fx*fx + fy*fy + fz*fz < snake[p].rad*2.f*.1f)
                            {
                                if (pel[i].issph)
                                {
                                    voxie_playsound("c:/windows/media/chimes.wav",-1,100,100,1.f);
                                    snake[p].i0 = max(max(snake[p].i0-16,0),snake[p].i1-SNAKEMAX);
                                    pel[i].p.x = (float)((rand()&32767)-16384)/16384.f*vw.aspx;
                                    pel[i].p.y = (float)((rand()&32767)-16384)/16384.f*vw.aspy;
                                    pel[i].p.z = (float)((rand()&32767)-16384)/16384.f*vw.aspz;
                                    vecrand(&pel[i].v,gsnakepelspeed);
                                }
                                else
                                {
#if 0
                                    //IMO speed boost on tetrahedron looks silly
                                    voxie_playsound("c:/windows/media/chimes.wav",-1,100,100,2.f);
                                    snake[p].spd = .08f;
                                    //snake[p].rad = .10f;
#else
                                    voxie_playsound("c:/windows/media/chord.wav",-1,100,100,1.f); snake[p].reset = 1;
                                    if (sploden < SPLODEMAX-1)
                                    {
                                        splode[sploden].p = snake[p].p[(snake[p].i1-1)&(SNAKEMAX-1)];
                                        splode[sploden].tim = tim;
                                        sploden++;
                                    }
#endif
                                }
                            }
                        }
                    }
                }

                //Draw pels (spheres and tetrahedrons) only during active game
                for(i=peln-1;i>=0;i--)
                {
                    if (pel[i].issph) voxie_drawsph(&vf,pel[i].p.x,pel[i].p.y,pel[i].p.z,0.05f,1,0xffffff);
                    else draw_platonic(0,pel[i].p.x,pel[i].p.y,pel[i].p.z,0.1f,tim*2.f,2,0xff00ff);

                }
            }

            //Draw snakes
            for(p=0;p<numplays;p++)
            {
                if ((gsnaketime > 0) && (winner != -2) && (winner != p)) continue;

                j = shcol[p];
                for(i=snake[p].i0;i<snake[p].i1;i++)
                {
                    f = snake[p].rad - min(snake[p].i1-1-i,12)/12.f*0.025f;
                    if (i >= snake[p].i1-12) j = 0xffffff;
                    voxie_drawsph(&vf,snake[p].p[i&(SNAKEMAX-1)].x,snake[p].p[i&(SNAKEMAX-1)].y,snake[p].p[i&(SNAKEMAX-1)].z,f,i==snake[p].i1-1,j);
                }
            }

            //Draw explosions
            for(i=sploden-1;i>=0;i--)
            {
                float fdotrad;
                f = tim-splode[i].tim; if (f >= 2.0) { sploden--; splode[i] = splode[sploden]; continue; }
                n = 128; fr = sqrt(f)*0.25; fdotrad = (1.f-f/2.0)*.007f;
                for(j=n-1;j>=0;j--)
                {
                    fz = ((double)j+.5)/(double)n*2.0-1.0; f = sqrt(1.0 - fz*fz);
                    g = (double)j*(PI*(sqrt(5.0)-1.0)); fx = cos(g)*f; fy = sin(g)*f;
                    voxie_drawsph(&vf,fx*fr+splode[i].p.x,
                                  fy*fr+splode[i].p.y,
                                  fz*fr+splode[i].p.z,fdotrad,0,0xffffff);
                }
            }


            {  //Draw scores
                static const int cornlut[4] = {0,2,3,1};
                char buf[256];
                point3d pp, rr, dd;
                for(i=0;i<numplays;i++)
                {
                    j = cornlut[i];
                    rr.x = cos(((float)j+0.0)*PI*2.0/4.0);
                    rr.y = sin(((float)j+0.0)*PI*2.0/4.0);
                    rr.z = 0.f;
                    dd.x =-sin(((float)j+0.0)*PI*2.0/4.0);
                    dd.y = cos(((float)j+0.0)*PI*2.0/4.0);
                    dd.z = 0.f;
                    pp.x = dd.x*(vw.aspx-.2f);
                    pp.y = dd.y*(vw.aspy-.2f);
                    pp.z = vw.aspz*.95f;
                    f = .12f; rr.x *= f; rr.y *= f; rr.z *= f;
                    f = .20f; dd.x *= f; dd.y *= f; dd.z *= f;
                    sprintf(buf,"%d",snake[i].score);
                    pp.x -= (float)strlen(buf)*rr.x*.5;
                    pp.y -= (float)strlen(buf)*rr.y*.5;
                    pp.z -= (float)strlen(buf)*rr.z*.5;
                    voxie_printalph_(&vf,&pp,&rr,&dd,shcol[i],"%s",buf);
                }

                //Draw timer (if applicable)
                if (gsnaketime > 0)
                {
                    rr.x = .12f; rr.y = 0.f; rr.z = 0.f;
                    dd.x = 0.f; dd.y = .20f; dd.z = 0.f;
                    pp.x = 0.f; pp.y = 0.f; pp.z = vw.aspz-0.01f;
                    f = timend-tim;
                    if (f > 0.f)
                    {
                        i = (int)(f)+1;
                        if (i < 3600) { sprintf(buf,"%d:%02d",i/60,i%60); }
                        else { sprintf(buf,"%d:%02d:%02d",i/3600,(i/60)%60,i%60); }
                        p = 0xffffff;
                    }
                    else
                    {
                        if (winner == -2)
                        {
                            winner = 0; j = 0;
                            for(i=numplays-1;i>0;i--)
                                if (snake[i].score >= snake[winner].score)
                                { j = (snake[i].score == snake[winner].score); winner = i; }
                            if (j) winner = -1;
                        }
                        if (winner < 0)
                        {
                            sprintf(buf,"Game Over");
                        }
                        else
                        {
                            //Make winning snake go in circle
                            if ((int)(otim*20.0) != (int)(tim*20.0))
                            {
                                p = winner;

                                snake[p].i0++;
                                i = snake[p].i1&(SNAKEMAX-1); j = (snake[p].i1-1)&(SNAKEMAX-1);
                                snake[p].dir.x = cos(tim*0.5)*vw.aspx - snake[p].p[j].x;
                                snake[p].dir.y = sin(tim*0.5)*vw.aspy - snake[p].p[j].y;
                                snake[p].dir.z = sin(tim*4.0)*vw.aspz - snake[p].p[j].z;
                                f = snake[p].dir.x*snake[p].dir.x + snake[p].dir.y*snake[p].dir.y + snake[p].dir.z*snake[p].dir.z;
                                if (f > 0.f) { f = 1.f/sqrt(f); snake[p].dir.x *= f; snake[p].dir.y *= f; snake[p].dir.z *= f; }
                                snake[p].p[i].x = snake[p].p[j].x + snake[p].dir.x*snake[p].spd;
                                snake[p].p[i].y = snake[p].p[j].y + snake[p].dir.y*snake[p].spd;
                                snake[p].p[i].z = snake[p].p[j].z + snake[p].dir.z*snake[p].spd;
                                snake[p].i1++;
                            }

                            sprintf(buf,"%s Wins!",playst[winner]);
                            p = shcol[winner];
                        }
                    }
                    pp.x -= ((float)strlen(buf)*rr.x + dd.x)*.5;
                    pp.y -= ((float)strlen(buf)*rr.y + dd.y)*.5;
                    pp.z -= ((float)strlen(buf)*rr.z + dd.z)*.5;
                    voxie_printalph_(&vf,&pp,&rr,&dd,p,"%s",buf);
                }

            }

        }
            break;

        case RENDMODE_FLYSTOMP: //FlyStomp
        {
            typedef struct { float x, y, z, xv, yv, zv, ha, va, onfloor, flaptim; int sc; } fly_t;
#define FLYMAX 4
            static fly_t fly[FLYMAX];
            static int flyn = 0;

            typedef struct { float x, y, z, xs, ys; } plat_t;
#define PLATMAX 16
            static plat_t plat[PLATMAX];
            static int platn = 0;

            typedef struct { float x, y, z, xv, yv, zv, rad; int col; float dtim; } part_t;
#define PARTMAX 4096
            static part_t part[PARTMAX];
            static int partn = 0;

            static int inited = 0;
            point3d pp, rr, dd, ff;
            float fx, fy, fz, dmin, d;

            if (voxie_keystat(keyremap[0][6]) == 1) { inited = 0; break; } //ButM:reset&title
            if ((voxie_keystat(0x0e) == 1) || (ghitkey == 0x0e) || ((vx[0].but&~ovxbut[0])&(1<<4))) { ghitkey = 0; inited = 0; } //Backspace:reset
            if (!inited)
            {
                voxie_mountzip("flystomp.zip");
                inited = 1;

                flyn = 0;

                fly[flyn].x =-.5f; fly[flyn].xv = 0.f;
                fly[flyn].y =-.2f; fly[flyn].yv = 0.f;
                fly[flyn].z =+.2f; fly[flyn].zv = 0.f; fly[flyn].ha = 0.f; fly[flyn].va = 0.f; fly[flyn].onfloor = 0.f; fly[flyn].sc = 0; fly[flyn].flaptim = -1.0; flyn++;

                fly[flyn].x =+.5f; fly[flyn].xv = 0.f;
                fly[flyn].y =+.2f; fly[flyn].yv = 0.f;
                fly[flyn].z =+.2f; fly[flyn].zv = 0.f; fly[flyn].ha = 0.f; fly[flyn].va = 0.f; fly[flyn].onfloor = 0.f; fly[flyn].sc = 0; fly[flyn].flaptim = -1.0; flyn++;

                platn = 0;

                plat[platn].x = 0.f; plat[platn].y = 0.f; plat[platn].z = vw.aspz; plat[platn].xs = vw.aspx; plat[platn].ys = vw.aspy; platn++;
                plat[platn].x = -0.7f; plat[platn].y = -0.25f; plat[platn].z = -0.05f; plat[platn].xs = 0.2f; plat[platn].ys = 0.20f; platn++;
                plat[platn].x = +0.5f; plat[platn].y = -0.25f; plat[platn].z = +0.01f; plat[platn].xs = 0.3f; plat[platn].ys = 0.15f; platn++;
                plat[platn].x = -0.4f; plat[platn].y = +0.25f; plat[platn].z = -0.05f; plat[platn].xs = 0.3f; plat[platn].ys = 0.15f; platn++;
                plat[platn].x = +0.6f; plat[platn].y = +0.25f; plat[platn].z = +0.13f; plat[platn].xs = 0.2f; plat[platn].ys = 0.20f; platn++;
            }

            voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,+vw.aspx,+vw.aspy,+vw.aspz);

            for(i=0;i<flyn;i++)
            {
#if 0
                if (i < vxnplays)
                {
                    fly[i].xv += (float)(vx[i].tx0+vx[i].tx1)*dtim*+.00005f;
                    fly[i].yv += (float)(vx[i].ty0+vx[i].ty1)*dtim*-.00005f;
                }
                if (voxie_keystat(keyremap[i*2][0])) { fly[i].xv -= dtim*1.f; } //Left
                if (voxie_keystat(keyremap[i*2][1])) { fly[i].xv += dtim*1.f; } //Right
                if (voxie_keystat(keyremap[i*2][2])) { fly[i].yv -= dtim*1.f; } //Up
                if (voxie_keystat(keyremap[i*2][3])) { fly[i].yv += dtim*1.f; } //Down
#else
                fx = 0.f;
                fy = 0.f;
                if (i < vxnplays)
                {
                    fx += (float)(vx[i].tx0+vx[i].tx1)*dtim*+.00005f;
                    fy += (float)(vx[i].ty0+vx[i].ty1)*dtim*-.00005f;
                }
                if (voxie_keystat(keyremap[i*2][0])) { fx -= dtim*1.f; } //Left
                if (voxie_keystat(keyremap[i*2][1])) { fx += dtim*1.f; } //Right
                if (voxie_keystat(keyremap[i*2][2])) { fy -= dtim*1.f; fly[i].va += (-.8f - fly[i].va)*.2; } //Up
                if (voxie_keystat(keyremap[i*2][3])) { fy += dtim*1.f; fly[i].va += (+.8f - fly[i].va)*.2; } //Down
                fly[i].va *= .95f;

                fly[i].ha += fx*2.5f;
                fly[i].xv += sin(fly[i].ha)*fy*.5f;
                fly[i].yv -= cos(fly[i].ha)*fy*.5f;
#endif
                if ((voxie_keystat(keyremap[i*2][4]) == 1) || (vx[i].but&~ovxbut[i]&(1<<12))) //ButA
                {
                    fly[i].zv -= 0.25f;
                    voxie_playsound("flap.flac",-1,100,100,1.f);
                    fly[i].flaptim = tim;
                }

                fly[i].zv += dtim*1.f; //Gravity

                //Max speed
                f = sqrt(fly[i].xv*fly[i].xv + fly[i].yv*fly[i].yv);
                if (f >= 1.f) { f = 1.f/f; fly[i].xv *= f; fly[i].yv *= f;  }

                for(j=8-1;j>0;j--)
                {
                    f = dtim*1.f;
                    fx = fly[i].x + fly[i].xv*f*(float)((j>>0)&1);
                    fy = fly[i].y + fly[i].yv*f*(float)((j>>1)&1);
                    fz = fly[i].z + fly[i].zv*f*(float)((j>>2)&1);

                    dmin = 1e32;
                    for(k=platn-1;k>=0;k--)
                    {
                        d = dist2plat2(fx,fy,fz,plat[k].x,plat[k].y,plat[k].z,plat[k].xs,plat[k].ys);
                        if (d < dmin) dmin = d;
                    }
                    if (dmin < .05f*.05f) continue;
                    f = 1.f/dtim;
                    fly[i].xv = (fx-fly[i].x)*f;
                    fly[i].yv = (fy-fly[i].y)*f;
                    fly[i].zv = (fz-fly[i].z)*f;
                    fly[i].x = fx;
                    fly[i].y = fy;
                    fly[i].z = fz;
                    break;
                }

                if (j < 7)
                {
                    if (fly[i].onfloor < 0.f) voxie_playsound("plop.flac",-1,25,25,1.f);
                    fly[i].onfloor = .25;
                    fly[i].xv *= pow(.05,dtim);
                    fly[i].yv *= pow(.05,dtim);
                }
                else
                {
                    fly[i].onfloor -= dtim;
                    fly[i].xv *= pow(.6,dtim);
                    fly[i].yv *= pow(.6,dtim);
                    fly[i].zv *= pow(.6,dtim);
                }


                if (fly[i].x < -vw.aspx) fly[i].x += vw.aspx*2.f;
                if (fly[i].x > +vw.aspx) fly[i].x -= vw.aspx*2.f;
                if (fly[i].y < -vw.aspy) fly[i].y += vw.aspy*2.f;
                if (fly[i].y > +vw.aspy) fly[i].y -= vw.aspy*2.f;
                if (fly[i].z < -vw.aspz) fly[i].zv = fabs(fly[i].zv);
                if (fly[i].z > +vw.aspz) fly[i].z = +vw.aspz;

#if 0
                fly[i].ha = atan2(-fly[i].xv,fly[i].yv);
#endif

                f = 0; if (tim-fly[i].flaptim < .25) f = sin((tim-fly[i].flaptim)*PI*2/.25)*.5;
                drawbird(fly[i].x,fly[i].y,fly[i].z,fly[i].ha+PI*.5f,PI*.5f+fly[i].va,f,.25f,(0x00ffff-0xffff00)*i + 0xffff00);

                for(j=0;j<flyn;j++)
                {
                    if (i == j) continue;
                    d = sqrt((fly[i].x-fly[j].x)*(fly[i].x-fly[j].x) +
                             (fly[i].y-fly[j].y)*(fly[i].y-fly[j].y) +
                             (fly[i].z-fly[j].z)*(fly[i].z-fly[j].z));
                    if (d >= 0.1) continue;
                    if (fly[i].z < fly[j].z)
                    {
                        for(k=256;k>0;k--)
                        {
                            if (partn >= PARTMAX-1) break;
                            do
                            {
                                fx = ((rand()/32768.f)-.5f)*2.f;
                                fy = ((rand()/32768.f)-.5f)*2.f;
                                fz = ((rand()/32768.f)-.5f)*2.f;
                            } while (fx*fx + fy*fy + fz*fz > 1.f*1.f);
                            part[partn].x = fly[j].x + fx*.1f;
                            part[partn].y = fly[j].y + fy*.1f;
                            part[partn].z = fly[j].z + fz*.1f;
                            do
                            {
                                fx = ((rand()/32768.f)-.5f)*2.f;
                                fy = ((rand()/32768.f)-.5f)*2.f;
                                fz = ((rand()/32768.f)-.5f)*2.f;
                            } while (fx*fx + fy*fy + fz*fz > 1.f*1.f);
                            part[partn].xv = fx*.5f;
                            part[partn].yv = fy*.5f;
                            part[partn].zv = fz*.5f;
                            part[partn].col = ((rand()&1)<<7) + ((rand()&1)<<15) + ((rand()&1)<<23);
                            part[partn].rad = (rand()/32768.0)*0.01+0.005;
                            part[partn].dtim = (rand()/32768.0)*1.f + 2.f;
                            partn++;
                        }

                        fly[j].x = (float)(rand()&32767)/32768.0*2.0*vw.aspx - vw.aspx;
                        fly[j].y = (float)(rand()&32767)/32768.0*2.0*vw.aspy - vw.aspy;
                        fly[j].z = -vw.aspz;
                        fly[i].sc++;
                        voxie_playsound("blowup2.flac",-1,40,40,1.f);
                    }
                }
            }

            for(i=0;i<platn;i++)
            {
                //voxie_drawbox(&vf,plat[i].x-plat[i].xs,plat[i].y-plat[i].ys,plat[i].z-plat[i].zs,
                //                  plat[i].x+plat[i].xs,plat[i].y+plat[i].ys,plat[i].z+plat[i].zs,1,0xffffff);

                pp.x = plat[i].x; pp.y = plat[i].y; pp.z = plat[i].z;
                rr.x = plat[i].xs*2.f; rr.y = 0.f; rr.z = 0.f;
                dd.x = 0.f; dd.y = plat[i].ys*2.f; dd.z = 0.f;
                ff.x = 0.f; ff.y = 0.f; ff.z = 0.25f;
                if (!i) voxie_drawspr(&vf,"lava.kv6",&pp,&rr,&dd,&ff,0x404040);
                else voxie_drawspr(&vf,"emuplatform.kv6",&pp,&rr,&dd,&ff,0x404040);

                voxie_drawcone(&vf,pp.x,pp.y,pp.z,-.02f,pp.x,pp.y,vw.aspz,-.02f,0,0x201810);
            }

            //Process particles..
            for(i=partn-1;i>=0;i--)
            {
                part[i].x += part[i].xv*dtim;
                part[i].y += part[i].yv*dtim;
                part[i].z += part[i].zv*dtim;
                part[i].xv *= pow(.125,dtim);
                part[i].yv *= pow(.125,dtim);
                part[i].zv *= pow(.125,dtim);
                part[i].zv += dtim*.5f;
                if (part[i].z > vw.aspz) { part[i].zv = -fabs(part[i].zv); }

                //voxie_drawvox(&vf,part[i].x,part[i].y,part[i].z,part[i].col);
                voxie_drawsph(&vf,part[i].x,part[i].y,part[i].z,part[i].rad,0,part[i].col);
                part[i].dtim -= dtim;
                if (part[i].dtim < 0.f)
                {
                    partn--; part[i] = part[partn];
                    continue;
                }
            }

            f = 0.f;
            for(i=1;i<4;i++)
            {
                rr.x = 0.12f; dd.x = 0.00f; pp.x = -0.4f - rr.x*f - dd.x*.5f;
                rr.y = 0.00f; dd.y = 0.00f; pp.y = -vw.aspy - rr.y*f - dd.y*.5f + (float)i*.01f;
                rr.z = 0.00f; dd.z = 0.25f; pp.z = -0.17f - rr.z*f - dd.z*.5f;
                voxie_printalph_(&vf,&pp,&rr,&dd,0xffff00,"%d",fly[0].sc);

                rr.x = 0.12f; dd.x = 0.00f; pp.x = +0.3f - rr.x*f - dd.x*.5f;
                rr.y = 0.00f; dd.y = 0.00f; pp.y = -vw.aspy - rr.y*f - dd.y*.5f + (float)i*.01f;
                rr.z = 0.00f; dd.z = 0.25f; pp.z = -0.17f - rr.z*f - dd.z*.5f;
                voxie_printalph_(&vf,&pp,&rr,&dd,0x00ffff,"%d",fly[1].sc);
            }

        }
            break;

        }


        #if (USEMAG6D)
        		if (gusemag6d > 0)
        		{
        				//Draw mag6d axes
        			voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,+vw.aspx,+vw.aspy,+vw.aspz);
        			f = .25f;
        			voxie_drawcone(&vf,m6p.x,m6p.y,m6p.z,.02f,m6p.x+m6r.x*f,m6p.y+m6r.y*f,m6p.z+m6r.z*f,.02f,1,0xff0000);
        			voxie_drawcone(&vf,m6p.x,m6p.y,m6p.z,.02f,m6p.x+m6d.x*f,m6p.y+m6d.y*f,m6p.z+m6d.z*f,.02f,1,0x00ff00);
        			voxie_drawcone(&vf,m6p.x,m6p.y,m6p.z,.02f,m6p.x+m6f.x*f,m6p.y+m6f.y*f,m6p.z+m6f.z*f,.02f,1,0x0000ff);
        			i = 0xffffff;
        			if (m6but) i = (m6but&1)*0xff0000 + ((m6but>>1)&1)*0x00ff00 + ((m6but>>2)&1)*0x0000ff;
        			voxie_drawsph(&vf,m6p.x,m6p.y,m6p.z,.03f,1,i);
        		}
        #endif

        		voxie_setview(&vf,0.0,0.0,-256,gxres,gyres,+256); //old coords

        		if ((grendmode == RENDMODE_KEYSTONECAL) || ((gshowborder) && ((grendmode == RENDMODE_DOTMUNCH) || (grendmode == RENDMODE_SNAKETRON) || (grendmode == RENDMODE_MODELANIM))))
        		{
        				//draw wireframe box
        			voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,+vw.aspx,+vw.aspy,+vw.aspz);
        			voxie_drawbox(&vf,-vw.aspx+1e-3,-vw.aspy+1e-3,-vw.aspz,+vw.aspx-1e-3,+vw.aspy-1e-3,+vw.aspz,1,0xffffff);
        		}

        		avgdtim += (dtim-avgdtim)*.1;
        		if (voxie_keystat(0xb8) || (showphase)) //R.Alt
        		{
        			point3d pp, rr, dd;
        			voxie_setview(&vf,-vw.aspx,-vw.aspy,-vw.aspz,+vw.aspx,+vw.aspy,+vw.aspz);
        			rr.x = 0.125f; dd.x = 0.00f; pp.x = vw.aspx*-.99f;
        			rr.y = 0.00f;  dd.y = 0.25f; pp.y = vw.aspy*-.99f;
        			rr.z = 0.00f;  dd.z = 0.00f; pp.z = vw.aspz-0.01f;
        								voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%8.5f  %.4f",vw.freq,vw.phase);
        			pp.z *= -1.f;  voxie_printalph_(&vf,&pp,&rr,&dd,0xffffff,"%.1f",1.0/avgdtim);
        		}

        voxie_frame_end(); voxie_getvw(&vw); numframes++;
    }

    voxie_uninit(0); //Close window and unload voxiebox.dll
#if (USEMAG6D)
    	if (gusemag6d >= 0) mag6d_uninit();
#endif
#if (USELEAP)
    	leap_uninit();
#endif
    return(0);
}

#if 0
!endif
#endif
