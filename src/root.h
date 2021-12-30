#ifndef ROOT_H
#define ROOT_H

void init_root (Display *d, Window root);

struct managed_window *get_root_mw (Display *d, Window root);

void on_root_event (XEvent *e);

void focus_top (Display *d, Time t);

#endif /* ROOT_H */
