#ifndef MANAGER_H
#define MANAGER_H

Window get_manager_window    (Display *d, Window root);
Time   get_manager_timestamp (Display *d, Window root);
void   send_manager_message  (Display *d, Window root);
void   on_manager_event (XEvent *e, Window root);

#endif /* MANAGER_H */
