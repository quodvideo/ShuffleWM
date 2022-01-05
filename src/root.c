#include <stdlib.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#include "shuffle.h"
#include "window.h"
#include "root.h"
#include "atoms.h"
#include "moveresize.h"

static void set_wm_icon_size   (Display *d, Window root);
#warning "This code makes assumptions about the modifier mapping."
static void grab_wm_keys       (Display *d, Window root);
static void grab_focus_buttons (Display *d, Window root);
static void grab_wm_buttons    (Display *d, Window root);

static void on_key_press         (XKeyEvent *e);
static void on_key_release       (XKeyEvent *e);
static void on_button_press      (XButtonEvent *e);
static void on_button_release    (XButtonEvent *e);
static void on_motion_notify     (XMotionEvent *e);
static void on_focus_in          (XFocusChangeEvent *e);
static void on_focus_out         (XFocusChangeEvent *e);
static void on_map_request       (XMapRequestEvent *e);
static void on_configure_request (XConfigureRequestEvent *e);
static void on_circulate_request (XCirculateRequestEvent *e);
static void on_property_notify   (XPropertyEvent *e);
static void on_client_message    (XClientMessageEvent *e);

void
init_root (Display *d, Window root)
{
  static Bool initialized = False;
  if (!initialized) {
    Cursor root_cursor = None;
    root_cursor = XCreateFontCursor (d, XC_left_ptr);
    XDefineCursor (d, root, root_cursor);
    XSelectInput (d, root, SubstructureRedirectMask | FocusChangeMask);
    grab_wm_keys (d, root);
    grab_focus_buttons (d, root);
    grab_wm_buttons (d, root);
    set_wm_icon_size (d, root);
    /* Set up the managed_window while we know the root XID. */
    (void) get_root_mw (d, root);
    initialized = True;
  }
}

struct managed_window *
get_root_mw (Display *d, Window root)
{
  static struct managed_window *mw = NULL;
  if (mw == NULL) {
    mw = add_root_window (d, root);
  }
  return mw;
}

void
on_root_event (XEvent *e)
{
  switch (e->type) {
  case KeyPress:         on_key_press ((XKeyEvent *) e);          break;
  case KeyRelease:       on_key_release ((XKeyEvent *)e);         break;
  case ButtonPress:      on_button_press ((XButtonEvent *) e);    break;
  case ButtonRelease:    on_button_release ((XButtonEvent *) e);  break;
  case MotionNotify:     on_motion_notify ((XMotionEvent *) e);   break;
  case FocusIn:          on_focus_in ((XFocusChangeEvent *) e);   break;
  case FocusOut:         on_focus_out ((XFocusChangeEvent *) e);  break;
  case MapRequest:       on_map_request ((XMapRequestEvent *) e); break;
  case ConfigureRequest: on_configure_request ((XConfigureRequestEvent *) e); break;
  case CirculateRequest: on_circulate_request ((XCirculateRequestEvent *) e); break;
  case PropertyNotify:   on_property_notify ((XPropertyEvent *) e); break;
  case ClientMessage:    on_client_message ((XClientMessageEvent *) e); break;
  default: LIMP("Unselected event received on root window.\n"); break;
  }
}

static void
set_wm_icon_size (Display *d, Window root)
{
  // this is wrong
  XIconSize iconsizes[8] = {
    { 150, 150, 150, 150, 0, 0 }, /* 100% */
    { 310, 150, 310, 150, 0, 0 },

    { 120, 120, 120, 120, 0, 0 }, /*  80% */
    { 248, 120, 248, 120, 0, 0 },

    { 210, 210, 210, 210, 0, 0 }, /* 140% */
    { 434, 210, 434, 210, 0, 0 },

    { 270, 270, 270, 270, 0, 0 }, /* 180% */
    { 558, 270, 558, 270, 0, 0 },
  };
  LIMP("Setting icon sizes\n");
  XSetIconSizes (d, root, iconsizes, 8);
}


static void
grab_wm_keys (Display *d, Window root)
{
  /* Only grab Super_L. */
  XGrabKey (d, XKeysymToKeycode (d, XK_Super_L), AnyModifier, root,
            False, GrabModeAsync, GrabModeAsync);
}

static void
grab_focus_buttons (Display *d, Window root)
{
  XGrabButton (d, Button1, AnyModifier, root, False, ButtonPressMask,
               GrabModeSync, GrabModeSync, None, None);
  XGrabButton (d, Button2, AnyModifier, root, False, ButtonPressMask,
               GrabModeSync, GrabModeSync, None, None);
  XGrabButton (d, Button3, AnyModifier, root, False, ButtonPressMask,
               GrabModeSync, GrabModeSync, None, None);
  /* The remaining buttons are for scrolling and such. */
}

static void
grab_wm_buttons (Display *d, Window root)
{
  /* For now, only:
   * Super+Button1: Move window.
   * Super+Button2: Size window.
   */

  int scroll_lock_mask = 0;

  int ignored_masks[8] = { // Since scroll_lock_mask is 0, redundancies
    0,
    LockMask,
    Mod2Mask,
    scroll_lock_mask,
    LockMask|Mod2Mask,
    LockMask|scroll_lock_mask,
    LockMask|Mod2Mask|scroll_lock_mask,
    Mod2Mask|scroll_lock_mask,
  };
  int event_mask = ( ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);

  Cursor move_cursor = XCreateFontCursor(d, XC_fleur);
  Cursor size_cursor = XCreateFontCursor(d, XC_sizing);
  
  LIMP("Grabbing Buttons 1 & 3 with Mod4Mask\n");
  for (int i=0;i<8;i++) {
    XGrabButton (d, Button1, Mod4Mask|ignored_masks[i], root, False, event_mask,
                 GrabModeAsync, GrabModeAsync, None, move_cursor);
    XGrabButton (d, Button3, Mod4Mask|ignored_masks[i], root, False, event_mask,
                 GrabModeAsync, GrabModeAsync, None, size_cursor);
  }
}

void
focus_top (Display *d, Window root, Time t)
{
  Window  rootr;
  Window  parent;
  Window *children;
  unsigned int nchildren = 0;
  struct managed_window *mw = NULL;

  XQueryTree (d, root, &rootr, &parent, &children, &nchildren);

  for (int i=nchildren-1;i>=0;i--) {
    if ((mw = find_window (d, children[i]))) {
      focus_from_wm (mw, t);
      break;
    }
  }
  if (nchildren) XFree (children);
}

static void
on_key_press (XKeyEvent *e)
{
  switch (XLookupKeysym (e, 0)) {
  case XK_Super_L:
    break;
  case XK_Escape:
    switch (shuffle_mode) {
    case MovingWindow: cancel_move (e); break;
    case ResizingWindow: cancel_resize (e); break;
    default: break;
    }
    if (e->state & Mod4Mask) {
      /* Don't release the keyboard yet. */
    } else {
      XUngrabKeyboard (e->display, e->time);
    }
    break;
  case XK_Tab:
    if (e->state & Mod4Mask) {
      if (shuffle_mode == NoMode) {
        // shuffle_mode = SwitchingWindows;
        // Do something to enable switching windows
      }
      if (e->state & ShiftMask) {
        LIMP("Got a Super+Shift+Tab\n");
        // prev_link (e->display, e->root, e->time);
      } else {
        LIMP("Got a Super+Tab\n");
        // next_link (e->display, e->root, e->time);
      }
    }
    break;
    default:
      break;
  }
}

static void
on_key_release (XKeyEvent *e)
{
  switch (XLookupKeysym (e, 0)) {
  case XK_Super_L:
    switch(shuffle_mode) {
    case MovingWindow: break;
    case ResizingWindow: break;
    default: XUngrabKeyboard (e->display, e->time); break;
    }
  default: break;
  }
}

static void
on_button_press (XButtonEvent *e)
{
  struct managed_window *mw;
  if ((mw = find_window (e->display, e->subwindow))) {
    if (e->state & Mod4Mask) {
      if (shuffle_mode == NoMode) {
        switch (e->button) {
        case Button1: begin_move (e); break;
        case Button2: /* Iconify? */ break;
        case Button3: begin_resize (e); break;
        default: LIMP("Ignoring button>3.");
        }
      }
    } else {
      LIMP("Passing the click on.\n");
      try_focus_from_client_area (mw, e->time);
      XAllowEvents (e->display, ReplayPointer, e->time);
      XUngrabPointer (e->display, e->time);
    }
  } else {
    LIMP("releasing from root grab needed?");
    XAllowEvents (e->display, ReplayPointer, e->time);
    XUngrabPointer (e->display, e->time);
  }
}

static void
on_button_release (XButtonEvent *e)
{
  switch (shuffle_mode) {
  case MovingWindow:   finish_move (e);   break;
  case ResizingWindow: finish_resize (e); break;
  default: break;
  }
  if (e->state & Mod4Mask) {
    /* Don't release the keyboard yet. */
  } else {
    XUngrabKeyboard (e->display, e->time);
  }
}

static void
on_motion_notify (XMotionEvent *e)
{
  switch (shuffle_mode) {
  case MovingWindow: do_move (e); break;
  case ResizingWindow: do_resize (e); break;
  default: break;
  }
}

static void
on_focus_in (XFocusChangeEvent *e)
{
  if (e->mode == NotifyNormal || e->mode == NotifyWhileGrabbed) {
    Time t;
    LIMP ("Focus fell to root.\n"            \
          "\t    Last user timestamp: %lu\n" \
          "\tLast property timestamp: %lu\n",
          last_user_timestamp,
          last_prop_timestamp);
    t = (last_user_timestamp>last_prop_timestamp)
      ?last_user_timestamp
      :last_prop_timestamp;
    focus_top (e->display, e->window, t);
  }
}

static void
on_focus_out (XFocusChangeEvent *e)
{
}

static void
on_map_request (XMapRequestEvent *e)
{
  Display *d = e->display;
  Window w = e->window;
  struct managed_window *mw = NULL;

  if (!(mw = find_window (d, w))) {
    LIMP("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"
         "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
    mw = add_window (d, w);
    /* Let's skip this for now.*/
    // XLowerWindow (d, w);
    LIMP("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
         "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
  }
  if (mw) {
    switch (get_state(mw)) {
    case WithdrawnState:
      switch (get_initial_state(mw)) {
      case NormalState: map_window (mw); break;
      case IconicState: map_icon (mw); break;
      default:
        LIMP("Fell down a rabbit hole looking for initial_state.\n");
        LIMP("Treating as NormalState.\n");
        map_window (mw);
        break;
      }
      break;
    case NormalState: /* This should not happen. */
      LIMP("MapRequest from window in Normal state.");
      break;
    case IconicState: map_window (mw); break;
    default:
      LIMP("Fell down a rabbit hole looking for wm_state_state.");
      break;
    }
  } else {
    LIMP("MapRequest #%lu did not produce a managed window.", e->serial);
  }
}

static void
on_configure_request (XConfigureRequestEvent *e)
{
  XWindowChanges changes;
  unsigned mask = 0;
  struct managed_window *mw = NULL;
  if ((mw=find_window (e->display, e->window))) {
    LIMP("Configure request for the known window: %lu.\n", e->window);
  } else {
    LIMP("Configure request for an unknown window: %lu.\n", e->window);
  }
  changes.x = e->x;
  changes.y = e->y;
  changes.width = e->width;
  changes.height = e->height;

  /* Probably shouldn't do this for unmanaged windows, but there won't be
   * another chance like this.
   */
  if (e->value_mask & CWBorderWidth && e->border_width != 0) {
    LIMP("Changing border_width.\n");
  }
  changes.border_width = 0;
  mask |= CWBorderWidth;

  changes.sibling = e->above;
  changes.stack_mode = e->detail;
  mask |= e->value_mask;
  XConfigureWindow (e->display, e->window, mask, &changes);
}

static void
on_circulate_request (XCirculateRequestEvent *e)
{
}

static void
on_property_notify (XPropertyEvent *e)
{
}

static void
on_client_message (XClientMessageEvent *e)
{
  if (e->message_type == WM_CHANGE_STATE
      && e->format == 32
      && e->data.l[0] == IconicState) {
    iconify (find_window (e->display, e->window));
  }
}

static struct managed_window *focus_ring[512];
static int ring_focus = 0;
static int ring_end = 0;

static void
begin_keyboard_focus_change (XKeyEvent *e)
{
  // Get the list of windows to include in the ring
  // grab whatever needs to be grabbed for the mode
  // set mode to SwitchingWindows
  Window root, parent, *children;
  unsigned int nchildren;
  struct managed_window *mw;
  XQueryTree (e->display, e->root, &root, &parent, &children, &nchildren);

  ring_focus = 0;
  ring_end = 0;

  for (int i=nchildren;i>=0;i--) {
    mw = find_window (e->display, children[i]);
    if (mw) {
      focus_ring[ring_end] = mw;
      ring_end++;
    }
  }
  if (nchildren) {
    XFree (children);
  }
  LIMP("Found %d managed windows\n", ring_end);
  /* focus is presumably on the topmost window */
  ring_focus = ring_end - 1;
}

static void
advance_keyboard_focus (XKeyEvent *e)
{
  if (ring_focus==0) {
    ring_focus=ring_end-1;
  } else {
    --ring_focus;
  }
  focus_from_wm (focus_ring[ring_focus], e->time);
}

static void
reverse_keyboard_focus (XKeyEvent *e)
{
  if (ring_focus==ring_end-1) {
    ring_focus=0;
  } else {
    ++ring_focus;
  }
  focus_from_wm (focus_ring[ring_focus], e->time);
}

