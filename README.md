GG Test Suite
==============

*largely inspired by Artemio's 240p, but not a fork of it!*

## Functions:

### Video Tests
(video test descriptions mainly taken from http://junkerhq.net/xrgb/index.php?title=240p_test_suite)

* PLUGE
(press 1 or 2 to exit)
The picture line-up generation equipment (PLUGE) is a test pattern used to adjust the black level and contrast of a video signal.
The pattern in this suite is mainly designed to adjust the black level. The control for setting this on your GameGear is the contrast knob.

* Color bars
(press 1 to change color or 2 to exit)
This is a typical pattern for adjusting black, white, and color levels in general. Displaying all the "pure" colors possible on each system, from lowest to highest intensity.
This is adjusted in the same way black levels are set with the PLUGE. Adjust the white level first, using the contrast knob.

* Color bleed
(press 1 to change the vertical bars to a checker board and back, press 2 to exit)
This pattern helps diagnose unneeded color up-sampling. It consists of one pixel width color bars alternating with one pixel width black bars.

* Grid
(press 1 or 2 to exit)
It is used in order to align the screen and find out overscan on the display. It uses the full resolution of the target console, with a margin of red squares for the corners and an additional internal margin of yellow squares.

* Stripes/Checks
(press 1 to change the bars to a checker board and back, press 2 to exit)
A pattern consisting of a full screen of horizontal black and white stripes, one pixel tall each. This is a taxing pattern
Checker board then is an even more taxing pattern, a one pixel black and one pixel white alternating pattern. This pattern also makes obvious if all lines are being scaled equally.

* Full colors
(press 1 to cycle white/colors, press 2 to exit)
The screen is filled with solid color. The user can change the fill color cycling between white, red, green and blue.

* Linearity
(press 1 or 2 to exit)
Five circles are displayed, one in each corner and a big one in the middle. Used to check linearity of the display. The pattern is designed to work respecting the pixel aspect ratios. Circles should be perfectly round in the display.

* Drop shadow
(press 1 or 2 to exit)
It displays a simple sprite shadow against a background, but the shadow is shown only on each other frame. This achieves a flicker/transparency effect, since you are watching a 25Hz/30Hz shadow on a 50Hz/60Hz signal. No background detail should be lost and the shadow should be visible.

* Striped sprite
(press UP/LEFT or DOWN/RIGHT to move the sprite diagonally, press 1 or 2 to exit)
It displays a simple striped sprite against a background. It should be easy to tell if the screen tries to deinterlace/interpolate or not.

### Audio Tests

* press UP to play music on PSG (square wave) channel 0
* press RIGHT to play music on PSG (square wave) channel 1
* press DOWN to play music on PSG (square wave) channel 2
* press LEFT to play music on PSG noise channel (channel 3)
* press 1 to test volume clipping. If the sound volume appears to do not change it means it's clipping.
* press START to toggle stereo mode (headphones only!)
* press 2 to exit

### Pad Tests

You can test your SEGA Game Gear buttons, and SEGA Master System pad buttons too, if one is connected to the ext port. If no key is pressed/held/released in approx 3 seconds, the test will end.

### System Info

You can read some info about your system. In detail:

* The result of a detection routine that tests if the Z80 processor inside your console is a NMOS or CMOS one.
* The 16-bit checksum of the first 8 KB of the console BIOS contents. This is used to identify the BIOS in the machine, if present, and informations will be printed in the lower part of the screen.
If "unidentified BIOS found!" is printed, please let us know here -> http://www.smspower.org/forums/
Also, pressing the DOWN key on the pad, the first 16 KB of the BIOS contents will be saved to SRAM, if your cartridge supports that (Krikzz's EverDrive GGs users will find a SAV file in their SD cards only after dumping the SRAM to SD, which happens when you load a new ROM to the cart).

