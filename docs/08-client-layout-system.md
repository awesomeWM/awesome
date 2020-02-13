# The AwesomeWM client layout system

This document explains how to use clients layouts and how awesome manage them.

**Client layout**  refers to the mechanism awesome uses to place client on the screen. Awesome distinguishes *floating* clients and *tiling* clients. Client layout only apply to tiling clients.

A clients layout defines how tiling clients are arranged on the screen. It is possible to define both, basic and more complexe placement strategy. It is also possible to implementing *[stateful](https://en.wikipedia.org/wiki/State_(computer_science)) placement strategy*.

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

-- Increase the gap with standard function:
awful.tag.incgap(5, my_tag)
```

Here is a list of common properties used to configure tags:

<table class='widget_list' border=1>
<tr>
<th align='center'>Property</th>
<th align='center'>Type</th>
<th align='center'>Description</th>
</tr>
<tr><td>useless_gap</td><td>number</td><td>The gap (spacing, also called useless_gap) between clients.</td></tr>
<tr><td>gap_single_client</td><td>boolean</td><td>Enable gaps for a single client.</td></tr>
<tr><td>master_fill_policy</td><td>string</td><td>Set size fill policy for the master client(s).</td></tr>
<tr><td>master_count</td><td>integer</td><td>Set the number of master windows.</td></tr>
<tr><td>icon</td><td>path (string) or surface</td><td>Set the tag icon.</td></tr>
<tr><td>column_count</td><td>integer</td><td>Set the number of columns.</td></tr>
</table>


## Creating new client layouts

### How clients layout internally works?

A layout is table which defines required property:

<table class='widget_list' border=1>
<tr>
<th align='center'>Property</th>
<th align='center'>Type</th>
<th align='center'>Description</th>
</tr>
<tr><td>name</td><td>string</td><td>The Layout name.</td></tr>
<tr><td>arrange</td><td>function</td><td>The function called to arrange clients.</td></tr>
<tr><td>need_focus_update</td><td>boolean</td><td>Optional property to say if the layout need to be updated (aka call the arrange function) on client focus signal.</td></tr>
<tr><td>mouse_resize_handler</td><td>function</td><td>Optional callback used by the `awful.mouse` API when a client is resized.</td></tr>
<tr><td>skip_gap</td><td>function</td><td>Optional callback to say when clients gap should be ignored.</td></tr>
</table>

Here is the most basic layout declaration awesome would accept:

```lua
local my_layout = {
    name = 'first_layout',
    arrange = function() end
}
```

If you want to build a module for your custom layout, you just need to return this table.

### The `arrange` function

The layout `arrange` function is called by `awful.layout` every time tag's clients need to be re-arranged. It is mostly called when clients get spawned or get deleted and when a tag is focused.

`awful.layout` uses this function as a stateless clients arrangement function and gives it a table with parameters for it to work. It is intended that this function changes clients geometries to arrange them on the screen.

The parameters table `awful.layout` gives to the `arrange` function is the following:

<table class='widget_list' border=1>
<tr>
<th align='center'>Property</th>
<th align='center'>Type</th>
<th align='center'>Description</th>
</tr>
<tr><td>workarea</td><td>table</td><td>The available workarea as returned by `screen.get_bounding_geometry` for the current tag's screen, with `honor_padding` and `honor_workarea` turned on and margins sets to the tag's `useless_gap` value. (see `screen.get_bounding_geometry`)</td></tr>
<tr><td>geometry</td><td>table</td><td>The screen geometry. (see `screen.geometry`)</td></tr>
<tr><td>clients</td><td>table</td><td>List of clients to arrange.</td></tr>
<tr><td>screen</td><td>number</td><td>Index of the tag's screen.</td></tr>
<tr><td>padding</td><td>table</td><td>The tag's screen padding. (see `screen.padding`)</td></tr>
<tr><td>useless_gap</td><td>number</td><td>The tag's useless_gap.</td></tr>
</table>

Please note that even if there is a `useless_gap` property in the parameter table, the `arrange` function is not supposed to manage gap by itself. Awesome will shrink clients automatically after the `arrange` function is called.

### The `mouse_resize_handler` function

This function is used by the `awful.mouse` API to manage tilling clients mouse resize events. This event can be triggered with the `awful.mouse.client.resize` function. With the default `awesomerc.lua`, you can trigger this event with the combo MOD_KEY + Mouse Button 3 on a client.

This function prototype is the following:

```lua
mouse_resize_handler(client, corner, x, y)
```

<table class='widget_list' border=1>
<tr>
<th align='center'>Property</th>
<th align='center'>Type</th>
<th align='center'>Description</th>
</tr>
<tr><td>client</td><td>client</td><td>The client to resize.</td></tr>
<tr><td>corner</td><td>string</td><td>The corner where the mouse is placed as a string.</td></tr>
<tr><td>x</td><td>number</td><td>The X coordinate of the mouse/corner.</td></tr>
<tr><td>y</td><td>number</td><td>The Y coordinate of the mouse/corner.</td></tr>
</table>

Note: the closest corner and mouse coordinates are the same here because the mouse is moved to the corner when this function is triggered.

It is recommended to use a `mousegrabber` in your `mouse_resize_handler` implementation to manage the user resizing process.

### The `skip_gap` function

This function is called when `awful.layout` computes useless_gap values before calling the `arrange` function.

The `skip_gap` function is used to give back control to the layout to allow awesome to apply gap on the layout.

the function prototype is the following:

```lua
skip_gap(number_of_client, tag) : boolean
```
<table class='widget_list' border=1>
<tr>
<th align='center'>Property</th>
<th align='center'>Type</th>
<th align='center'>Description</th>
</tr>
<tr><td>number_of_client</td><td>number</td><td>The number of tiled clients on the tag.</td></tr>
<tr><td>tag</td><td>tag</td><td>The tag.</td></tr>
</table>
