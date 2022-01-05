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
#include "session.h"

#include "shuffle.h"

#include "decode.h"

/* Globals that change a lot. */
#warning "This code does not check for X server clock roll-over."
Time last_user_timestamp = CurrentTime;
Time last_prop_timestamp = CurrentTime;
unsigned long last_key_release_serial;

struct managed_window *current_focus = NULL;

/* This changes less. */
enum ShuffleModes shuffle_mode = NoMode;

/* All the basic set-up. */
void     process_command_line    (int argc, char **argv);  // does nothing
Display *connect_to_display      (const char *display_name);
Window   select_root             (Display *display, int screen_number);
void     set_up_environment      (Display *display, Window root);
void     manage_existing_windows (Display *display, Window root);

/* Event loop */
void update_timestamps (XEvent *e);
void dispatch_event    (XEvent *e, Window root);
void check_sanity      (Display *display, Window root);

/* Clean up when done. */
void release_managed_windows (Display *display, Window root);

/* Error handler function for XSetErrorHandler */
int on_error (Display *display, XErrorEvent *e);
int on_io_error (Display *display);

/* Here we go! */
int
main (int argc, char **argv)
{
  Display *d;
  Window   root;

  process_command_line (argc, argv);
  d = connect_to_display (NULL);
  XSetErrorHandler (on_error);
  XSetIOErrorHandler (on_io_error);
  intern_display_atoms (d);       // atoms.c
  root = select_root (d, DefaultScreen(d));
  // This could be event driven from about here on."
  acquire_wm_selection (d, root); // manager.c
  connect_to_session_manager ();  // session.c
  set_up_environment (d, root);
  manage_existing_windows (d, root);

  shuffle_mode = NoMode;
  while (shuffle_mode) {
    XEvent event;
    LIMP("===XNextEvent\n");
    XNextEvent (d, &event);
    if (event.type == KeyRelease) last_key_release_serial = event.xkey.serial;
//    if (event.xany.serial == last_key_release_serial) continue;
#ifdef DECODE_H
    LIMP("===decode_event\n");
    decode_event (&event);
#endif /* DECODE_H */
    LIMP("===update_timestamps\n");
    update_timestamps (&event);
    LIMP("===dispatch_event\n");
    dispatch_event (&event, root);
    LIMP("===check_sanity\n");
    check_sanity (d, root);
    LIMP("===iteration complete\n");
  }
  release_managed_windows (d, root);
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

Window
select_root (Display *display, int screen_number)
{
  extern Atom WM_Sn;
  char wm_sn[6];
  
  sprintf (wm_sn, "WM_S%d", screen_number);
  
  WM_Sn = XInternAtom (display, wm_sn, False);

  return RootWindow(display, screen_number);
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
set_up_environment (Display *display, Window root)
{
  LIMP("Taking control.\n");
  init_root (display, root);
  init_windows (display);
}

void
manage_existing_windows (Display *display, Window root)
{
  /* Doing this like TWM. Filter out the icons and the Overriders.
   * Then unmap and remap what remain. That way there's only one function
   * to process windows.
   *
   * The filter loop and remap loop could be merged at the risk of 
   * an icon window having WM_HINTS.
   */
  Window rootr, parent, *children;
  unsigned int nchildren;

  if (XQueryTree (display, root, &rootr, &parent, &children, &nchildren)) {
    for (int i=0;i<nchildren;i++) {
      if (children[i] != None) {
        XWindowAttributes wattr;
        XWMHints *hints;
        XGetWindowAttributes (display, children[i], &wattr);
        if (wattr.override_redirect || wattr.map_state == IsUnmapped) {
          children[i] = None;
        }
        if(children[i] && (hints = XGetWMHints (display, children[i]))) {
          if (hints->flags & IconWindowHint) {
            for (int j=0;j<nchildren;j++) {
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
    for (int i=0;i<nchildren;i++) {
      LIMP("Looking at child %d of %d, XID %lu\n", i+1, nchildren, children[i]);
      if (children[i] != None) {
        XMapRequestEvent fake;
        XUnmapWindow (display, children[i]);
        fake.type = MapRequest;
        fake.serial = 0;
        fake.send_event = True;
        fake.display = display;
        fake.parent = root;
        fake.window = children[i];
        LIMP("Sending proxied MapRequest.\n");
        XSendEvent (display, root, False,
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
dispatch_event (XEvent *e, Window root)
{
  Display *d = e->xany.display;
  Window w = e->xany.window;
  struct managed_window *mw;
  struct icon *icon;

  if (w == root) {
    on_root_event (e);                          // root.c
  } else if (w == get_manager_window(d, root)) {
    on_manager_event (e, root);                 // manager.c
  } else if ((mw = find_window (d, w))) {
    on_window_event (mw, e);                    // window.c
  } else if ((icon = get_icon(d,w))) {
    on_icon_event (icon, e);                    // icon.c
  } else {
    /* should not happen */
  }
}

void
check_sanity (Display *display, Window root)
{
  Window current_focus;
  int    revert_to;
  Window rootr, parent, *children;
  unsigned int nchildren;

  XGetInputFocus (display, &current_focus, &revert_to);
  LIMP("\nFOCUS IS %ld %d\n\n",current_focus, revert_to);

  if (current_focus==1) {
    /* Looks like focus was lost. Set it to the top window. */
    focus_top (display, root, last_prop_timestamp>last_user_timestamp
                              ?last_prop_timestamp
                              :last_user_timestamp);
  } else if (XQueryTree (display, root,
                         &rootr, &parent, &children, &nchildren)) {  
    /* Check the stacking of the windows.
     * If the focused window isn't on top and and if there isn't a
     * ConfigureNotify or ConfigureRequest pending, then
     * raise the focused window or its transients to the top of the stack.
     * 
     * It's expected that clients will generally handle raising their own
     * windows.
     */
    for (int i=nchildren-1;i>=0; i--) {
      struct managed_window *mw;
      if ((mw = find_window (display, children[i]))) {
        if (children[i] != current_focus) {
          /* There's a managed window near the top, but it's not focused. */
          XEvent e;
          if (XCheckWindowEvent (display, children[i],
                                 StructureNotifyMask
                                 | SubstructureNotifyMask
                                 | SubstructureRedirectMask ,
                                 &e)) {
            break;
          } else {
            /* carry on? */
          }
        }
      }
    }
    if (nchildren) XFree (children);
  }
  LIMP("Sanity check complete.\n");
  /* Eventually this will keep the desktop below other windows and the
   * menu bar above other windows.
   */
}

void
release_managed_windows (Display *display, Window root)
{
  Window rootr, parent, *children;
  unsigned int nchildren;

  XSelectInput (display, root, None);

  if (XQueryTree (display, root, &rootr, &parent, &children, &nchildren)) {
    for (int i=0;i<nchildren;i++) {
      remove_window (display, children[i]);
    }
    if (nchildren) XFree (children);
  }
  XDestroyWindow (display, get_manager_window(display, root));
  WIN("I surrender!\n");
}

int
on_error (Display *display, XErrorEvent *e)
{
  char buf[256];
  XGetErrorText (display, e->error_code, buf, 256);
  LIMP("X Error %s\n", buf);
  return 0;
}

int
on_io_error (Display *display)
{
  FAIL("!!! ABSOLUTE CHAOS !!!\n");
}
