#ifndef MOVERESIZE_H
#define MOVERESIZE_H

void begin_move    (XButtonEvent *e);
void begin_resize  (XButtonEvent *e);
void do_move       (XMotionEvent *e);
void do_resize     (XMotionEvent *e);
void finish_move   (XButtonEvent *e);
void finish_resize (XButtonEvent *e);
void cancel_move   (XKeyEvent *e);
void cancel_resize (XKeyEvent *e);


#endif /* MOVERESIZE_H */
