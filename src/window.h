#ifndef WINDOW_H
#define WINDOW_H

struct managed_window;

void init_windows (Display *d);

void on_window_event (struct managed_window *mw, XEvent *e);

struct managed_window *add_root_window (Display *d, Window w);
struct managed_window *add_window      (Display *d, Window w);
struct managed_window *find_window     (Display *d, Window w);
void                   remove_window   (Display *d, Window w);

int  get_state         (struct managed_window *mw);
int  get_initial_state (struct managed_window *mw);

void map_window        (struct managed_window *mw);
void map_icon          (struct managed_window *mw);

void focus_from_wm     (struct managed_window *mw, Time t);
void try_focus_from_client_area (struct managed_window *mw, Time t);


#endif /* WINDOW_H */
