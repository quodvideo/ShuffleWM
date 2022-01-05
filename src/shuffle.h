#ifndef SHUFFLE_H
#define SHUFFLE_H

#include <stdio.h>
#include <stdlib.h>

#define WIN(...)  fprintf (stderr, __VA_ARGS__), exit (EXIT_SUCCESS);
#define FAIL(...) fprintf (stderr, __VA_ARGS__), exit (EXIT_FAILURE);
#define LIMP(...) fprintf (stderr, __VA_ARGS__);
#define YO(...)   fprintf (stderr, __VA_ARGS__);

/*
 * One goal of Shuffle is complete ICCCM compliance. This may be facilitated in
 * part by taking what liberties are afforded window managers. If some part of
 * the ICCCM is deliberately ignored, it will be noted. Otherwise, it should be
 * regarded as an error. There are a lot of errors right now.
 *
 * Shuffle will eventually support some EWMH protocols, such as the stacking
 * order requirements.
 *
 * Whether shuffle will be compositing depends on whether anyone takes note of
 * it. Ideally, the better focus handling of shuffle will be noted soon and so
 * further development will be unnecessary. If I have to keep using shuffle to
 * get the right behavior from applications, then I'll keep developing it.
 */

enum ShuffleModes {
  ShutDown,
  NoMode,
  LockedScreen,
  ShowingDesktop,
  ShowingIcons,    /* or ShowingTiles? */
  ShowingFullscreen,
  SwitchingDesktops,
  SwitchingWindows,
  MovingWindow,
  ResizingWindow,
  AdjustingTiling, /* Need tabbing too? */
  /* Some things for consideration. */
  Annotate,        /* From Compiz. Draw on the screen and do stuff with that. */
  FiftyTwoPickup,  /* Seems like a fair name for something like Expose. */
  Gadgets          /* Just part of showing icons or tiles? */
};

/*
 * I may need to make shuffle_mode a bitfield instead,
 * to support some composite actions.
 */
extern enum ShuffleModes shuffle_mode;

extern Time last_user_timestamp;
extern Time last_prop_timestamp;

extern struct managed_window *current_focus;

#endif /* SHUFFLE_H */
