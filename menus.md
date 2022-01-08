# INDEX Menu Protocol

The INDEX menu protocol aims to provide a simple and fast way to have a global menu bar on a system built on X11. It is expected that some application will manage the display of menus, but the protocol is intended to work without an ICCCM manager selection. Instead of a manager selection, the protocol uses window properties and client messages.

        _INDEX_MENUS

The \_INDEX_MENUS


        (menu
          (item
            (label "File")
            (icon "")
            (enabled "true")
            (menu
              (item (label "New")   (icon "") (enabled "true") (shortcut "Ctrl+N"))
              (item (label "Open")  (icon "") (enabled "true") (shortcut "Ctrl+O"))
              (item (label "Save")  (icon "") (enabled "true") (shortcut "Ctrl+S"))
              (item (label "Close") (icon "") (enabled "true") (shortcut "Ctrl+W"))))
          (item
            (label "Edit")
            (icon "")
            (enabled "true")
            (menu
              (item (label "Undo")        (icon "") (enabled "true"))
              (item (label "Redo")        (icon "") (enabled "true"))
              (item (label "")            (icon "") (enabled ""))
              (item (label "Cut")         (icon "") (enabled "true"))
              (item (label "Copy")        (icon "") (enabled "true"))
              (item (label "Paste")       (icon "") (enabled "true"))
              (item (label "Preferences") (icon "") (enabled "true")))))

  
