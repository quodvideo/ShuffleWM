#include <stdlib.h>
#include <X11/Xlib.h>
#include "shuffle.h"
#include "atoms.h"

Atom WM_Sn;

Atom MANAGER;
Atom TARGETS;
Atom MULTIPLE;
Atom TIMESTAMP;
Atom VERSION;

Atom ATOM_PAIR;
Atom WM_TAKE_FOCUS;
Atom WM_DELETE_WINDOW;
Atom WM_STATE;
Atom WM_NAME;

Atom WM_ICON_NAME;
Atom WM_CLASS;
Atom WM_NORMAL_HINTS;
Atom WM_TRANSIENT_FOR;
Atom WM_COLORMAP_WINDOWS;

Atom WM_CLIENT_MACHINE;
Atom WM_HINTS;
Atom WM_PROTOCOLS;
Atom WM_ICON_SIZE;
Atom WM_CLIENT_LEADER;

Atom SM_CLIENT_ID;
Atom WM_WINDOW_ROLE;
Atom ICE_PROTOCOLS;
Atom WM_COMMAND;
Atom WM_CHANGE_STATE;

Status
intern_display_atoms (Display *display)
{
#define N_NAMES 25
  char *names[N_NAMES] = {
    "MANAGER",
    "TARGETS",
    "MULTIPLE",
    "TIMESTAMP",
    "VERSION",

    "ATOM_PAIR",
    "WM_TAKE_FOCUS",
    "WM_DELETE_WINDOW",
    "WM_STATE",
    "WM_NAME",

    "WM_ICON_NAME",
    "WM_CLASS",
    "WM_NORMAL_HINTS",
    "WM_TRANSIENT_FOR",
    "WM_COLORMAP_WINDOWS",

    "WM_CLIENT_MACHINE",
    "WM_HINTS",
    "WM_PROTOCOLS",
    "WM_ICON_SIZE",
    "WM_CLIENT_LEADER",

    "SM_CLIENT_ID",
    "WM_WINDOW_ROLE",
    "ICE_PROTOCOLS",
    "WM_COMMAND",
    "WM_CHANGE_STATE"
  };
  Atom atoms[N_NAMES];
  int n = 0;
  Status status;
  status = XInternAtoms(display, names, N_NAMES, False, atoms);

  MANAGER = atoms[n]; n++;
  TARGETS = atoms[n]; n++;
  MULTIPLE = atoms[n]; n++;
  TIMESTAMP = atoms[n]; n++;
  VERSION = atoms[n]; n++;

  ATOM_PAIR = atoms[n]; n++;
  WM_TAKE_FOCUS = atoms[n]; n++;
  WM_DELETE_WINDOW = atoms[n]; n++;
  WM_STATE = atoms[n]; n++;
  WM_NAME = atoms[n]; n++;

  WM_ICON_NAME = atoms[n]; n++;
  WM_CLASS = atoms[n]; n++;
  WM_NORMAL_HINTS = atoms[n]; n++;
  WM_TRANSIENT_FOR = atoms[n]; n++;
  WM_COLORMAP_WINDOWS = atoms[n]; n++;

  WM_CLIENT_MACHINE = atoms[n]; n++;
  WM_HINTS = atoms[n]; n++;
  WM_PROTOCOLS = atoms[n]; n++;
  WM_ICON_SIZE = atoms[n]; n++;
  WM_CLIENT_LEADER = atoms[n]; n++;

  SM_CLIENT_ID = atoms[n]; n++;
  WM_WINDOW_ROLE = atoms[n]; n++;
  ICE_PROTOCOLS = atoms[n]; n++;
  WM_COMMAND = atoms[n]; n++;
  WM_CHANGE_STATE = atoms[n]; n++;
  
  if (n != N_NAMES) FAIL("This program is defective.");
#undef N_NAMES
  return status;
}
