--- awesome widget API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("widget")

--- Generic widget.
-- @field visible The widget visibility.
-- @field type The widget type. Can only be set using constructor. Type can be either textbox,
-- imagebox or systray.
-- @class table
-- @name widget

--- Set the widget buttons. It will receive a press or release event when of this buttons is
-- pressed.
-- @param buttons_table A table with buttons, or nil if you do not want to modify it.
-- @return A table with buttons the widget is listening to.
-- @name buttons
-- @class function

--- Get the widget extents, i.e., the size the widgets needs to be drawn.
-- @param -
-- @return A table with x, y, width and height.
-- @name extents
-- @class function

--- Textbox widgets.
-- @field text The text to display.
-- @field width The width of the textbox. Set to 0 for auto.
-- @field wrap The wrap mode: word, char, word_char.
-- @field ellipsize The ellipsize mode: start, middle or end.
-- @field border_width The border width to draw around.
-- @field border_color The border color.
-- @field align Text alignment, left, center or right.
-- @field margin Method to pass text margin: a table with top, left, right and bottom keys.
-- @field bg Background color.
-- @field bg_image Background image.
-- @field bg_align Background image alignment, left, center, right, bottom, top or middle
-- @field bg_resize Background resize.
-- @class table
-- @name textbox

--- Imagebox widget.
-- @field image The image to display.
-- @field bg The background color to use.
-- @class table
-- @name imagebox

--- Systray widget.
-- @class table
-- @name systray

--- Add a signal.
-- @param name A signal name.
-- @param func A function to call when the signal is emitted.
-- @name connect_signal
-- @class function

--- Remove a signal.
-- @param name A signal name.
-- @param func A function to remove.
-- @name disconnect_signal
-- @class function

--- Emit a signal.
-- @param name A signal name.
-- @param ... Various arguments, optional.
-- @name emit_signal
-- @class function

--- Get the number of instances.
-- @return The number of widget objects alive.
-- @name instances
-- @class function
