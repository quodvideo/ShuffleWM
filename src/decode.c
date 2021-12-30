#include <stdio.h>
#include <X11/Xlib.h>
#include "decode.h"

#define WIMP(...) fprintf (stderr, __VA_ARGS__);

static char *event_names[] = {
  "0",
  "1",
  "KeyPress",
  "KeyRelease",
  "ButtonPress",
  "ButtonRelease",
  "MotionNotify",
  "EnterNotify",
  "LeaveNotify",
  "FocusIn",
  "FocusOut",
  "KeymapNotify",
  "Expose",
  "GraphicsExpose",
  "NoExpose",
  "VisibilityNotify",
  "CreateNotify",
  "DestroyNotify",
  "UnmapNotify",
  "MapNotify",
  "MapRequest",
  "ReparentNotify",
  "ConfigureNotify",
  "ConfigureRequest",
  "GravityNotify",
  "ResizeRequest",
  "CirculateNotify",
  "CirculateRequest",
  "PropertyNotify",
  "SelectionClear",
  "SelectionRequest",
  "SelectionNotify",
  "ColormapNotify",
  "ClientMessage",
  "MappingNotify",
  "GenericEvent"
};
static char *modes[] = {
  "Normal",
  "Grab",
  "Ungrab",
  "WhileGrabbed"
};
static char *details[] = {
  "Ancestor",
  "Virtual",
  "Inferior",
  "Nonlinear",
  "NonlinearVirtual",
  "Pointer",
  "PointerRoot",
  "None"
};
static char *visibility_state[] = {
  "Unobscured",
  "Partially Obscured",
  "Fully Obscured"
};
static char *stack_details[] = {
  "Above",
  "Below",
  "TopIf",
  "BottomIf",
  "Opposite"
};
static char *window_changes[] = {
  "X",
  "Y",
  "Width",
  "Height",
  "BorderWidth",
  "Sibling",
  "StackMode"
};
static char *modifier_masks[] = {
  "Shift",
  "Lock",
  "Control",
  "Mod1",
  "Mod2",
  "Mod3",
  "Mod4",
  "Mod5",
  "Button1",
  "Button2",
  "Button3",
  "Button4",
  "Button5",
  NULL,
  NULL,
  "Any"
};
static char *place[] = { "top", "bottom" };
static char *requests[] = { "Modifier", "Keyboard", "Pointer" };
static char *property_state[] = { "has new value", "deleted" };
static char *sentp[] = { " real", " from SendEvent" };
static char *overridesp[] = { "", " overrides redirection" };
static char *boop[] = { "No", "Yes" };
static char *request_codes[] = {
  [62] = "CopyArea",
  [63] = "CopyPlane"
};
static void on_key (XKeyEvent *e);
static void on_button (XButtonEvent *e);
static void on_motion_notify (XMotionEvent *e);
static void on_crossing (XCrossingEvent *e);
static void on_focus (XFocusChangeEvent *e);
static void on_keymap_notify (XKeymapEvent *e);
static void on_expose (XExposeEvent *e);
static void on_graphics_expose (XGraphicsExposeEvent *e);
static void on_no_expose (XNoExposeEvent *e);
static void on_visibility_notify (XVisibilityEvent *e);
static void on_create_notify (XCreateWindowEvent *e);
static void on_destroy_notify (XDestroyWindowEvent *e);
static void on_unmap_notify (XUnmapEvent *e);
static void on_map_notify (XMapEvent *e);
static void on_map_request (XMapRequestEvent *e);
static void on_reparent_notify (XReparentEvent *e);
static void on_configure_notify (XConfigureEvent *e);
static void on_configure_request (XConfigureRequestEvent *e);
static void on_gravity_notify (XGravityEvent *e);
static void on_resize_request (XResizeRequestEvent *e);
static void on_circulate_notify (XCirculateEvent *e);
static void on_circulate_request (XCirculateRequestEvent *e);
static void on_property_notify (XPropertyEvent *e);
static void on_selection_clear (XSelectionClearEvent *e);
static void on_selection_request (XSelectionRequestEvent *e);
static void on_selection_notify (XSelectionEvent *e);
static void on_colormap_notify (XColormapEvent *e);
static void on_client_message (XClientMessageEvent *e);
static void on_mapping_notify (XMappingEvent *e);
static void on_generic_event (XGenericEvent *e);

void
decode_event (XEvent *e)
{
  WIMP("%s #%lu%s\t Window: %lu\n",
       event_names[e->type], e->xany.serial, sentp[e->xany.send_event], e->xany.window);
  switch (e->type) {
  case KeyPress:
  case KeyRelease: on_key ((XKeyEvent *) e); break;
  case ButtonPress:
  case ButtonRelease: on_button ((XButtonEvent *) e); break;
  case MotionNotify: on_motion_notify ((XMotionEvent *) e); break;
  case EnterNotify:
  case LeaveNotify: on_crossing ((XCrossingEvent *) e); break;
  case FocusIn:
  case FocusOut: on_focus ((XFocusChangeEvent *) e); break;
  case KeymapNotify: on_keymap_notify ((XKeymapEvent *) e); break;
  case Expose: on_expose ((XExposeEvent *) e); break;
  case GraphicsExpose: on_graphics_expose ((XGraphicsExposeEvent *)e); break;
  case NoExpose: on_no_expose ((XNoExposeEvent *) e); break;
  case VisibilityNotify: on_visibility_notify ((XVisibilityEvent *) e); break;
  case CreateNotify: on_create_notify ((XCreateWindowEvent *) e); break;
  case DestroyNotify: on_destroy_notify ((XDestroyWindowEvent *) e); break;
  case UnmapNotify: on_unmap_notify ((XUnmapEvent *) e); break;
  case MapNotify: on_map_notify ((XMapEvent *) e); break;
  case MapRequest: on_map_request ((XMapRequestEvent *) e); break;
  case ReparentNotify: on_reparent_notify ((XReparentEvent *) e); break;
  case ConfigureNotify: on_configure_notify ((XConfigureEvent *) e); break;
  case ConfigureRequest: on_configure_request ((XConfigureRequestEvent *) e); break;
  case GravityNotify: on_gravity_notify ((XGravityEvent *) e); break;
  case ResizeRequest: on_resize_request ((XResizeRequestEvent *) e); break;
  case CirculateNotify: on_circulate_notify ((XCirculateEvent *) e); break;
  case CirculateRequest: on_circulate_request ((XCirculateRequestEvent *) e); break;
  case PropertyNotify: on_property_notify ((XPropertyEvent *) e); break;
  case SelectionClear: on_selection_clear ((XSelectionClearEvent *) e); break;
  case SelectionRequest: on_selection_request ((XSelectionRequestEvent *) e); break;
  case SelectionNotify: on_selection_notify ((XSelectionEvent *) e); break;
  case ColormapNotify: on_colormap_notify ((XColormapEvent *) e); break;
  case ClientMessage: on_client_message ((XClientMessageEvent *) e); break;
  case MappingNotify: on_mapping_notify ((XMappingEvent *) e); break;
  case GenericEvent: on_generic_event ((XGenericEvent *) e); break;
  default: WIMP("Woah!\n");
  }
}

void
on_key (XKeyEvent *e)
{
  KeySym sym = XLookupKeysym (e, 0);
  char *cap = XKeysymToString (sym);
  int i;
  WIMP("\t     Window: %lu\n"
       "\t       Root: %lu\n"
       "\t  Subwindow: %lu\n"
       "\t       Time: %lu\n"
       "\t     Coords: %d %d\n"
       "\tRoot Coords: %d %d\n",
       e->window, e->root, e->subwindow,
       e->time, e->x, e->y, e->x_root, e->y_root);
  WIMP("\t      State:");
  for (i=0;i<16;i++) if (e->state & (1<<i)) WIMP(" %s", modifier_masks[i]);
  WIMP("\n\t        Key: %s\n", cap);
  WIMP("\tSame screen: %s\n", boop[e->same_screen]);
}

void
on_button (XButtonEvent *e)
{
  int i;
  WIMP("\t     Window: %lu\n"
       "\t       Root: %lu\n"
       "\t  Subwindow: %lu\n"
       "\t       Time: %lu\n"
       "\t     Coords: %d %d\n"
       "\tRoot Coords: %d %d\n",
       e->window, e->root, e->subwindow,
       e->time, e->x, e->y, e->x_root, e->y_root);
  WIMP("\t      State:");
  for (i=0;i<16;i++) if (e->state & (1<<i)) WIMP(" %s", modifier_masks[i]);
  WIMP("\n\t     Button: %u\n\tSame screen: %s\n",
       e->button, boop[e->same_screen]);
}

void
on_motion_notify (XMotionEvent *e)
{
  int i;
  WIMP("\t     Window: %lu\n"
       "\t       Root: %lu\n"
       "\t  Subwindow: %lu\n"
       "\t       Time: %lu\n"
       "\t     Coords: %d %d\n"
       "\tRoot Coords: %d %d\n",
       e->window, e->root, e->subwindow,
       e->time, e->x, e->y, e->x_root, e->y_root);
  WIMP("\t      State:");
  for (i=0;i<16;i++) if (e->state & (1<<i)) WIMP(" %s", modifier_masks[i]);
  WIMP("\n\t    Is hint: %s\n\tSame screen: %s\n",
       boop[(int) e->is_hint], boop[e->same_screen]);
}

void
on_crossing (XCrossingEvent *e)
{
  int i;
  WIMP("\t     Window: %lu\n"
       "\t       Root: %lu\n"
       "\t  Subwindow: %lu\n"
       "\t       Time: %lu\n"
       "\t     Coords: %d %d\n"
       "\tRoot Coords: %d %d\n"
       "\t       Mode: %s\n"
       "\t     Detail: %s\n"
       "\tSame screen: %s\n"
       "\t      Focus: %s\n",
       e->window, e->root, e->subwindow,
       e->time, e->x, e->y, e->x_root, e->y_root,
       modes[e->mode], details[e->detail], boop[e->same_screen],
       boop[e->focus]);
  WIMP("\t      State:");
  for (i=0;i<16;i++) if (e->state & (1<<i)) WIMP(" %s", modifier_masks[i]);
  WIMP("\n");

}

void
on_focus (XFocusChangeEvent *e)
{
  WIMP("\tWindow: %lu\n\t  Mode: %s\n\tDetail: %s\n",
       e->window, modes[e->mode], details[e->detail]);
}

void
on_keymap_notify (XKeymapEvent *e)
{
  WIMP("\t    Window: %lu\n\tKey Vector: %s\n", e->window, e->key_vector);
}

void
on_expose (XExposeEvent *e)
{
  WIMP("\tWindow: %lu\n\t  Area: %ux%u%+d%+d\n\t Count: %d\n",
       e->window, e->width, e->height, e->x, e->y, e->count);
}

void
on_graphics_expose (XGraphicsExposeEvent *e)
{
  WIMP("\t  Drawable: %lu\n"
       "\t      Area: %ux%u%+d+%+d\n"
       "\t     Count: %d\n"
       "\tMajor Code: %s\n"
       "\tMinor Code: %d\n",
       e->drawable, e->x, e->y, e->width, e->height,
       e->count, request_codes[e->major_code], e->minor_code);

}

void
on_no_expose (XNoExposeEvent *e)
{
  WIMP("\t  Drawable: %lu\n"
       "\tMajor Code: %s\n"
       "\tMinor Code: %d\n",
       e->drawable, request_codes[e->major_code], e->minor_code);
}

void
on_visibility_notify (XVisibilityEvent *e)
{
  WIMP("\tWindow: %lu\n\t State: %s\n", e->window, visibility_state[e->state]);
}

void
on_create_notify (XCreateWindowEvent *e)
{
  WIMP("\t Parent window: %lu\n"
       "\tCreated window: %lu%s\n"
       "\t      Geometry: %ux%u%+d%+d (%u)\n",
       e->parent, e->window, overridesp[e->override_redirect],
       e->width, e->height, e->x, e->y, e->border_width);
}

void
on_destroy_notify (XDestroyWindowEvent *e)
{
  WIMP("\t    Event window: %lu\n\tDestroyed window: %lu\n",
       e->event, e->window);
}

void
on_unmap_notify (XUnmapEvent *e)
{
  WIMP("\t   Event window: %lu\n\tUnmapped window: %lu%s\n",
       e->event, e->window, e->from_configure?" from UnmapGravity resize":"");
}

void
on_map_notify (XMapEvent *e)
{
  WIMP("\t Event window: %lu\n\tMapped window: %lu%s\n",
       e->event, e->window, overridesp[e->override_redirect]);
}

void
on_map_request (XMapRequestEvent *e)
{
  WIMP("\tParent: %lu\n\tWindow: %lu\n", e->parent, e->window);
}

void
on_reparent_notify (XReparentEvent *e)
{
  WIMP("\t     Event window: %lu\n"
       "\tReparented window: %lu%s\n"
       "\t    Parent window: %lu\n"
       "\t           Coords: %d %d\n",
       e->event, e->window, overridesp[e->override_redirect],
       e->parent, e->x, e->y);
}

void
on_configure_notify (XConfigureEvent *e)
{
  WIMP("\t     Event window: %lu\n"
       "\tConfigured window: %lu%s\n"
       "\t         Geometry: %ux%u%+d%+d (%u)\n"
       "\t          Sibling: %lu\n",
       e->event, e->window, overridesp[e->override_redirect],
       e->width, e->height, e->x, e->y, e->border_width,
       e->above);
}

void
on_configure_request (XConfigureRequestEvent *e)
{
  int i;
  WIMP("\t     Event window: %lu\n"
       "\tRequesting window: %lu\n"
       "\t         Geometry: %ux%u%+d%+d (%u)\n"
       "\t          Sibling: %lu\n"
       "\t           Detail: %s\n",
       e->parent, e->window, e->width, e->height, e->x, e->y, e->border_width,
       e->above, stack_details[e->detail]);
  WIMP("\t           Values:");
  for (i=0;i<7;i++) if (e->value_mask & (1<<i)) WIMP(" %s", window_changes[i]);
  WIMP("\n");
}

void
on_gravity_notify (XGravityEvent *e)
{
  WIMP("\tEvent window: %lu\n\tMoved window: %lu\n\t      Coords: %d %d\n",
       e->event, e->window, e->x, e->y);
}

void
on_resize_request (XResizeRequestEvent *e)
{
  WIMP("\tWindow: %lu\n\t  Size: %ux%u\n", e->window, e->width, e->height);
}

void
on_circulate_notify (XCirculateEvent *e)
{
  WIMP("\t    Event window: %lu\n\tRestacked window: %lu place on %s\n",
       e->event, e->window, place[e->place]);
}

void
on_circulate_request (XCirculateRequestEvent *e)
{
  WIMP("\t   Parent window: %lu\n\tRestacked window: %lu place on %s\n",
       e->parent, e->window, place[e->place]);
}

void
on_property_notify (XPropertyEvent *e)
{
  char *atom_name;
  atom_name = XGetAtomName (e->display, e->atom);
  WIMP("\t  Window: %lu\n\tProperty: %s %s\n\t    Time: %lu\n",
       e->window, atom_name, property_state[e->state], e->time);
  XFree (atom_name);
}

void
on_selection_clear (XSelectionClearEvent *e)
{
  char *selection_name;
  selection_name = XGetAtomName (e->display, e->selection);
  WIMP("\tWindow: %lu\n\tSelection: %s\n\tTime: %lu\n",
       e->window, selection_name, e->time);
  XFree (selection_name);
}

void
on_selection_request (XSelectionRequestEvent *e)
{
  char *selection_name, *target_name, *property_name;
  selection_name = XGetAtomName (e->display, e->selection);
  target_name = XGetAtomName (e->display, e->target);
  if (e->property != None) {
    property_name = XGetAtomName (e->display, e->property);
  } else {
    property_name = "None";
  }
  WIMP("\t    Owner: %lu\n"
       "\tRequestor: %lu\n"
       "\tSelection: %s\n"
       "\t   Target: %s\n"
       "\t Property: %s\n"
       "\t     Time: %lu\n",
       e->owner, e->requestor, selection_name, target_name, property_name,
       e->time);
  XFree (selection_name);
  XFree (target_name);
  if (e->property != None) XFree (property_name);
}

void
on_selection_notify (XSelectionEvent *e)
{
  char *selection_name, *target_name, *property_name;
  selection_name = XGetAtomName (e->display, e->selection);
  target_name = XGetAtomName (e->display, e->target);
  if (e->property != None) {
    property_name = XGetAtomName (e->display, e->property);
  } else {
    property_name = "None";
  }
  WIMP("\tRequestor: %lu\n"
       "\tSelection: %s\n"
       "\t   Target: %s\n"
       "\t Property: %s\n"
       "\t     Time: %lu\n",
       e->requestor, selection_name, target_name, property_name, e->time);
  XFree (selection_name);
  XFree (target_name);
  if (e->property != None) XFree (property_name);
}

void
on_colormap_notify (XColormapEvent *e)
{
  WIMP("\tWindow: %lu\n\tIt's complicated.\n", e->window);
}

void
on_client_message (XClientMessageEvent *e)
{
  char *message_type;
  if (e->message_type != None) {
    message_type = XGetAtomName (e->display, e->message_type);
  } else {
    message_type = "None";
  }
  WIMP("\tWindow: %lu\n\t  Type: %s\n\tFormat: %d\n",
       e->window, message_type, e->format);
  switch (e->format) {
  case 8:
    WIMP("\t%c %c %c %c %c\n\t%c %c %c %c %c\n"
         "\t%c %c %c %c %c\n\t%c %c %c %c %c\n",
         e->data.b[0], e->data.b[1], e->data.b[2], e->data.b[3], e->data.b[4],
         e->data.b[5], e->data.b[6], e->data.b[7], e->data.b[8], e->data.b[9],
         e->data.b[10], e->data.b[11], e->data.b[12], e->data.b[13], e->data.b[14],
         e->data.b[15], e->data.b[16], e->data.b[17], e->data.b[18], e->data.b[19]);
    break;
  case 16:
    WIMP("\t%hu %hu %hu %hu %hu\n\t%hu %hu %hu %hu %hu\n",
         e->data.s[0], e->data.s[1], e->data.s[2], e->data.s[3], e->data.s[4],
         e->data.s[5], e->data.s[6], e->data.s[7], e->data.s[8], e->data.s[9]);
    break;
  case 32:
    WIMP("\t%lu %lu %lu %lu %lu\n",
         e->data.l[0], e->data.l[1], e->data.l[2], e->data.l[3], e->data.l[4]);
    break;
  }
  if (e->message_type != None) XFree (message_type);
}

void
on_mapping_notify (XMappingEvent *e)
{
  WIMP("\tRequest: %s\n\t  First: %d\n\t  Count: %d\n",
       requests[e->request], e->first_keycode, e->count);
}

void
on_generic_event (XGenericEvent *e)
{
  WIMP("\tIt's Complicated.\n");
}
