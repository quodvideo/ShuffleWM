#include <stdlib.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>

#include "shuffle.h"
#include "moveresize.h"

static struct window_in_move_resize {
  Window w;
  int original_window_x, original_window_y;
  int original_pointer_x, original_pointer_y;
  int original_window_width, original_window_height;
  XSizeHints hints;
} wimr;


void
begin_move (XButtonEvent *e)
{
  XWindowAttributes wattr;

  XGetWindowAttributes (e->display, e->subwindow, &wattr);

  shuffle_mode = MovingWindow;
  wimr.w = e->subwindow;
  wimr.original_window_x = wattr.x;
  wimr.original_window_y = wattr.y;
  wimr.original_pointer_x = e->x_root;
  wimr.original_pointer_y = e->y_root;
  
  XGrabKeyboard (e->display, e->window, True,
                 GrabModeAsync, GrabModeAsync, e->time);
  
  shuffle_mode = MovingWindow;
}

void
begin_resize (XButtonEvent *e)
{
  XWindowAttributes wattr;

  long supplied_hints;

  XGetWindowAttributes (e->display, e->subwindow, &wattr);
  XGetWMNormalHints (e->display, e->subwindow, &(wimr.hints), &supplied_hints);

  /* Basic Sanity */
  if (wimr.hints.min_width <= 0)    wimr.hints.min_width = 1;
  if (wimr.hints.min_height <= 0)   wimr.hints.min_height = 1;
  if (wimr.hints.max_width <= 0)    wimr.hints.max_width = INT_MAX;
  if (wimr.hints.max_height <= 0)   wimr.hints.max_height = INT_MAX;
  if (wimr.hints.width_inc <= 0)    wimr.hints.width_inc = 1;
  if (wimr.hints.height_inc <= 0)   wimr.hints.height_inc = 1;
  if (wimr.hints.min_aspect.y == 0) wimr.hints.min_aspect.y = 1;
  if (wimr.hints.max_aspect.y == 0) wimr.hints.max_aspect.y = 1;
  if (wimr.hints.base_width <= 0)   wimr.hints.base_width = 1;
  if (wimr.hints.base_height <= 0)  wimr.hints.base_height = 1;

  /* if base size isn't provided, use minimum size. */
  if (!(wimr.hints.flags & PBaseSize)) {
    wimr.hints.base_width  = wimr.hints.min_width;
    wimr.hints.base_height = wimr.hints.min_height;
  }
  /* if minimum size isn't provided, use base size. */
  if (!(wimr.hints.flags & PMinSize)) {
    wimr.hints.min_width  = wimr.hints.base_width;
    wimr.hints.min_height = wimr.hints.base_height;
  }

  shuffle_mode = ResizingWindow;

  wimr.w = e->subwindow;
  wimr.original_window_x = wattr.x;
  wimr.original_window_y = wattr.y;
  wimr.original_pointer_x = e->x_root;
  wimr.original_pointer_y = e->y_root;
  wimr.original_window_width = wattr.width;
  wimr.original_window_height = wattr.height;
  LIMP("Resize original geometry: %dx%d+%d+%d\n",
       wimr.original_window_width,
       wimr.original_window_height,
       wimr.original_window_x,
       wimr.original_window_y);


  XGrabKeyboard (e->display, e->window, True,
                 GrabModeAsync, GrabModeAsync, e->time);

}

void
do_move (XMotionEvent *e)
{
  int delta_x = e->x_root - wimr.original_pointer_x;
  int delta_y = e->y_root - wimr.original_pointer_y;
  
  XMoveWindow (e->display, wimr.w,
               wimr.original_window_x + delta_x,
               wimr.original_window_y + delta_y);
  LIMP("Moving window %lu to %d %d\n",
       wimr.w,
       wimr.original_window_x + delta_x,
       wimr.original_window_x + delta_x);
}

void
do_resize (XMotionEvent *e)
{
  /* hints were sorted at the beginning of the resize */

  int delta_x = e->x_root - wimr.original_pointer_x;
  int delta_y = e->y_root - wimr.original_pointer_y;

  LIMP("First delta = (%d,%d)\n", delta_x, delta_y);

  /* Shave off the extra bit */
  delta_x -= delta_x % wimr.hints.width_inc;
  delta_y -= delta_y % wimr.hints.height_inc;

  LIMP("Second delta = (%d,%d)\n", delta_x, delta_y);

  int trial_width  = wimr.original_window_width + delta_x;
  int trial_height = wimr.original_window_height + delta_y;

  if (trial_width < wimr.hints.min_width) trial_width = wimr.hints.min_width;
  if (trial_width > wimr.hints.max_width) trial_width = wimr.hints.max_width;
  if (trial_height < wimr.hints.min_height) trial_height = wimr.hints.min_height;
  if (trial_height > wimr.hints.max_height) trial_height = wimr.hints.max_height;

  if (wimr.hints.flags & PAspect) {
    /* Make sure the aspect ratio is within bounds */
    float min_aspect = wimr.hints.min_aspect.x / wimr.hints.min_aspect.y;
    float max_aspect = wimr.hints.max_aspect.x / wimr.hints.max_aspect.y;
    float trial_aspect = 1.0;
    if (wimr.hints.flags & PBaseSize) {
      trial_aspect = (trial_width - wimr.hints.base_width) 
                      / (trial_height - wimr.hints.base_height);
    } else {
      trial_aspect = trial_width / trial_height;
    }
    if (min_aspect > max_aspect) {
      float tmp = min_aspect;
      min_aspect = max_aspect;
      max_aspect = tmp;
    }
    if (trial_aspect < min_aspect) {
      // do something
    }
    if (trial_aspect > max_aspect) {
      // do something
    }
  }
  LIMP("Resize original geometry: %dx%d+%d+%d\n",
       wimr.original_window_width,
       wimr.original_window_height,
       wimr.original_window_x,
       wimr.original_window_y);
  LIMP("Resize next geometry: %dx%d+%d+%d\n",
       trial_width,
       trial_height,
       wimr.original_window_x,
       wimr.original_window_y);

  XResizeWindow (e->display, wimr.w, trial_width, trial_height);
}

void
finish_move (XButtonEvent *e)
{
  shuffle_mode = NoMode;
}

void
finish_resize (XButtonEvent *e)
{
  shuffle_mode = NoMode;
}

void
cancel_move (XKeyEvent *e)
{
  shuffle_mode = NoMode;
  XMoveWindow (e->display, wimr.w,
               wimr.original_window_x,
               wimr.original_window_y);
}

void
cancel_resize (XKeyEvent *e)
{
  shuffle_mode = NoMode;
  XResizeWindow (e->display, wimr.w,
                 wimr.original_window_width,
                 wimr.original_window_height);
}

