# INDEX Menu Protocol

The INDEX menu protocol aims to provide a simple and fast way to have a global menu bar on a system built on X11. To keep the protocol simple, the window manager will be responsible for the display of the menu bar and submenus and notifying clients of menu selections. Clients will provide their menus to the window manager through a number of properties. The window manager will notify clients of menu item selections with client messages.

The client window properties take the form `_INDEX_MENU_n`, where `n` is an integer zero or greater.
```
_INDEX_MENU_0
_INDEX_MENU_1
_INDEX_MENU_2
_INDEX_MENU_3
.
.
.
```
The value of these properties is type `UTF8_STRING`. The content is a description of the menu items in a lisp-like syntax. Each property corresponds to an item on the menu bar and its submenu items. A simple File menu might appear as `_INDEX_MENU_1` with the following content:
```
(item
  (label "_File")
  (menu
    (item (label "_New")   (shortcut "Ctrl+N") (token "1"))
    (item (label "_Open")  (shortcut "Ctrl+O") (token "2"))
    (item (label "_Save")  (shortcut "Ctrl+S") (token "3"))
    (item (label "_Close") (shortcut "Ctrl+W") (token "4"))))
```
The whitespace can be collapsed in the property.

An underscore before a letter in a label indicates a keyboard key that can be used to activate an item when the menu is open. The shortcut is displayed on the menu, but each application must implement its own keyboard shortcuts. The token item provides a number which the will be used to indicate the action a user has chosen. The use of a token allows a consistent signalling system in different localizations, which should only change the label or shortcut.

I need to write a formal spec for this.

* menu: Contains items. A menu within an item creates a submenu.
* item: The rows of the menu. An empty item creates a dividing line
* label: The text to be displayed. An underscore before a character indicates a keyboard shortcut that can be used when the menu is showing.
* icon: The icon to be displayed. The format is yet to be determined.
* enabled: "true" if the item is enabled, "false" if it is disabled.
* shortcut: the keyboard shortcut that can be used from the application
* token: the unique integer identifier for the item
* active: "true" is the item applies to the selection, "false" if it does not, and "mixed" if it applies to only part of the selection
* default: For an item with a submenu, a default token can be activated without opening the submenu. This is familiar for OS/2 and IceWM users.

The menus are placed in different properties so small changes can be made without reparsing the entire menu. The window manager is expected to match the current menu to the properties of the focused window. It can keep its own copy which is updated on property notification events, or it can read from the property as needed to display the correct menu. Changes while the menu is showing (shouldn't happen?) . . .

The menus will be placed on the menu bar in the order of the property indices. After the system menu will be the menu specified by `_INDEX_MENU_0` then the one given by `_INDEX_MENU_1` and so on.

More to come.
