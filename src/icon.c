#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "shuffle.h"
#include "window.h"
#include "icon.h"

static XContext icon_context;

struct icon {
  Display *display;
  Window icon_id;
  Window window_id;
};

void
init_icons (Display *d)
{
  static Bool initialized = False;
  if (!initialized) {
    icon_context = XUniqueContext ();
    initialized = True;
  }
}

struct icon *
create_icon (Display *d, Window root, int x, int y,
             unsigned int width, unsigned int height, int rd)
{
  return NULL;
}

struct icon *
get_icon (Display *d, Window icon)
{
  return NULL;
}

void
delete_icon (Display *d, struct icon *i)
{
}

void
moveresize_icon (struct icon *i, int x, int y, unsigned int width, unsigned int height)
{
}

void
resize_icon (struct icon *i, unsigned int width, unsigned int height)
{
}

void
resize_icon_contents (struct icon *i, unsigned int width, unsigned int height)
{
}

void
on_icon_event (XEvent *e, struct icon *i)
{
}
