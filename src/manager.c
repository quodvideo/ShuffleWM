#include <X11/Xlib.h>
#include "shuffle.h"
#include "atoms.h"
#include "targets.h"
#include "manager.h"

Bool confirm_wm_change          (void);
void wait_for_destruction       (Display *d, Window old_wm_window);
void try_forced_destruction     (Display *d, Window old_wm_window);
Bool confirm_forced_destruction (void);

static Bool match_timestamp_property (Display *d, XEvent *e, XPointer p);
static void on_property_notify   (XPropertyEvent         *e, Window root);
static void on_selection_clear   (XSelectionClearEvent   *e, Window root);
static void on_selection_request (XSelectionRequestEvent *e, Window root);
static void on_client_message    (XClientMessageEvent    *e, Window root);

void
acquire_wm_selection (Display *d, Window root)
{
  Window old_wm_window = None;
  Window wm_sn_manager_window;
  Time   timestamp;

  old_wm_window = XGetSelectionOwner (d, WM_Sn);

  if (old_wm_window != None) {
    if (confirm_wm_change ()) {
      XSelectInput (d, old_wm_window, StructureNotifyMask);
    } else {
      WIN("To replace an existing window manager, use the command line option '--replace'. End program.\n");
    }
  }

  wm_sn_manager_window = get_manager_window (d, root);
  timestamp = get_manager_timestamp (d, root);

  if (timestamp > last_prop_timestamp) {
    last_prop_timestamp = timestamp;
  }
  if (old_wm_window != XGetSelectionOwner (d, WM_Sn)) {
    FAIL("Old window manager replaced during start-up. Aborting.\n");
  }
  XSetSelectionOwner (d, WM_Sn, wm_sn_manager_window, timestamp);
  if (old_wm_window != None) {
    wait_for_destruction (d, old_wm_window);
  }
  if (wm_sn_manager_window != XGetSelectionOwner (d, WM_Sn)) {
    FAIL("Failed to become window manager. Aborting.\n");
  }
  send_manager_message (d, root);
}

Window
get_manager_window (Display *d, Window root)
{
  static Window w = None;
  if (w == None) {
    w = XCreateWindow (d, root, 0, 0, 1, 1, 0,
                       CopyFromParent, CopyFromParent, CopyFromParent,
                       CWEventMask,
                       &((XSetWindowAttributes){
                           .event_mask = PropertyChangeMask }));
  }
  return w;
}

Time
get_manager_timestamp (Display *d, Window root)
{
  static Time timestamp = CurrentTime;
  if (timestamp == CurrentTime) {
    Window manager_window;
    XEvent e;
    manager_window = get_manager_window (d, root);
    XChangeProperty (d, manager_window, 1, 1, 8, PropModeAppend, NULL, 0);
    XIfEvent (d, &e, match_timestamp_property, (XPointer) manager_window);
    timestamp = e.xproperty.time;
  }
  return timestamp;
}

void
send_manager_message (Display *d, Window root)
{
  extern Atom WM_Sn;
  XClientMessageEvent cme;
  
  cme.type = ClientMessage;
  cme.window = root;
  cme.message_type = MANAGER;
  cme.format = 32;
  cme.data.l[0] = get_manager_timestamp (d, root);
  cme.data.l[1] = WM_Sn;
  cme.data.l[2] = get_manager_window (d, root);
  cme.data.l[3] = 0;
  cme.data.l[4] = 0;
  XSendEvent (d, root, False, StructureNotifyMask, (XEvent *) &cme);
}

void
on_manager_event (XEvent *e, Window root)
{
  switch (e->type) {
  case PropertyNotify:
    on_property_notify ((XPropertyEvent *) e, root);
    break;
  case SelectionClear:
    on_selection_clear ((XSelectionClearEvent *) e, root);
    break;
  case SelectionRequest:
    on_selection_request ((XSelectionRequestEvent*) e, root);
    break;
  case ClientMessage:
    on_client_message ((XClientMessageEvent *) e, root);
    break;
  default:
    LIMP("Unselected event received on selection manager window.\n");
    break;
  }
}

Bool
confirm_wm_change (void)
{
  Bool replace = True;
  return (replace);
}


void
wait_for_destruction (Display *d, Window old_wm_window)
{
  XEvent foo; // Xlib doesn't like NULL.
  Bool gone;
  int n = 10, s = 1; // Check 10 times, 1 sec apart.
  do {
    gone = XCheckTypedWindowEvent (d, old_wm_window, DestroyNotify, &foo);
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
    try_forced_destruction (d, old_wm_window);
  }
}

void
try_forced_destruction (Display *d, Window old_wm_window)
{
  XEvent foo; // Xlib doesn't like NULL
  if (confirm_forced_destruction ()) {
    YO("Forcing existing window manager release.");
    XKillClient (d, old_wm_window);
    while (!XCheckTypedWindowEvent (d, old_wm_window, DestroyNotify, &foo)) {
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

static Bool
match_timestamp_property (Display *d, XEvent *e, XPointer p)
{
  return (e->type == PropertyNotify 
          && e->xproperty.window == (Window) p
          && e->xproperty.atom == 1);
}

static void
on_property_notify (XPropertyEvent *e, Window root)
{
}

static void
on_selection_clear (XSelectionClearEvent *e, Window root)
{
  shuffle_mode = ShutDown;
}

static void
on_selection_request (XSelectionRequestEvent *e, Window root)
{
  XSelectionEvent se;
  se.type = SelectionNotify;
  se.requestor = e->requestor;
  se.selection = e->selection;
  se.target = e->target;
  se.property = (e->owner == get_manager_window (e->display, root)
                 && e->selection == WM_Sn
                 && (e->time >= get_manager_timestamp (e->display, root)
                     || e->time == CurrentTime)
                 && convert_selection (e))
    ?e->property
    :None;
  se.time = e->time;
  XSendEvent (e->display, se.requestor, False, 0L, (XEvent *) &se);
}

static void
on_client_message (XClientMessageEvent *e, Window root)
{
  LIMP("Received ClientMessage on selection manager window.\n");
}
