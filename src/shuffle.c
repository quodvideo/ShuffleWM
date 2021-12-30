#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>

#include "atoms.h"
#include "targets.h"
#include "window.h"
#include "manager.h"
#include "root.h"
#include "icon.h"

#include "shuffle.h"

#include "decode.h"

/* Globals that change a lot. */
#warning "This code does not check for X server clock roll-over."
Time last_user_timestamp = CurrentTime;
Time last_prop_timestamp = CurrentTime;

struct managed_window *current_focus = NULL;

/* This changes less. */
enum ShuffleModes shuffle_mode = NoMode;

/* Globals that almost never change. */
static Window wm_sn_root_window = None;
static Window wm_sn_manager_window = None;

/* All the basic set-up. */
void process_command_line       (int argc, char **argv);  // does nothing
Display *connect_to_display     (const char *display_name);
void select_screen              (Display *display, int screen_number);
void acquire_wm_selection       (Display *display);
void connect_to_session_manager (void);                   // does nothing
void set_up_environment         (Display *display);
void manage_existing_windows    (Display *display);
void update_timestamps          (XEvent *e);
void process_event              (XEvent *e);
void release_managed_windows    (Display *display);
Bool confirm_wm_change          (void);
void wait_for_destruction       (Display *display, Window old_wm_window);
void try_forced_destruction     (Display *display, Window old_wm_window);
Bool confirm_forced_destruction (void);

/* Here we go! */
int
main (int argc, char **argv)
{
  Display *d;
  process_command_line (argc, argv);
  d = connect_to_display (NULL);
  intern_display_atoms (d);
  select_screen (d, DefaultScreen(d));
#warning "This could be event driven from about here on."
  acquire_wm_selection (d);
  connect_to_session_manager ();
  set_up_environment (d);
  manage_existing_windows (d);

  shuffle_mode = NoMode;
  while (shuffle_mode) {
    XEvent event;
    XNextEvent (d, &event);
#ifdef DECODE_H
    decode_event (&event);
#endif /* DECODE_H */
    process_event (&event);
    if (1) {
      /* Temporary bit of code to put us in click to focus mode. */
      Window current_focus;
      int    revert_to;
      XGetInputFocus (d, &current_focus, &revert_to);
      LIMP("\nFOCUS IS %ld %d\n\n",current_focus, revert_to);
      if (current_focus==1) {
        focus_top (d, last_prop_timestamp>last_user_timestamp
                      ?last_prop_timestamp
                      :last_user_timestamp);
      }
    }
  }
  release_managed_windows (d);
  return 0;
}

void
process_command_line (int argc, char **argv)
{
}

Display *
connect_to_display (const char *display_name)
{
  return XOpenDisplay (display_name);
}

void
select_screen (Display *display, int screen_number)
{
  extern Atom WM_Sn;
  extern Window wm_sn_root_window;
  char wm_sn[6];
  
  sprintf (wm_sn, "WM_S%d", screen_number);
  
  WM_Sn = XInternAtom (display, wm_sn, False);
  wm_sn_root_window = RootWindow(display, screen_number);
  /* For nesting shuffle, consider basing this on the root window
   * (or pseudo-root, when nesting), instead of the screen number.
   * for (i = 0; i < ScreenCount (dpy); i++) {
   *   if (root == RootWindow (dpy, i)) {
   *     screen_number = i;
   *   }
   * }
   *
   */
}

void
acquire_wm_selection (Display *display)
{
  Window        old_wm_window = None;
  extern Window wm_sn_manager_window;
  Time          timestamp;

  old_wm_window = XGetSelectionOwner (display, WM_Sn);

  if (old_wm_window != None) {
    if (confirm_wm_change ()) {
      XSelectInput (display, old_wm_window, StructureNotifyMask);
    } else {
      WIN("To replace an existing window manager, use the command line option '--replace'. End program.\n");
    }
  }

  wm_sn_manager_window = get_manager_window (display, wm_sn_root_window);
  timestamp = get_manager_timestamp (display, wm_sn_root_window);

  if (timestamp > last_prop_timestamp) {
    last_prop_timestamp = timestamp;
  }
  if (old_wm_window != XGetSelectionOwner (display, WM_Sn)) {
    FAIL("Old window manager replaced during start-up. Aborting.\n");
  }
  XSetSelectionOwner (display, WM_Sn, wm_sn_manager_window, timestamp);
  if (old_wm_window != None) {
    wait_for_destruction (display, old_wm_window);
  }
  if (wm_sn_manager_window != XGetSelectionOwner (display, WM_Sn)) {
    FAIL("Failed to become window manager. Aborting.\n");
  }
  send_manager_message (display, wm_sn_root_window);
}

void
connect_to_session_manager (void)
{
}

void
set_up_environment (Display *display)
{
  LIMP("Taking control.\n");
  init_root (display, wm_sn_root_window);
  init_windows (display);
}

void
manage_existing_windows (Display *display)
{
  /* Doing this like TWM. Filter out the icons and the Overriders.
   * Then unmap and remap what remain. That way there's only one function
   * to process windows.
   *
   * The filter loop and remap loop could be merged at the risk of 
   * an icon window having WM_HINTS.
   */
  Window root, parent, *children;
  unsigned int nchildren;

  if (XQueryTree (display,wm_sn_root_window,&root,&parent,&children,&nchildren)) {
    int i;
    for (i=0;i<nchildren;i++) {
      if (children[i] != None) {
        XWindowAttributes wattr;
        XWMHints *hints;
        XGetWindowAttributes (display, children[i], &wattr);
        if (wattr.override_redirect || wattr.map_state == IsUnmapped) {
          children[i] = None;
        }
        if(children[i] && (hints = XGetWMHints (display, children[i]))) {
          if (hints->flags & IconWindowHint) {
            int j;
            for (j=0;j<nchildren;j++) {
              if (children[j] == hints->icon_window) {
                children[j] = None;
              }
            }
          }
          XFree (hints);
        }
      }
    }
    LIMP("Remapping windows.\n");
    for (i=0;i<nchildren;i++) {
      LIMP("Looking at child %d of %d, XID %lu\n", i+1, nchildren, children[i]);
      if (children[i] != None) {
        XMapRequestEvent fake;
        XUnmapWindow (display, children[i]);
        fake.type = MapRequest;
        fake.serial = 0;
        fake.send_event = True;
        fake.display = display;
        fake.parent = wm_sn_root_window;
        fake.window = children[i];
        LIMP("Sending proxied MapRequest.\n");
        XSendEvent (display, wm_sn_root_window, False,
                    SubstructureRedirectMask, (XEvent *) &fake);
      }
    }
    XFree (children);
  } else {
    FAIL("Could not acquire window tree. Aborting.");
  }
}

void
update_timestamps (XEvent *e)
{
  switch (e->type) {
  case KeyPress:
  case KeyRelease:
    if (e->xkey.time>last_user_timestamp) {
      last_user_timestamp = e->xkey.time;
    }
    break;
  case ButtonPress:
  case ButtonRelease:
    if (e->xbutton.time>last_user_timestamp) {
      last_user_timestamp = e->xbutton.time;
    }
    break;
  case PropertyNotify:
    if (e->xproperty.time>last_prop_timestamp) {
      last_prop_timestamp = e->xproperty.time;
    }
    break;
  }
}

void
process_event (XEvent *e)
{
  Display *d = e->xany.display;
  Window w = e->xany.window;
  struct managed_window *mw;
  struct icon *icon;
  
  update_timestamps (e);

  if (w == wm_sn_root_window) {
    on_root_event (e);                       // root.c
  } else if (w == wm_sn_manager_window) {
    on_manager_event (e, wm_sn_root_window); // manager.c
  } else if ((mw = find_window (d, w))) {
    on_window_event (e, mw);                 // window.c
  } else if ((icon = get_icon(d,w))) {
    on_icon_event (e, icon);                 // icon.c
  } else {
    /* should not happen */
  }
}

void
release_managed_windows (Display *display)
{
  Window root, parent, *children;
  unsigned int nchildren, i;

  XSelectInput (display, wm_sn_root_window, None);

  if (XQueryTree (display, wm_sn_root_window,
                  &root, &parent, &children, &nchildren)) {
    for (i=0;i<nchildren;i++) {
      remove_window (display, children[i]);
    }
    XFree (children);
  }
  XDestroyWindow (display, wm_sn_manager_window);
  WIN("I surrender!\n");
}

Bool
confirm_wm_change (void)
{
  Bool replace = True;
  return (replace);
}


void
wait_for_destruction (Display *display, Window old_wm_window)
{
  XEvent foo; // Xlib doesn't like NULL.
  Bool gone;
  int n = 10, s = 1; // Check 10 times, 1 sec apart.
  do {
    gone = XCheckTypedWindowEvent (display, old_wm_window, DestroyNotify, &foo);
    if (!gone) {
      if (n==10) {
        YO("Waiting for existing window manager to relinquish control.");
      } else {
        YO(" .");
      }
      sleep (s);
      n--;
    }
  } while (!gone && n!=0);
  if (n!=10) {
    YO("\n");
  }
  if (!gone) {
    try_forced_destruction (display, old_wm_window);
  }
}

void
try_forced_destruction (Display *display, Window old_wm_window)
{
  XEvent foo; // Xlib doesn't like NULL
  if (confirm_forced_destruction ()) {
    YO("Forcing existing window manager release.");
    XKillClient (display, old_wm_window);
    while (!XCheckTypedWindowEvent (display, old_wm_window, DestroyNotify, &foo)) {
      YO(" .");
      sleep (1);
    }
    YO(" Done.\n");
  } else {
    FAIL("Old window manager did not relinquish control. Aborting.\n");
  }
}

Bool
confirm_forced_destruction (void)
{
  Bool forced_replace = True;
  return (forced_replace);
}

