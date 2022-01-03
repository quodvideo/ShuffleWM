#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include "shuffle.h"
#include "atoms.h"
#include "window.h"

enum FocusModel {
  FocusNoInput,
  FocusPassive,
  FocusLocallyActive,
  FocusGloballyActive
};

struct managed_window {
  struct managed_window  *mw_transient_parent;
  struct managed_window **mw_transient_children;
  
  Display         *display;
  Window           id;
  int              wm_state_state;
  Window           wm_state_icon;
  enum FocusModel  focus_model;
  Bool             deletable;
  XRectangle       geometry;

  XWMHints         hints;
  XTextProperty    wm_name;
  XTextProperty    wm_icon_name;
  XClassHint      *class_hint;
  XSizeHints       normal_hints;
  long             supplied_normal_hints;
  Window           transient_parent;
  Window          *colormap_windows;
  int              n_colormap_windows;
  XTextProperty    client_machine;
};

static XContext toplevel_context;

static void store_property  (struct managed_window *mw, Atom prop);
static void remove_property (struct managed_window *mw, Atom prop);
static void set_wm_state (struct managed_window *mw,
                          unsigned long          state,
                          Window                 icon);
static void legacy_raise_on_focus (struct managed_window *mw);

static void on_focus_in          (struct managed_window *mw,
                                  XFocusChangeEvent     *e);
static void on_focus_out         (struct managed_window *mw,
                                  XFocusChangeEvent     *e);
static void on_destroy_notify    (struct managed_window *mw,
                                  XDestroyWindowEvent   *e);
static void on_unmap_notify      (struct managed_window *mw,
                                  XUnmapEvent           *e);
static void on_map_notify        (struct managed_window *mw,
                                  XMapEvent             *e);
static void on_reparent_notify   (struct managed_window *mw,
                                  XReparentEvent        *e);
static void on_configure_notify  (struct managed_window *mw,
                                  XConfigureEvent       *e);
static void on_gravity_notify    (struct managed_window *mw,
                                  XGravityEvent         *e);
static void on_circulate_notify  (struct managed_window *mw,
                                  XCirculateEvent       *e);
static void on_property_notify   (struct managed_window *mw,
                                  XPropertyEvent        *e);
static void on_client_message    (struct managed_window *mw,
                                  XClientMessageEvent   *e);
                                  

int
get_state (struct managed_window *mw)
{
  return mw->wm_state_state;
}

int
get_initial_state (struct managed_window *mw)
{
  return mw->hints.initial_state;
}

void
init_windows (Display *d)
{
  static Bool initialized = False;
  if (!initialized) {
    toplevel_context = XUniqueContext ();
    initialized = True;
  }
}

struct managed_window *
add_root_window (Display *d, Window w)
{
  struct managed_window *mw;
  mw = (struct managed_window *) malloc (sizeof(struct managed_window));
  mw->display = d;
  mw->id = w;
  mw->mw_transient_parent = NULL;
  mw->mw_transient_children = NULL;
  
  return mw;
}

struct managed_window *
add_window (Display *d, Window w)
{
  XWindowAttributes      wattr;
  struct managed_window *mw;
  Atom                  *props;
  int                    n_props;
  XGrabServer (d);
  XGetWindowAttributes (d, w, &wattr);
  if (wattr.override_redirect) {
    LIMP("Ignoring override redirect window %lu\n", w);
    return NULL; // We ignore menus, tooltips, etc.
  }
  LIMP("Adding window %lu\n", w);
  mw = (struct managed_window *) malloc (sizeof(struct managed_window));
  if (mw==NULL) {
    LIMP("Memory allocation failed.\n");
  } else {
    LIMP("Memory allocated.\n");
  }
  mw->display = d;
  mw->id = w;
  mw->wm_state_state = WithdrawnState;
  mw->wm_state_icon = None;
  /* It may be necessary to change this because some windows which should
   * be focusable, like `xev`, fail to set WM_HINTS.
   */
  mw->focus_model = FocusNoInput;
  mw->deletable = False;
  mw->geometry = (XRectangle){ wattr.x, wattr.y, wattr.width, wattr.height };

  mw->hints.flags = 0;
  mw->wm_name.value = NULL;
  mw->wm_icon_name.value = NULL;
  mw->class_hint = NULL;
  mw->normal_hints.flags = 0;
  mw->supplied_normal_hints = 0;
  mw->transient_parent = None;
  mw->colormap_windows = NULL;
  mw->n_colormap_windows = 0;
  mw->client_machine.value = NULL;
  LIMP("Saving window to XContext.\n");
  XSaveContext (d, w, toplevel_context, (XPointer) mw);
  LIMP("Adding window to SaveSet.\n");
  XAddToSaveSet (d, w);
  LIMP("Selecting input on window.\n");
  XSelectInput (d, w, ( FocusChangeMask
                        | PropertyChangeMask
                        | StructureNotifyMask ));
  LIMP("Getting property list.\n");
  props = XListProperties (d, w, &n_props);
  LIMP("Found %d properties.\n", n_props);
  for (int i=0;i<n_props;i++) {
    LIMP("Storing property %d of %d\n", i+1, n_props);
    store_property (mw, props[i]);
  }
  XFree (props);
  
  mw->mw_transient_parent = NULL;
  mw->mw_transient_children = NULL;

  XUngrabServer (d);

  return mw;
}

struct managed_window *
find_window (Display *d, Window w)
{
  struct managed_window *mw;
  if (XFindContext (d, w, toplevel_context, (XPointer *) &mw) == XCNOENT) {
    mw = NULL;
  }
  return mw;
}

void
remove_window (Display *d, Window w)
{
  struct managed_window *mw;
  if (!(mw=find_window (d, w))) {
    LIMP ("Window #%lu is not my window.\n", w);
    return;
  }
#warning "Make sure all managed_window allocations are freed."
  LIMP ("Removing window %lu\n", mw->id);
  // Is this necessary? Error prone?
  // XRemoveFromSaveSet (d, w);
  if (mw->wm_name.value != NULL) {
    LIMP ("\tWM_NAME = %s\n", mw->wm_name.value);
    XFree (mw->wm_name.value);
  }
  LIMP("Beep\n");
  if (mw->wm_icon_name.value != NULL) {
    LIMP ("\tWM_ICON_NAME = %s\n", mw->wm_icon_name.value);
    XFree (mw->wm_icon_name.value);
  }
  LIMP("Beep\n");
  if (mw->class_hint) {
    if (mw->class_hint->res_name != NULL) {
      LIMP ("\tWM_CLASS.name = %s\n",mw->class_hint->res_name);
      XFree (mw->class_hint->res_name);
    }
    LIMP("Beep\n");
    if (mw->class_hint->res_class != NULL) {
      LIMP ("\tWM_CLASS.clas = %s\n",mw->class_hint->res_class);
      XFree (mw->class_hint->res_class);
    }
  }
  LIMP("Beep\n");
  if (mw->colormap_windows != NULL) {
    LIMP ("\tColormap windows?\n");
    XFree (mw->colormap_windows);
  }
  LIMP("Beep\n");
  if (mw->client_machine.value != NULL) {
    LIMP ("\tWM_CLIENT_MACHINE = %s\n",mw->client_machine.value);
    XFree (mw->client_machine.value);
  }
  LIMP("Properties freed. Freeing struct managed_window.\n");
  free (mw);
  LIMP("Removing window %lu from toplevel_context map.\n", w);
  XDeleteContext (d, w, toplevel_context);
}

void
map_window (struct managed_window *mw)
{
  XMapWindow (mw->display, mw->id);
  set_wm_state (mw, NormalState, None);
}

void
map_icon (struct managed_window *mw)
{
  set_wm_state (mw, IconicState, None);
}

void
focus_from_wm (struct managed_window *mw, Time t)
{
  Display *d = mw->display;
  Window w = mw->id;
  XClientMessageEvent cme;
  switch (mw->focus_model) {
  case FocusNoInput:
    LIMP("Not focusing NoInput window.\n");
    break;
  case FocusPassive:
    LIMP("Focusing Passive window.\n");
    XSetInputFocus (d, w, RevertToParent, t);
    break;
  case FocusLocallyActive:
    LIMP("Focusing LocallyActive window and . . . ");
    XSetInputFocus (d, w, RevertToParent, t);
  case FocusGloballyActive:
    LIMP("Sending WM_TAKE_FOCUS message.\n");
    cme.window = w;
    cme.message_type = WM_PROTOCOLS;
    cme.format = 32;
    cme.data.l[0] = WM_TAKE_FOCUS;
    cme.data.l[1] = t;
    cme.data.l[2] = 0;
    cme.data.l[3] = 0;
    cme.data.l[4] = 0;
    XSendEvent (d, w, False, 0, (XEvent *) &cme);
    break;
  default:
    LIMP("Fell down a rabbit hole trying to focus from window manager.\n");
  }
}

void
try_focus_from_client_area (struct managed_window *mw, Time t)
{
  Display *d = mw->display;
  Window w = mw->id;
  XClientMessageEvent cme;
  switch (mw->focus_model) {
  case FocusNoInput:
    LIMP("Not focusing NoInput window.\n");
    break;
  case FocusPassive:
    LIMP("Focusing Passive window.\n");
    XSetInputFocus (d, w, RevertToParent, t);
    break;
  case FocusLocallyActive:
    LIMP("Focusing LocallyActive window & sending WM_TAKE_FOCUS message.\n");
    XSetInputFocus (d, w, RevertToParent, t);
    cme.window = w;
    cme.message_type = WM_PROTOCOLS;
    cme.format = 32;
    cme.data.l[0] = WM_TAKE_FOCUS;
    cme.data.l[1] = t;
    cme.data.l[2] = 0;
    cme.data.l[3] = 0;
    cme.data.l[4] = 0;
    XSendEvent (d, w, False, 0, (XEvent *) &cme);
    break;
  case FocusGloballyActive:
    LIMP("Don't send WM_TAKE_FOCUS for client area clicks on GloballyActive windows.\n");
    break;
  default:
    LIMP("Fell down a rabbit hole trying to focus from client area.\n");
  }
}

void
on_window_event (struct managed_window *mw, XEvent *e)
{
  switch (e->type) {
  case FocusIn:         on_focus_in (mw, (XFocusChangeEvent *) e);        break;
  case FocusOut:        on_focus_out (mw, (XFocusChangeEvent *) e);       break;
  case DestroyNotify:   on_destroy_notify (mw, (XDestroyWindowEvent *) e); break;
  case UnmapNotify:     on_unmap_notify (mw, (XUnmapEvent *) e);          break;
  case MapNotify:       on_map_notify (mw, (XMapEvent *) e);              break;
  case ReparentNotify:  on_reparent_notify (mw, (XReparentEvent *) e);    break;
  case ConfigureNotify: on_configure_notify (mw, (XConfigureEvent *) e);  break;
  case GravityNotify:   on_gravity_notify (mw, (XGravityEvent *) e);      break;
  case CirculateNotify: on_circulate_notify (mw, (XCirculateEvent *) e);  break;
  case PropertyNotify:  on_property_notify (mw, (XPropertyEvent *) e);    break;
  case ClientMessage:   on_client_message (mw, (XClientMessageEvent *) e); break;
  default:
    LIMP("Unselected event received on client window.\n");
    break;
  }
}

static void
retrieve_and_store_wm_name (struct managed_window *mw)
{
  if (mw->wm_name.value != NULL) XFree (mw->wm_name.value);  
  XGetWMName (mw->display, mw->id, &(mw->wm_name));
}

static void
retrieve_and_store_wm_icon_name (struct managed_window *mw)
{
  if (mw->wm_icon_name.value != NULL) XFree (mw->wm_icon_name.value);
  XGetWMIconName (mw->display, mw->id, &(mw->wm_icon_name));
}

static void
retrieve_and_store_wm_class (struct managed_window *mw)
{
  if (mw->class_hint != NULL) {
    if (mw->class_hint->res_name != NULL) XFree (mw->class_hint->res_name);
    if (mw->class_hint->res_class != NULL) XFree (mw->class_hint->res_class);
  }
  mw->class_hint = XAllocClassHint ();
  XGetClassHint (mw->display, mw->id, mw->class_hint);
}

static void
retrieve_and_store_wm_normal_hints (struct managed_window *mw)
{
  mw->normal_hints.flags = 0;
  mw->supplied_normal_hints = 0;
  XGetWMNormalHints (mw->display, mw->id, &(mw->normal_hints),
                     &(mw->supplied_normal_hints));
  if (!(mw->supplied_normal_hints & PWinGravity)){
    mw->normal_hints.win_gravity = NorthWestGravity;
  }
}

static void
retrieve_and_store_wm_transient_for (struct managed_window *mw)
{
  XGetTransientForHint (mw->display, mw->id, &(mw->transient_parent));
}

static void
retrieve_and_store_wm_colormap_windows (struct managed_window *mw)
{
  if (mw->colormap_windows != NULL) XFree (mw->colormap_windows);
  XGetWMColormapWindows (mw->display, mw->id, &(mw->colormap_windows),
                         &(mw->n_colormap_windows));

}

static void
retrieve_and_store_wm_client_machine (struct managed_window *mw)
{
  if (mw->client_machine.value != NULL) XFree (mw->client_machine.value);
  XGetWMClientMachine (mw->display, mw->id, &(mw->client_machine));
}

static void
retrieve_and_store_wm_hints (struct managed_window *mw)
{
  XWMHints *tmp_hints;
  if ((tmp_hints = XGetWMHints (mw->display, mw->id))) {
    if (tmp_hints->flags & InputHint) {
      if (tmp_hints->input == True) {
        switch (mw->focus_model) {
        case FocusNoInput: mw->focus_model = FocusPassive; break;
        case FocusPassive: /* No change. */ break;
        case FocusLocallyActive: /* No change. */ break;
        case FocusGloballyActive: mw->focus_model = FocusLocallyActive; break;
        }
      } else {
        switch (mw->focus_model) {
        case FocusNoInput: /* No change. */ break;
        case FocusPassive: mw->focus_model = FocusNoInput; break;
        case FocusLocallyActive: mw->focus_model = FocusGloballyActive; break;
        case FocusGloballyActive: /* No change. */ break;
        }
      }
    }
    if (tmp_hints->flags & IconWindowHint) {
      mw->wm_state_icon = tmp_hints->icon_window;
    }
    memcpy (&(mw->hints), tmp_hints, sizeof(XWMHints));
    XFree (tmp_hints);
  }
}

static void
retrieve_and_store_wm_protocols (struct managed_window *mw)
{
  Atom *protocols;
  int   n_protocols;
  if (XGetWMProtocols (mw->display, mw->id, &protocols, &n_protocols)) {
    Bool take_focus = False;
    for (int i=0;i<n_protocols;i++) {
      if (protocols[i] == WM_TAKE_FOCUS) {
        take_focus = True;
      } else if (protocols[i] == WM_DELETE_WINDOW) {
        mw->deletable = True;
      }
    }
    if (take_focus) { 
      switch (mw->focus_model) {
      case FocusNoInput: mw->focus_model = FocusGloballyActive; break;
      case FocusPassive: mw->focus_model = FocusLocallyActive; break;
      case FocusLocallyActive: /* No change. */ break;
      case FocusGloballyActive: /* No change. */ break;
      }
    } else {
      switch (mw->focus_model) {
      case FocusNoInput: /* No change. */ break;
      case FocusPassive: /* No change. */ break;
      case FocusLocallyActive: mw->focus_model = FocusPassive; break;
      case FocusGloballyActive: mw->focus_model = FocusNoInput; break;
      }
    }
    XFree (protocols);
  }
}

static void
retrieve_and_store_wm_state (struct managed_window *mw)
{
  LIMP("WM_STATE change seen on %lu\n", mw->id)
  /* Perhaps use this to check the prop's been set. */
}

static void
retrieve_and_store_wm_icon_size (struct managed_window *mw)
{
  /* This shouldn't be happening. */
}

static void
retrieve_and_store_wm_client_leader (struct managed_window *mw)
{
}

static void
retrieve_and_store_sm_client_id (struct managed_window *mw)
{
}

static void
retrieve_and_store_wm_window_role (struct managed_window *mw)
{
}

static void
retrieve_and_store_ice_protocols (struct managed_window *mw)
{
}

static void
retrieve_and_store_wm_command (struct managed_window *mw)
{
}


static void
store_property (struct managed_window *mw, Atom prop)
{
  char *atom_name;

  if ((atom_name = XGetAtomName(mw->display, prop))) {
    LIMP("Looking for %s\n", atom_name);
    XFree(atom_name);
  } else {
    LIMP("Can't find atom %lu\n", prop);
  }
  
  if (prop == WM_NAME) { retrieve_and_store_wm_name (mw);
  } else if (prop == WM_ICON_NAME) { retrieve_and_store_wm_icon_name (mw);
  } else if (prop == WM_CLASS) { retrieve_and_store_wm_class (mw);
  } else if (prop == WM_NORMAL_HINTS) { retrieve_and_store_wm_normal_hints (mw);
  } else if (prop == WM_TRANSIENT_FOR) { retrieve_and_store_wm_transient_for (mw);
  } else if (prop == WM_COLORMAP_WINDOWS) { retrieve_and_store_wm_colormap_windows (mw);
  } else if (prop == WM_CLIENT_MACHINE) { retrieve_and_store_wm_client_machine (mw);
  } else if (prop == WM_HINTS) { retrieve_and_store_wm_hints (mw);
  } else if (prop == WM_PROTOCOLS) { retrieve_and_store_wm_protocols (mw);
  } else if (prop == WM_STATE) { retrieve_and_store_wm_state (mw);
  } else if (prop == WM_ICON_SIZE) { retrieve_and_store_wm_icon_size (mw);
  } else if (prop == WM_CLIENT_LEADER) { retrieve_and_store_wm_client_leader (mw);
  } else if (prop == SM_CLIENT_ID) { retrieve_and_store_sm_client_id (mw);
  } else if (prop == WM_WINDOW_ROLE) { retrieve_and_store_wm_window_role (mw);
  } else if (prop == ICE_PROTOCOLS) { retrieve_and_store_ice_protocols (mw);
  } else if (prop == WM_COMMAND) { retrieve_and_store_wm_command (mw);
  } else { 
    LIMP ("Property not recognized.\n");
  }
}

static void
remove_property (struct managed_window *mw, Atom prop)
{
  if (prop == WM_NAME) {
    if (mw->wm_name.value != NULL) XFree (mw->wm_name.value);
  } else if (prop == WM_ICON_NAME) {
    if (mw->wm_icon_name.value != NULL) XFree (mw->wm_icon_name.value);
  } else if (prop == WM_CLASS) {
    if (mw->class_hint->res_name != NULL) XFree (mw->class_hint->res_name);
    if (mw->class_hint->res_class != NULL) XFree (mw->class_hint->res_class);
  } else if (prop == WM_NORMAL_HINTS) {
  } else if (prop == WM_TRANSIENT_FOR) {
  } else if (prop == WM_COLORMAP_WINDOWS) {
    if (mw->colormap_windows != NULL) XFree (mw->colormap_windows);
  } else if (prop == WM_CLIENT_MACHINE) {
    if (mw->client_machine.value != NULL) XFree (mw->client_machine.value);
  } else if (prop == WM_HINTS) {
  } else if (prop == WM_PROTOCOLS) {
  } else if (prop == WM_STATE) { /* Perhaps use this to check the prop's been set. */
  } else if (prop == WM_ICON_SIZE) { /* This shouldn't be happening. */
  } else if (prop == WM_CLIENT_LEADER) { /* Ignore the rest? */
  } else if (prop == SM_CLIENT_ID) {
  } else if (prop == WM_WINDOW_ROLE) {
  } else if (prop == ICE_PROTOCOLS) {
  } else if (prop == WM_COMMAND) {
  } else { 
    LIMP ("Property not recognized.");
  }
}

static void
set_wm_state (struct managed_window *mw, unsigned long state, Window icon)
{
  unsigned long wm_state[2];
  wm_state[0] = mw->wm_state_state = state;
  wm_state[1] = mw->wm_state_icon = icon;
  XChangeProperty (mw->display, mw->id, WM_STATE, WM_STATE, 32,
                   PropModeReplace, (unsigned char *) wm_state, 2);
}

static void
legacy_raise_on_focus (struct managed_window *mw)
{
  switch (mw->focus_model) {
  case FocusNoInput:
    LIMP("Not raising NoInput window.\n");
    break;
  case FocusPassive:
    LIMP("Raising Passive window.\n");
    XRaiseWindow (mw->display, mw->id);
    break;
  case FocusLocallyActive:
    LIMP("Raising LocallyActive window.\n");
    XRaiseWindow (mw->display, mw->id);
    break;
  case FocusGloballyActive:
    LIMP("Not raising GloballyActive window.\n");
    break;
  default:
    break;
  }
}

static void
on_focus_in (struct managed_window *mw, XFocusChangeEvent *e)
{
  if (e->mode == NotifyNormal || e->mode == NotifyWhileGrabbed) {
    current_focus = mw;
    legacy_raise_on_focus (mw);
  }
}

static void
on_focus_out (struct managed_window *mw, XFocusChangeEvent *e)
{
  if (e->mode == NotifyNormal || e->mode == NotifyWhileGrabbed) {
    if (current_focus == mw) current_focus = NULL;
  }
}

static void
on_destroy_notify (struct managed_window *mw, XDestroyWindowEvent *e)
{
  remove_window (e->display, e->window);
}

static void
on_unmap_notify (struct managed_window *mw, XUnmapEvent *e)
{
  switch (mw->wm_state_state) {
  case WithdrawnState:
    LIMP("UnmapNotify while in WithdrawnState???\n");
    break;
  case NormalState:
    LIMP("UnmapNotify into WithdrawnState.\n");
    /* Gaol the property removal, in case the window is being destroyed.
     * Or check the queue to see if there's a DestroyNotify coming.
     * For now, just forget the window.
     */
    remove_window (e->display, e->window);
    break;
  case IconicState:
    LIMP("UnmapNotify while iconifying.\n");
    break;
  default:
    LIMP("Fell down a rabbit hole looking for the state of an unmapped window.\n");
  }
}

static void
on_map_notify (struct managed_window *mw, XMapEvent *e)
{
  /* Useful? */
}

static void
on_reparent_notify (struct managed_window *mw, XReparentEvent *e)
{
  /* This may be used to confirm putting the window in a frame. */
}

static void
on_configure_notify (struct managed_window *mw, XConfigureEvent *e)
{
  /* For now just update the stored geometry. */
  mw->geometry.x = e->x;
  mw->geometry.y = e->y;
  mw->geometry.width = e->width;
  mw->geometry.height = e->height;
  /* Border width shouldn't change. */
  /* Sibling shouldn't change perhaps unless tabbing. */
  /* override_redirect will be of interest if the client
   * chooses ResizeRedirect. Not sure how yet.
   */
}

static void
on_gravity_notify (struct managed_window *mw, XGravityEvent *e)
{
  /* Don't know if I'll see this or have use for it. */
}

static void
on_circulate_notify (struct managed_window *mw, XCirculateEvent *e)
{
  /* Shouldn't happen. */
}

static void
on_property_notify (struct managed_window *mw, XPropertyEvent *e)
{
  switch (e->state) {
  case PropertyNewValue: store_property (mw, e->atom); break;
  case PropertyDelete: remove_property (mw, e->atom); break;
  default: LIMP("Fell through a rabbit hole.");
  }
}

static void
on_client_message (struct managed_window *mw, XClientMessageEvent *e)
{
}
