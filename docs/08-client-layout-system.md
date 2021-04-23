# The AwesomeWM client layout system

This document explains how to use clients layouts and how awesome manage them.

**Client layout**  refers to the mechanism awesome uses to place client on the screen. The layout definition can be a basic system where clients are all *floating* like in a normal *DE* (eg GNOME, KDE, ...) or tiled on the screen like in *tilled window manager* (eg i3, BSPWM, ...). It is also possible to define complex client layout mixing both concepts and even implementing a *[stateful](https://en.wikipedia.org/wiki/State_(computer_science)) placement strategy*.

Awesome WM manages client layouts per tag. It means each tag has its own layout selection and uses them independently from other tags. When multiple tags are selected at once, Awesome uses only client layouts from the first selected tag and apply it to all current clients.

## Layouts configuration

Layout can be configured by setting properties from the tag instance.

Example of creating a new tag with client layout parameters:
```lua
awful.tag.add("My Tag", {
    screen = screen.primary,
    layout = awful.layout.suit.tile,
    master_fill_policy = "master_width_factor",
    gap_single_client = false,
    gap = 15
})
```

Example of changing client layout parameters on an existing tag:
```lua
-- Change the gap for the tag `my_tag`:
my_tag.useless_gap = 10

-- Change the master_width_factor for the tag `my_tag`:
my_tag.master_width_factor = 0.5
```

Here is a list of common properties used to configure tags:

<table class='widget_list' border=1>
<tr>
<th align='center'>Property</th>
<th align='center'>Type</th>
<th align='center'>Description</th>
</tr>
<tr><td>gap</td><td>number</td><td>The gap (spacing, also called useless_gap) between clients.</td></tr>
<tr><td>gap_single_client</td><td>boolean</td><td>Enable gaps for a single client.</td></tr>
<tr><td>master_fill_policy</td><td>string</td><td>Set size fill policy for the master client(s).</td></tr>
<tr><td>master_count</td><td>integer</td><td>Set the number of master windows.</td></tr>
<tr><td>icon</td><td>path or surface</td><td>Set the tag icon.</td></tr>
<tr><td>column_count</td><td>integer</td><td>Set the number of columns.</td></tr>
</table>


## Creating new client layouts

* arrange function (params definition)
* needs a name property
* mouse_resize_handler function
