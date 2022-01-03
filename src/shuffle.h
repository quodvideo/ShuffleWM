#ifndef SHUFFLE_H
#define SHUFFLE_H

#include <stdio.h>
#include <stdlib.h>

#define WIN(...)  fprintf (stderr, __VA_ARGS__), exit (EXIT_SUCCESS);
#define FAIL(...) fprintf (stderr, __VA_ARGS__), exit (EXIT_FAILURE);
#define LIMP(...) fprintf (stderr, __VA_ARGS__);
#define YO(...)   fprintf (stderr, __VA_ARGS__);

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

extern enum ShuffleModes shuffle_mode;

extern Time last_user_timestamp;
extern Time last_prop_timestamp;

extern struct managed_window *current_focus;

#endif /* SHUFFLE_H */
