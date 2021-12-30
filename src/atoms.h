#ifndef ATOMS_H
#define ATOMS_H

extern Atom WM_Sn;

extern Atom MANAGER;
extern Atom TARGETS;
extern Atom MULTIPLE;
extern Atom TIMESTAMP;
extern Atom VERSION;

extern Atom ATOM_PAIR;
extern Atom WM_TAKE_FOCUS;
extern Atom WM_DELETE_WINDOW;
extern Atom WM_STATE;
extern Atom WM_NAME;

extern Atom WM_ICON_NAME;
extern Atom WM_CLASS;
extern Atom WM_NORMAL_HINTS;
extern Atom WM_TRANSIENT_FOR;
extern Atom WM_COLORMAP_WINDOWS;

extern Atom WM_CLIENT_MACHINE;
extern Atom WM_HINTS;
extern Atom WM_PROTOCOLS;
extern Atom WM_ICON_SIZE;
extern Atom WM_CLIENT_LEADER;

extern Atom SM_CLIENT_ID;
extern Atom WM_WINDOW_ROLE;
extern Atom ICE_PROTOCOLS;
extern Atom WM_COMMAND;
extern Atom WM_CHANGE_STATE;

Status intern_display_atoms (Display *display);

#endif /* ATOMS_H */
