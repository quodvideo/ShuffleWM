#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

void on_button_press   (Display *d, XButtonEvent *e, Window w0, Window w1);
void on_button_release (Display *d, XButtonEvent *e, Window w0, Window w1);
void on_client_message (Display *d, XClientMessageEvent *e);

Time last_button_time;

int
main (int argc, char **argv)
{
  Display *d = XOpenDisplay (NULL);
  int s = DefaultScreen(d);

  /* This is the toplevel window. */
  Window w0 = XCreateSimpleWindow (d, RootWindow(d,s), 10, 10, 300, 300,
                                   0, BlackPixel(d,s), WhitePixel(d,s));

  /* All we need to know is if there's a button pressed on it. */
  XSelectInput (d, w0, ButtonPressMask);

  /* By not accepting input focus but taking the WM_TAKE_FOCUS message
   * this window follows the globally active input model.
   */
  XWMHints hints;
  hints.flags = InputHint;
  hints.input = False;
  XSetWMHints (d, w0, &hints);

  Atom protocols[1];
  protocols[0] = XInternAtom (d, "WM_TAKE_FOCUS", False);
  XSetWMProtocols (d, w0, protocols, 1);

  /* To use the XDND protocol, this property is needed.
   */
  Atom XA_XdndAware = XInternAtom (d, "XdndAware", False);
  int version = 5;
  XChangeProperty (d, w0, XA_XdndAware, XA_ATOM, 32, PropModeReplace,
                   (unsigned char *) &version, 1);


  /* This represents a draggable section of the window. */
  Window w1 = XCreateSimpleWindow (d, w0, 95, 95, 100, 100,
                                   10, BlackPixel(d,s), WhitePixel(d,s));

  /* We need to know if a button is pressed to start a drag and when the
   * button is released.
   */
  XSelectInput (d, w1, ButtonPressMask|ButtonReleaseMask);
  
  XMapSubwindows (d, w0);
  XMapWindow (d, w0);  
  while (1) {
    XEvent e;
    XNextEvent (d, &e);
    switch (e.type) {
    case ButtonPress:
      on_button_press (d, (XButtonEvent *) &e, w0, w1);
      break;
    case ButtonRelease:
      on_button_release (d, (XButtonEvent *) &e, w0, w1);
      break;
    case ClientMessage:
      on_client_message (d, (XClientMessageEvent *) &e);
      break;
    default:
      break;
    }

  }
}

void
on_button_press (Display *d, XButtonEvent *e, Window w0, Window w1)
{
  last_button_time = e->time;
  if (e->window == w0 && (e->button == Button1
                       || e->button == Button2
                       || e->button == Button3)) {
    XSetInputFocus (d, w0, RevertToParent, e->time);
  }
}

void
on_button_release (Display *d, XButtonEvent *e, Window w0, Window w1)
{
  if (e->subwindow == w1) {
    XSetInputFocus (d, w0, RevertToParent, e->time);
  }
}

void
on_client_message (Display *d, XClientMessageEvent *e)
{
  if (e->message_type == XInternAtom (d, "WM_PROTOCOLS", False)
      && e->format == 32
      && e->data.l[0] == XInternAtom (d, "WM_TAKE_FOCUS", False)) {
//    XSetInputFocus (d, e->window, RevertToParent, e->data.l[1]);
  }
}
