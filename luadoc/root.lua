--- awesome root window API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("root")

--- Get or set global mouse bindings.
-- This binding will be available when you'll click on root window.
-- @param button_table An array of mouse button bindings objects, or nothing.
-- @return The array of mouse button bindings objects.
-- @name buttons
-- @class function

--- Get or set global key bindings.
-- This binding will be available when you'll press keys on root window.
-- @param keys_array An array of key bindings objects, or nothing.
-- @return The array of key bindings objects of this client.
-- @name keys
-- @class function

--- Set the root cursor.
-- @param cursor_name A X cursor name.
-- @name cursor
-- @class function

--- Send fake events. Usually the current focused client will get it.
-- @param event_type The event type: key_press, key_release, button_press, button_release
-- or motion_notify.
-- @param detail The detail: in case of a key event, this is the keycode to send, in
-- case of a button event this is the number of the button. In case of a motion
-- event, this is a boolean value which if true make the coordinates relatives.
-- @param x In case of a motion event, this is the X coordinate.
-- @param y In case of a motion event, this is the Y coordinate.
-- @name fake_input
-- @class function

--- Get the wiboxes attached to a screen.
-- @param -
-- @return A table with all wiboxes.
-- @name wiboxes
-- @class function

--- Get the wallpaper as a cairo surface or set it as a cairo pattern.
-- @param pattern A cairo pattern as light userdata
-- @return A cairo surface or nothing.
-- @name wallpaper
-- @class function

--- Get the attached tags.
-- @param -
-- @return A table with all tags.
-- @name tags
-- @class function
