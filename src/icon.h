#ifndef ICON_H
#define ICON_H

struct icon;

void init_icons (Display *d);
void on_icon_event (struct icon *i, XEvent *e);

struct icon *create_icon (Display      *d,
                          Window        root,
                          int           x,
                          int           y,
                          unsigned int  width,
                          unsigned int  height,
                          int           rd);

struct icon *get_icon (Display *d, Window icon);

void delete_icon (Display *d, struct icon *i);

void moveresize_icon (struct icon  *i,
                      int           x,
                      int           y,
                      unsigned int  width,
                      unsigned int  height);
void resize_icon (struct icon  *i,
                  unsigned int  width,
                  unsigned int  height);
void resize_icon_contents (struct icon  *i,
                           unsigned int  width,
                           unsigned int  height);



#endif /* ICON_H */
