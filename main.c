/* *********************************************************************
      GG Test Suite (forked from SMS Test Suite) - by sverx
           ("largely inspired by Artemio's 240p, but not a fork of it!")
************************************************************************ */

#define MAJOR_VER 0
#define MINOR_VER 11

#include <stdio.h>
#include <stdbool.h>

#define TARGET_GG

#include "../SMSlib/SMSlib.h"
#include "../PSGlib/PSGlib.h"

#include "bank1.h"

#define MAIN_MENU_ITEMS 4
const unsigned char * const main_menu[MAIN_MENU_ITEMS] =   {"Video Tests",
                                                            "Audio Tests",
                                                            "Pad Tests",
                                                            "System Info"};

#define VIDEO_MENU_ITEMS 10
const unsigned char * const video_menu[VIDEO_MENU_ITEMS] = {"PLUGE",
                                                            "Color bars",
                                                            "Color bleed",
                                                            "Grid",
                                                            "Patterns",
                                                            "Full colors",
                                                            "Linearity",
                                                            "Drop Shadow",
                                                            "Striped Sprite",
                                                            "[ back ]"};

#define GG_BIOSES_ITEMS 2
const struct {
  const unsigned int sum1k;
  const unsigned char * const name;
} GG_BIOSes[GG_BIOSES_ITEMS] = {
  {0x5e3a,"  'Majesco' BIOS  "},
  {0x207e,"Emulicious GG BIOS"}       //  ;)
};

unsigned char const stereomode[3]={0xFF, 0xF0, 0x0F};  // stereo, left only, right only

/* define GGRegionPort */
__sfr __at 0x00 GGRegionPort;

/* define GGStereoPort */
__sfr __at 0x06 GGStereoPort;

#define MENU_FIRST_ROW  8
#define MENU_FIRST_COL  8

#define FOOTER_COL  8
#define FOOTER_ROW  19

unsigned char cur_menu_item, main_menu_items, pointer_anim;

/* hardware tests results */
bool has_BIOS_GG, is_Japanese, do_Port3E_works, CMOS_CPU;

/*  ****************** for PADS TESTS *********************** */

#define COLOR_UP         3
#define COLOR_DOWN       1
#define COLOR_LEFT       10
#define COLOR_RIGHT      9
#define COLOR_1          15
#define COLOR_2          2
#define COLOR_START      11

#define HILIT_COLOR      0x07FF
#define BLACK            0x00

#define PAD_TIMEOUT      (60*3)

/*  *********** for running code in RAM *******************  */

#define CODE_IN_RAM_SIZE  256
#define TEMP_BUF_SIZE     (4*1024)

// #define CART_CHECK_ADDR   0x7F00
#define CART_CHECK_ADDR   0x0100
#define CART_CHECK_SIZE   256

unsigned char pause_cnt;
unsigned int kp,kr,ks;
unsigned char code_in_RAM[CODE_IN_RAM_SIZE];   // 256 bytes should be enough for everything
unsigned char temp_buf[TEMP_BUF_SIZE];

#define BIOS_SIZE_8K   0x20
#define BIOS_SIZE_1K   0x04

/*  **************** [[[ CODE ]]] ************************** */

unsigned int compute_BIOS_sum (void) __naked {
  /* *************************
     NOTE: this code will be copied to RAM and run from there!
     *************************  */
  __asm
    di               ; interrupts should be disabled!

    ld a,#0b11100011 ; reset bits 4,3 (enable RAM/BIOS) and set bits 5,6,7 (to disable card/cartridge/expansion)
    out (#0x3E),a    ; do!

    ld de,#0         ; sum
    ld hl,#0x0000    ; src
loop:
    ld a,(hl)        ; a=*src
    add a,e
    ld e,a
    jr nc,nocarry
    inc d
nocarry:
    inc hl           ; src++
    ld a,h
    cp c             ; size to check preloaded in C by caller
    jr nz,loop
    ex de,hl         ; hl=sum

    ; ld a,#0b10101011 ; reset bits 4,6 (enable RAM/cartridge) and set bits 3,5,7 (to disable BIOS/card/expansion)
    ld a,(_SMS_Port3EBIOSvalue)
    out (#0x3E),a    ; restore it

    ei               ; re-enable interrupts!

    ret              ; because I am naked ;)
  __endasm;
}

#pragma save
#pragma disable_warning 85
unsigned int get_BIOS_sum (unsigned char size) __naked __z88dk_fastcall {
  __asm
    ld a,l                         ; save size in A
    ld hl,#_compute_BIOS_sum
    ld de,#_code_in_RAM
    ld bc,#CODE_IN_RAM_SIZE
    ldir                           ; copy code in RAM
    ld c,a                         ; restore size in C
    jp _code_in_RAM
  __endasm;
}
#pragma restore

void ldir_BIOS_SRAM (void) __naked {
  /* *************************
     NOTE: this code will be copied to RAM and run from there!
     *************************  */
  __asm
    di               ; interrupts should be disabled!

    ; 1st half ***********

    ld hl,#0x0000    ; src (BIOS)
    ld de,#0x8000    ; dst (SRAM)
    ld b,#4          ; 4 times (4 times 4KB)

outloop1:
    push bc
      ld a,#0b11100011    ; reset bits 3,4 (enable BIOS/RAM) and set bits 5,6,7 (to disable card/cartridge/expansion)
      out (#0x3E),a       ; do!

      push de
        ld de,#_temp_buf         ; dst (RAM)
        ld bc,#TEMP_BUF_SIZE     ; 4K
        ldir

        ; ld a,#0b10101011  ; reset bits 4,6 (enable RAM/cartridge) and set bits 3,5,7 (to disable BIOS/card/expansion)
        ld a,(_SMS_Port3EBIOSvalue)
        out (#0x3E),a     ; restore it
      pop de
      push hl
        ld hl,#_temp_buf         ; src (RAM)
        ld bc,#TEMP_BUF_SIZE     ; 4K
        ldir
      pop hl

    pop bc
    djnz outloop1

/* *************************************

    // ---> My Master EverDrive is the old model
    //      and doesn't seem to support 32 KB saves :(

    ld a,#0x0C       ; select SRAM bank 2
    ld (#0xfffc),a

    ; 2nd half ***********

    ld b,#4          ; 4 times
    ld de,#0x8000    ; dst (SRAM)

outloop2:
    push bc

      ld a,#0b11100011    ; reset bits 3,4 (enable BIOS/RAM) and set bits 5,6,7 (to disable card/cartridge/expansion)
      out (#0x3E),a       ; do!

      push de
        ld de,#_temp_buf         ; dst (RAM)
        ld bc,#TEMP_BUF_SIZE     ; 4K
        ldir

        ld a,#0b10101011  ; reset bits 4,6 (enable RAM/cartridge) and set bits 3,5,7 (to disable BIOS/card/expansion)
        out (#0x3E),a     ; restore it
      pop de
      push hl
        ld hl,#_temp_buf         ; src (RAM)
        ld bc,#TEMP_BUF_SIZE     ; 4K
        ldir
      pop hl

    pop bc
    djnz outloop2

 *************************************  */

    ei               ; re-enable interrupts!

    ret              ; because I am naked ;)
  __endasm;
}

void dump_BIOS (void) __naked {
  __asm
    ld hl,#_ldir_BIOS_SRAM
    ld de,#_code_in_RAM
    ld bc,#CODE_IN_RAM_SIZE
    ldir                           ; copy code in RAM
    jp _code_in_RAM
  __endasm;
}

unsigned char detect_Port3E_match (void)  __naked {
  /* *************************
     NOTE: this code will be copied to RAM and run from there!
     *************************  */
  __asm
    di               ; interrupts should be disabled!

    ld a,(_SMS_Port3EBIOSvalue)
    ; or #0xE0         ; set bits 5,6,7 (to disable card/cartridge/expansion)
    or l             ; set these bits to disable medias/stuff
    and h            ; reset these bits to enable medias/stuff

    out (#0x3E),a    ; do! (should have NO effect on a GameGear and on a Mark III)

    ld hl,#CART_CHECK_ADDR
    ld de,#_temp_buf
    ld b,#CART_CHECK_SIZE
match_loop:
    ld a,(de)
    cp (hl)
    jr nz,no_match   ; check if I can still read from card/cartridge/expansion
    inc hl
    inc de
    djnz match_loop
    ld l,#0          ; I can: port3E is *not* effective
    jr cont

no_match:
    ld l,#1          ; I failed: port3E *is* effective

cont:
    ld a,(_SMS_Port3EBIOSvalue)
    out (#0x3E),a    ; restore port 0x3E
    ei               ; re-enable interrupts!

    ret              ; because I am naked ;)
  __endasm;
}

#pragma save
#pragma disable_warning 85
bool is_Port3E_effective (unsigned int masks) __naked __z88dk_fastcall {
  __asm
    push hl
      ld hl,#CART_CHECK_ADDR
      ld de,#_temp_buf
      ld bc,#CART_CHECK_SIZE
      ldir                           ; copy some bytes from card/cartridge/expansion to RAM
      ld hl,#_detect_Port3E_match
      ld de,#_code_in_RAM
      ld bc,#CODE_IN_RAM_SIZE
      ldir                           ; copy code in RAM
    pop hl
    jp _code_in_RAM
  __endasm;
}
#pragma restore

void draw_footer_and_ver (void) {

  // print region
  SMS_setNextTileatXY(FOOTER_COL,FOOTER_ROW);
  printf (" Region:");
  if (is_Japanese)
    printf ("JPN/KOR ");                 // Japanese/Korean
  else
    printf ("EXPORT  ");

  // print BIOS info
  SMS_setNextTileatXY(FOOTER_COL,FOOTER_ROW+1);
  printf ("   BIOS:");
  if (has_BIOS_GG)
    printf ("present ");
  else
    printf ("absent  ");

  // print program version (just under the title)
  SMS_setNextTileatXY(7,5);
  printf ("v%d.%02d",MAJOR_VER,MINOR_VER);
}

void draw_menu (unsigned char *menu[], unsigned int max) {
  unsigned char i;
  for (i=0;i<max;i++) {
    SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+i);
    printf ("%-11s",menu[i]);
  }
}

void load_menu_assets (void) {
  // load background tiles, map and palette
  SMS_loadPSGaidencompressedTiles (BG__tiles__psgcompr,96);
  SMS_loadSTMcompressedTileMap (0,0,BG__tilemap__stmcompr);
  GG_loadBGPalette(BG__palette__bin);
  // load sprite tile, build palette, turn on text renderer
  SMS_loadPSGaidencompressedTiles (arrow__tiles__psgcompr,256);
  GG_setSpritePaletteColor (0, RGB(0,0,0));
  GG_setSpritePaletteColor (1, RGB(15,15,15));
  SMS_autoSetUpTextRenderer();
}

void static_screen (void* tiles, void* tilemap, void* palette, void* alt_tiles) {
  bool alt=false;
  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_loadPSGaidencompressedTiles (tiles,0);
  SMS_loadSTMcompressedTileMap (0,0,tilemap);
  GG_loadBGPalette(palette);
  SMS_displayOn();
  for (;;) {
    SMS_waitForVBlank();
    kp=SMS_getKeysPressed();
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2))
      break;
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1))
      if (alt_tiles==NULL)
        break;
      else
        alt=!alt, SMS_loadPSGaidencompressedTiles (((alt)?alt_tiles:tiles),0);
  }
  SMS_displayOff();
}

void color_cycle  (void* tiles, void* tilemap) {
  unsigned char i,color_selection=0;
  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_loadPSGaidencompressedTiles (tiles,0);
  SMS_loadSTMcompressedTileMap (0,0,tilemap);
  GG_setNextBGColoratIndex(0);
  for (i=0;i<16;i++)
    GG_setColor(RGB(i,i,i));
  SMS_displayOn();
  for (;;) {
    SMS_waitForVBlank();
    kp=SMS_getKeysPressed();
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2))
      break;
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1)) {
      color_selection=(color_selection+1) & 0x03;
      GG_setNextBGColoratIndex(0);
      switch (color_selection) {
        case 0:
          for (i=0;i<16;i++)
            GG_setColor(RGB(i,i,i));
          break;
        case 1:
          for (i=0;i<16;i++)
            GG_setColor(RGB(i,0,0));
          break;
        case 2:
          for (i=0;i<16;i++)
            GG_setColor(RGB(0,i,0));
          break;
        default:
          for (i=0;i<16;i++)
            GG_setColor(RGB(0,0,i));
          break;
      }
    }
  }
  SMS_displayOff();
}

void drop_shadow_striped_sprite (bool striped) {
  unsigned char frame=0,pos=0;

  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  UNSAFE_SMS_loadZX7compressedTiles (AlexKidd__tiles__zx7,0);
  SMS_loadSTMcompressedTileMap (0,0,AlexKidd__tilemap__stmcompr);
  GG_loadBGPalette(AlexKidd__palette__bin);
  if (striped) {
    SMS_loadPSGaidencompressedTiles (striped__tiles__psgcompr,256);
  } else {
    SMS_loadPSGaidencompressedTiles (drop__tiles__psgcompr,256);
  }
  GG_setSpritePaletteColor(0,0);
  GG_setSpritePaletteColor(1,0);
  SMS_setSpriteMode(SPRITEMODE_TALL);
  SMS_displayOn();
  for (;;) {
    SMS_initSprites();
    ks=SMS_getKeysStatus();

    if (striped) {
      SMS_addTwoAdjoiningSprites (110+pos,   80+pos,0);
      SMS_addTwoAdjoiningSprites (110+16+pos,80+pos,4);
      SMS_addTwoAdjoiningSprites (110+pos,   80+16+pos,8);
      SMS_addTwoAdjoiningSprites (110+16+pos,80+16+pos,12);

      if (ks & (PORT_A_KEY_UP|PORT_A_KEY_LEFT|PORT_B_KEY_UP|PORT_B_KEY_LEFT))
        pos--;
      else if (ks & (PORT_A_KEY_DOWN|PORT_A_KEY_RIGHT|PORT_B_KEY_DOWN|PORT_B_KEY_RIGHT))
        pos++;

    } else {
      SMS_addTwoAdjoiningSprites (32+pos,(64*frame)+pos,0);
      SMS_addTwoAdjoiningSprites (32+16+pos,(64*frame)+pos,4);
      SMS_addTwoAdjoiningSprites (32+pos,(64*frame)+pos+16,8);
      SMS_addTwoAdjoiningSprites (32+16+pos,(64*frame)+pos+16,12);

      SMS_addTwoAdjoiningSprites (96+pos,48+(64*frame)+pos,0);
      SMS_addTwoAdjoiningSprites (96+16+pos,48+(64*frame)+pos,4);
      SMS_addTwoAdjoiningSprites (96+pos,48+(64*frame)+pos+16,8);
      SMS_addTwoAdjoiningSprites (96+16+pos,48+(64*frame)+pos+16,12);

      SMS_addTwoAdjoiningSprites (160+pos,96+(64*frame)+pos,0);
      SMS_addTwoAdjoiningSprites (160+16+pos,96+(64*frame)+pos,4);
      SMS_addTwoAdjoiningSprites (160+pos,96+(64*frame)+pos+16,8);
      SMS_addTwoAdjoiningSprites (160+16+pos,96+(64*frame)+pos+16,12);

      pos++;
      frame=1-frame;
    }

    SMS_waitForVBlank();
    SMS_copySpritestoSAT();

    kp=SMS_getKeysPressed();
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2|PORT_A_KEY_1|PORT_B_KEY_1))
      break;
  }
  SMS_displayOff();
  SMS_setSpriteMode(SPRITEMODE_NORMAL);
}

void fullscreen (void) {
  unsigned char which=0;
  SMS_displayOff();
  SMS_VRAMmemset (0x0000, 0x00, 32);               // a 'empty' tile
  SMS_VRAMmemset (XYtoADDR(0,0), 0x00, 32*28*2);  // full map of 'empty' tiles
  GG_setBGPaletteColor (0, RGB(15,15,15));
  SMS_displayOn();
  for (;;) {
    SMS_initSprites();
    SMS_waitForVBlank();
    SMS_copySpritestoSAT();
    kp=SMS_getKeysPressed();
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2))
      break;
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1)) {
      which=(which+1)%4;
      switch (which) {
        case 0:GG_setBGPaletteColor (0, RGB(15,15,15)); break;
        case 1:GG_setBGPaletteColor (0, RGB(15,0,0)); break;
        case 2:GG_setBGPaletteColor (0, 0x0f0); break;            // RGB(0,15,0) confuses SDCC (!!!)
        case 3:GG_setBGPaletteColor (0, RGB(0,0,15)); break;
      }
    }
  }
  SMS_displayOff();
}

void video_tests (void) {
  bool go_back=false;
  draw_menu(video_menu, VIDEO_MENU_ITEMS);
  draw_footer_and_ver();
  cur_menu_item=0,pointer_anim=0;
  while (!go_back) {
    SMS_initSprites();
    SMS_addSprite(MENU_FIRST_COL*8-16+(pointer_anim/8),(MENU_FIRST_ROW+cur_menu_item)*8,0);
    SMS_waitForVBlank();
    SMS_copySpritestoSAT();

    if ((++pointer_anim)==7*8)
      pointer_anim=0;

    kp=SMS_getKeysPressed();

    if (kp & (PORT_A_KEY_UP|PORT_B_KEY_UP)) {        // UP
      if (cur_menu_item>0)
        cur_menu_item--;
      else
        cur_menu_item=VIDEO_MENU_ITEMS-1;
      pointer_anim=0;
    }
    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN)) {    // DOWN
      if (cur_menu_item<(VIDEO_MENU_ITEMS-1))
        cur_menu_item++;
      else
        cur_menu_item=0;
      pointer_anim=0;
    }
    if (kp & (PORT_A_KEY_1|PORT_A_KEY_2|PORT_B_KEY_1|PORT_B_KEY_2)) {
      switch (cur_menu_item) {
        case 0:static_screen(PLUGE__tiles__psgcompr,PLUGE__tilemap__stmcompr,PLUGE__palette__bin,NULL); break;
        case 1:color_cycle(color_bars__tiles__psgcompr,color_bars__tilemap__stmcompr); break;
        case 2:static_screen(color_bleed__tiles__psgcompr,color_bleed__tilemap__stmcompr,color_bleed__palette__bin,color_bleed2__tiles__psgcompr); break;
        case 3:static_screen(grid__tiles__psgcompr,grid__tilemap__stmcompr,grid__palette__bin,NULL); break;
        case 4:static_screen(stripes__tiles__psgcompr,fullscreen__tilemap__stmcompr,bw_palette_bin,checkerboard__tiles__psgcompr); break;
        case 5:fullscreen(); break;
        case 6:static_screen(linearity__tiles__psgcompr,linearity__tilemap__stmcompr,linearity__palette__bin,NULL); break;
        case 7:drop_shadow_striped_sprite(false);break;
        case 8:drop_shadow_striped_sprite(true);break;
        case VIDEO_MENU_ITEMS-1:go_back=true; break;
      }
      if (!go_back) {
        load_menu_assets();
        draw_menu(video_menu, VIDEO_MENU_ITEMS);
        draw_footer_and_ver();
      }
    }
  }
  cur_menu_item=0,pointer_anim=0;
}

void audio_test (void) {
  unsigned char stereo=0;

  SMS_initSprites();
  SMS_waitForVBlank();
  SMS_copySpritestoSAT();

  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW);
  printf ("key -> test    ");
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+1);
  printf ("<U> channel 0  ");
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+2);
  printf ("<R> channel 1  ");
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+3);
  printf ("<D> channel 2  ");
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+4);
  printf ("<L> noise chn. ");
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+5);
  printf ("<1> volume clip");
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+6);
  printf ("<2> to exit    ");

  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+9);
  printf ("change: <START>");

  for(;;) {
    SMS_waitForVBlank();
    PSGFrame();
    kp=SMS_getKeysPressed();
    if (kp & (PORT_A_KEY_UP|PORT_B_KEY_UP))
      PSGPlay(CH0_psgc);
    if (kp & (PORT_A_KEY_RIGHT|PORT_B_KEY_RIGHT))
      PSGPlay(CH1_psgc);
    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN))
      PSGPlay(CH2_psgc);
    if (kp & (PORT_A_KEY_LEFT|PORT_B_KEY_LEFT))
      PSGPlay(CH3_psgc);
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1))
      PSGPlay(VolumeTest_psgc);
    if (kp & GG_KEY_START) {
      stereo=(stereo<2)?(stereo+1):0;
      GGStereoPort=stereomode[stereo];
    }
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2))
      break;

    SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+8);
    switch (stereo) {
      case 0:
        printf ("[[STEREO MODE]]");
        break;
      case 1:
        printf ("LEFT ONLY <<<<<");
        break;
      default:
        printf (">>>> RIGHT ONLY");
        break;
    }
  }
  PSGStop();
  GGStereoPort=0xFF;  // restore port default
}

void pad_tests (void) {
  unsigned char i,stay_cnt=0;
  bool should_stay=true;

  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  UNSAFE_SMS_loadZX7compressedTiles (pads__tiles__zx7,0);
  SMS_loadSTMcompressedTileMap (0,0,pads__tilemap__stmcompr);
  GG_loadBGPalette(pads__palette__bin);

  // set all pads key to black
  GG_setBGPaletteColor(COLOR_UP,BLACK);
  GG_setBGPaletteColor(COLOR_DOWN,BLACK);
  GG_setBGPaletteColor(COLOR_LEFT,BLACK);
  GG_setBGPaletteColor(COLOR_RIGHT,BLACK);
  GG_setBGPaletteColor(COLOR_1,BLACK);
  GG_setBGPaletteColor(COLOR_2,BLACK);
  GG_setBGPaletteColor(COLOR_START,BLACK);

  SMS_displayOn();
  while(should_stay) {
    SMS_waitForVBlank();

    // read controller
    kp=SMS_getKeysPressed();
    kr=SMS_getKeysReleased();

    // press
    if (kp & (PORT_A_KEY_UP|PORT_B_KEY_UP))
      GG_setBGPaletteColor(COLOR_UP,HILIT_COLOR);
    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN))
      GG_setBGPaletteColor(COLOR_DOWN,HILIT_COLOR);
    if (kp & (PORT_A_KEY_LEFT|PORT_B_KEY_LEFT))
      GG_setBGPaletteColor(COLOR_LEFT,HILIT_COLOR);
    if (kp & (PORT_A_KEY_RIGHT|PORT_B_KEY_RIGHT))
      GG_setBGPaletteColor(COLOR_RIGHT,HILIT_COLOR);
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1))
      GG_setBGPaletteColor(COLOR_1,HILIT_COLOR);
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2))
      GG_setBGPaletteColor(COLOR_2,HILIT_COLOR);
    if (kp & GG_KEY_START)
      GG_setBGPaletteColor(COLOR_START,HILIT_COLOR);


    // release
    if (kr & (PORT_A_KEY_UP|PORT_B_KEY_UP))
      GG_setBGPaletteColor(COLOR_UP,BLACK);
    if (kr & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN))
      GG_setBGPaletteColor(COLOR_DOWN,BLACK);
    if (kr & (PORT_A_KEY_LEFT|PORT_B_KEY_LEFT))
      GG_setBGPaletteColor(COLOR_LEFT,BLACK);
    if (kr & (PORT_A_KEY_RIGHT|PORT_B_KEY_RIGHT))
      GG_setBGPaletteColor(COLOR_RIGHT,BLACK);
    if (kr & (PORT_A_KEY_1|PORT_B_KEY_1))
      GG_setBGPaletteColor(COLOR_1,BLACK);
    if (kr & (PORT_A_KEY_2|PORT_B_KEY_2))
      GG_setBGPaletteColor(COLOR_2,BLACK);
    if (kr & GG_KEY_START)
      GG_setBGPaletteColor(COLOR_START,BLACK);

    // if there are no keys pressed or released or held in 3 seconds, leave the test
    if ((kp | kr | SMS_getKeysHeld())==0) {
      if (++stay_cnt>=PAD_TIMEOUT)
        should_stay=false;
    } else
      stay_cnt=0;
  }
}

void prepare_and_show_main_menu (void) {
  SMS_displayOff();
  load_menu_assets();
  draw_footer_and_ver();
  draw_menu(main_menu, main_menu_items);
}

void sysinfo (void) {
  unsigned int bios_sum;
  unsigned char i=0;

  SMS_initSprites();
  SMS_copySpritestoSAT();

  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW);
  printf ("Hardware tests");
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+1);
  printf ("--------------");
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+2);
  printf ("Z80 type? %-4s",(CMOS_CPU?"CMOS":"NMOS"));
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+3);
  if (has_BIOS_GG) {
    bios_sum=get_BIOS_sum(BIOS_SIZE_1K);
    printf ("BIOS sum:$%04X",bios_sum);
    SMS_setNextTileatXY(7,MENU_FIRST_ROW+5);
    while (i<GG_BIOSES_ITEMS) {
      if (bios_sum==GG_BIOSes[i].sum1k) {
        printf ("%s",GG_BIOSes[i].name);
        break;
      }
      i++;
    }
    if (i==GG_BIOSES_ITEMS)
      printf ("unidentified BIOS!");
  } else {
    printf ("No BIOS       ");
  }

  for (;;) {
    SMS_waitForVBlank();
    kp=SMS_getKeysPressed();

    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN)) {
      SMS_enableSRAM();
      dump_BIOS();
      SMS_disableSRAM();
    }
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2|PORT_A_KEY_1|PORT_B_KEY_1))
      break;
  }
}

bool is_cmos_CPU (void) __naked {
  __asm

    ld hl,#0x4000
    rst #0x08

    dec c
    .db 0xED, 0x71         ; out (c),#0

    ld hl,#0x0000
    rst #0x08

    dec c
    in a,(c)
    jr nz, cmos

    ld l,#0
    ret

cmos:
    ld l,#1
    ret
  __endasm;
}

void main (void) {
  // detect region
  is_Japanese=((GGRegionPort & 0x40)==0);

  // detect BIOS (if present)
  has_BIOS_GG=is_Port3E_effective(0xF7E0);   // OR 0xE0 (set bits 5,6,7 to disable card/cartridge/expansion), AND 0xF7 (enable BIOS)

  // detect Z80 type
  CMOS_CPU=is_cmos_CPU();

  main_menu_items=MAIN_MENU_ITEMS;

  // restore standard operation modes
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_useFirstHalfTilesforSprites(false);
  SMS_setSpriteMode (SPRITEMODE_NORMAL);

  // prepare and show main menu
  prepare_and_show_main_menu();

  for (;;) {
    SMS_initSprites();
    SMS_addSprite(MENU_FIRST_COL*8-16+(pointer_anim/8),(MENU_FIRST_ROW+cur_menu_item)*8,0);
    SMS_waitForVBlank();
    SMS_copySpritestoSAT();

    if ((++pointer_anim)==7*8)
      pointer_anim=0;

    kp=SMS_getKeysPressed();

    if (kp & (PORT_A_KEY_UP|PORT_B_KEY_UP)) {        // UP
      if (cur_menu_item>0)
        cur_menu_item--;
      else
        cur_menu_item=main_menu_items-1;
      pointer_anim=0;
    }
    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN)) {    // DOWN
      if (cur_menu_item<(main_menu_items-1))
        cur_menu_item++;
      else
        cur_menu_item=0;
      pointer_anim=0;
    }
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1)) {
      switch (cur_menu_item) {
        case 0:video_tests(); break;
        case 1:audio_test(); break;
        case 2:pad_tests(); break;
        case 3:sysinfo(); break;
      }
      prepare_and_show_main_menu();
    }
  }
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'

SMS_EMBED_SDSC_HEADER_AUTO_DATE(MAJOR_VER,MINOR_VER, "sverx", "SEGA Game Gear TestSuite",
  "Built using devkitSMS/SMSlib\n[https://github.com/sverx/devkitSMS]");

/* NOTE: coding started on 09-09-2021 forking SMS Test Suite 0.32 */
