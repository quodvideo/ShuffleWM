# ShuffleWM
The Shuffle window manager for INDEX.

> If you want to see what this is about, you need an X11 application that uses the _globally active_ input model. Most probably don't. You can check with _xprop_:
>> xprop WM\_PROTOCOLS WM\_HINTS
> Look for "WM\_TAKE\_FOCUS" in the first line of output. Also look for
>> Client accepts input or input focus: False
> in the remaining lines.
>
> Just having a globally active application isn't enough. The application has to make use of the input model. Eventually there will be an INDEX app to demonstrate this.

INDEX is the INcompatible Desktop Environment for X.

It's just a window manager (shuffle) and a bunch of ideas at the moment.

INDEX will be built on the X11 windowing system. It also happens to be built on Linux. It might be incompatible with other things built on X11, and no Wayland support is planned.

INDEX will mostly abide by the ICCCM, but not in the way that's usually understood. INDEX will use what have been called "Client-Side Decorations", but really this means leaving window frame _controls_ up to the application. "Decorations" implies something superfluous; being able to move and resize windows is not superfluous. This is not the only difference from other X11 desktops.

Shuffle is a minimal window manager with just as much legacy support as I need to work with it. It's smaller than TWM right now. It's a click-to-focus window manager, but clicking won't force focus. This is really the most important feature: Shuffle will let you drag a selection from an unfocused window without focusing or raising that window. Most X11 apps don't support this right now. INDEX should change that.

Shuffle can't draw a window frame, and probably never will. With no frame, there's no way for Shuffle to move or resize a window with a simple button press. Instead Shuffle will use what on most keyboards is the "Windows" key to allow manipulation of legacy app windows. That key is called "Super" on the Linux systems I've seen, so I'll stick with that name. Super and a left click will let you move a window. Super and a right click will let you resize a window.

That's all there is for now. I'm not even sure how to upload the code into Github yet.
