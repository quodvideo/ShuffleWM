
static struct managed_window *focus_ring[512];
static int ring_focus = 0;
static int ring_end = 0;

void
build_focus_ring (Display *d, Window root)
{
  Window rootr, parent, *children;
  unsigned int nchildren;
  struct managed_window *mw;
  
  XQueryTree (d, root, &rootr, &parent, &children, &nchildren);

  ring_focus = 0;
  ring_end = 0;

  for (int i=nchildren;i>=0;i--) {
    mw = find_window (d, children[i]);
    if (mw) {
      focus_ring[ring_end] = mw;
      ring_end++;
    }
  }
  if (nchildren) {
    XFree (children);
  }
}

void
set_ring_focus_to_current_focus (Display *d)
{
  Window current_focus;
  int revert_to;

  XGetInputFocus (d, &current_focus, &revert_to);

  for (int i=0;i<ring_end;i++) {
    Window id = get_id (focus_ring[i]);
    if (id == current_focus) {
      ring_focus = i;
      break;
    }
  }
}

static void
begin_keyboard_focus_change (XKeyEvent *e)
{
  shuffle_mode = SwitchingWindows;
  build_focus_ring (e->display, e->root);
  set_ring_focus_to_current_focus (e->display);
}

static void
end_keyboard_focus_change (XKeyEvent *e)
{
  shuffle_mode = NoMode;
  ring_focus = 0;
  ring_end = 0;
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

