# ShuffleWM
The Shuffle window manager for INDEX.

> If you want to see what this is about, you need an X11 application that uses the _globally active_ input model. Most probably don't. You can check with _xprop_
>
>     xprop WM_PROTOCOLS WM_HINTS
>
> In the first line of output, look for
> 
>     WM_TAKE_FOCUS
>     
> In the remaining lines, look for
>
>     Client accepts input or input focus: False
>
> Just having a globally active application isn't enough. The application has to make use of the input model. Eventually there will be an INDEX app to demonstrate this.

INDEX is the INcompatible Desktop Environment for X.

It's just a window manager (shuffle) and a bunch of ideas at the moment.

INDEX will be built on the X11 windowing system. It also happens to be built on Linux. It might be incompatible with other things built on X11, and no Wayland support is planned.

INDEX will mostly abide by the ICCCM, but not in the way that's usually understood. INDEX will use what have been called "Client-Side Decorations", but really this means leaving window frame _controls_ up to the application. "Decorations" implies something superfluous; being able to move and resize windows is not superfluous. This is not the only difference from other X11 desktops.

Shuffle is a minimal window manager with just as much legacy support as I need to work with it. It's smaller than TWM right now. It's a click-to-focus window manager, but clicking won't force focus. This is really the most important feature: Shuffle will let you drag a selection from an unfocused window without focusing or raising that window. Most X11 apps don't support this right now. INDEX should change that.

Shuffle can't draw a window frame, and probably never will. With no frame, there's no way for Shuffle to move or resize a window with a simple button press. Instead Shuffle will use what on most keyboards is the "Windows" key to allow manipulation of legacy app windows. That key is called "Super" on the Linux systems I've seen, so I'll stick with that name. Super and a left click will let you move a window. Super and a right click will let you resize a window.

Here's an old explanation of the problem: https://quodvideo.wordpress.com/2017/11/17/focus-and-stacking-on-the-x-window-system/

The demo directory contains a simple program to demonstrate how a globally active window handles focus. It shows a black square outline in a white window. Changing to the window from the keyboard or by clicking the frame (if one is provided) will focus the window. If the window is not focused, clicking in the window but outside the square will focus the window as might be expected. What happens when the window is not focused and you click inside the square depends on your window manager. Lots of window managers will cause the window to be focused and raised. That is unfortunate. A window manager may also focus the window without raising it, or raise it without focusing it. Those are also unfortunate results. Ideally, nothing happens when you just click the square. If you release the mouse button while the pointer is still in the square, the demo window should receive focus. If you release the mouse button while the pointer is not inside the square, focus should not change.

The idea in the demo is that the square is like something you might drag away from the window. If the window is not focused and is underneath another window, you should be able to drag something from it to another window without the focus or the stacking changing. If you're used to a Mac, you probably do this all the time and may not have even noticed it or that it doesn't work on Windows or Linux. (There's one expection that does work on Windows: dragging files from the file manager.) I don't know what has to happen for this to work on Windows, but on Linux it's easy.

That's all there is for now.
