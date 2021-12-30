#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "atoms.h"
#include "manager.h"
#include "targets.h"

/* Eventually this will support a number of side-effect targets.
 *
 * char *wm_sn_target_names[] = {
 *   "TARGETS",
 *   "MULTIPLE",
 *   "TIMESTAMP",
 *   "VERSION",
 *   "TILE_STACKED",
 *   "TILE_SIDE_BY_SIDE",
 *   "TILE_BEST",
 *   "CASCADE",
 *   "MOVERESIZE",
 * };
 *
 * To tile two or more windows side-by-side
 * Create a list of windows, starting from the left, to be tiled.
 * Place the list on a window as a property:
 *     XChangeProperty (d, w, MY_PROP, WINDOW, 32, PropModeReplace,
 *                      window_list, sizeof(window_list));
 * Resume event processing until a PropertyNotify event occurs.
 * Use the event timestamp to:
 *     XConvertSelection (d, WM_Sn, TILE_SIDE_BY_SIDE, MY_PROP, w, timestamp);
 */


static Bool convert_targets   (XSelectionRequestEvent *e);
static Bool convert_multiple  (XSelectionRequestEvent *e);
static Bool convert_timestamp (XSelectionRequestEvent *e);
static Bool convert_version   (XSelectionRequestEvent *e);

Bool
convert_selection (XSelectionRequestEvent *e)
{
  Atom target = e->target;
  Bool converted = False;
  if (e->property == None) {
    /* There's really no point to supporting older clients,
     * but allow it for TARGETS and TIMESTAMP.
     */
    e->property = target;
    /*  */ if (target == TARGETS)   { converted = convert_targets (e);
    } else if (target == TIMESTAMP) { converted = convert_timestamp (e);
    }
    /* Restore to the specified property, even though it looks like failure. */
    e->property = None;
  } else if (target == TARGETS)   { converted = convert_targets (e);
  } else if (target == MULTIPLE)  { converted = convert_multiple (e);
  } else if (target == TIMESTAMP) { converted = convert_timestamp (e);
  } else if (target == VERSION)   { converted = convert_version (e);
  }
  return converted;
 }

static Bool
convert_targets (XSelectionRequestEvent *e)
{
#define N_TARGETS 4
  Atom targets[N_TARGETS] = {
    TARGETS,
    MULTIPLE,
    TIMESTAMP,
    VERSION
  };
  XChangeProperty (e->display, e->requestor, e->property,
                   XA_ATOM, 32, PropModeReplace,
                   (unsigned char *) targets, N_TARGETS);
#undef N_TARGETS
  return True;
}

static Bool
convert_multiple (XSelectionRequestEvent *e)
{
  long offset = 0;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  Atom pair[2];
  Bool converted = True;
  /* An extra check, since we really need this property. */
  if (e->property == None) return False;

  /* This really should be two loops:
   *   1. Collect the ATOM_PAIRs.
   *   2. Convert the selections.
   * Using two loops, interaction between steps can be controlled.
   * Actually, that's wrong. ICCCM says the conversions are to be
   * independent.
   */
  do {
    XSelectionRequestEvent sre;
    XGetWindowProperty (e->display, e->requestor, e->property,
                        offset, 2L, False, ATOM_PAIR,
                        &actual_type, &actual_format, &nitems, &bytes_after,
                        (unsigned char **) &pair);
    sre.type = e->type;
    sre.serial = e->serial;
    sre.send_event = True;
    sre.display = e->display;
    sre.owner = e->owner;
    sre.requestor = e->requestor;
    sre.selection = e->selection;
    sre.target = pair[0];
    sre.property = pair[1];
    sre.time = e->time;
    /* MULTIPLE fails if any one fails. */
    converted = convert_selection (&sre)?converted:False;
    offset += 2;
  } while (bytes_after > 0);
  return converted;
}

static Bool
convert_timestamp (XSelectionRequestEvent *e)
{
  Time timestamp = get_manager_timestamp (e->display, None);
  XChangeProperty (e->display, e->requestor, e->property,
                   XA_INTEGER, 32, PropModeReplace,
                   (unsigned char *) &timestamp, 1);
  return True;
}

static Bool
convert_version (XSelectionRequestEvent *e)
{
  long version[2] = {2, 0L};
  XChangeProperty (e->display, e->requestor, e->property,
                   XA_INTEGER, 32, PropModeReplace,
                   (unsigned char *) &version, 2);
  return True;
}
