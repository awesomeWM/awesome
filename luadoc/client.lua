--- awesome client API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("client")

--- Client object.
-- @field window The X window id.
-- @field name The client title.
-- @field skip_taskbar True if the client does not want to be in taskbar.
-- @field type The window type (desktop, normal, dock, …).
-- @field class The client class.
-- @field instance The client instance.
-- @field pid The client PID, if available.
-- @field role The window role, if available.
-- @field machine The machine client is running on.
-- @field icon_name The client name when iconified.
-- @field icon The client icon.
-- @field screen Client screen.
-- @field hidden Define if the client must be hidden, i.e. never mapped,
-- invisible in taskbar.
-- @field minimized Define it the client must be iconify, i.e. only visible in
-- taskbar.
-- @field size_hints_honor Honor size hints, i.e. respect size ratio.
-- @field border_width The client border width.
-- @field border_color The client border color.
-- @field urgent The client urgent state.
-- @field content An image representing the client window content (screenshot).
-- @field focus The focused client.
-- @field opacity The client opacity between 0 and 1.
-- @field ontop The client is on top of every other windows.
-- @field above The client is above normal windows.
-- @field below The client is below normal windows.
-- @field fullscreen The client is fullscreen or not.
-- @field maximized_horizontal The client is maximized horizontally or not.
-- @field maximized_vertical The client is maximized vertically or not.
-- @field transient_for The client the window is transient for.
-- @field group_window Window identification unique to a group of windows.
-- @field leader_window Identification unique to windows spawned by the same command.
-- @field size_hints A table with size hints of the client: user_position,
-- user_size, program_position, program_size, etc.
-- @field sticky Set the client sticky, i.e. available on all tags.
-- @field modal Indicate if the client is modal.
-- @field focusable True if the client can receive the input focus.
-- @field shape_bounding The client's bounding shape as set by awesome as a (native) cairo surface.
-- @field shape_clip The client's clip shape as set by awesome as a (native) cairo surface.
-- @field shape_client_bounding The client's bounding shape as set by the program as a (native) cairo surface.
-- @field shape_client_clip The client's clip shape as set by the program as a (native) cairo surface.
-- @field blob A string containing data that will still be available after awesome restarts.
-- @class table
-- @name client

--- Get all clients into a table.
-- @param screen An optional screen number.
-- @return A table with all clients.
-- @name get
-- @class function

--- Check if a client is visible on its screen.
-- @return A boolean value, true if the client is visible, false otherwise.
-- @name isvisible
-- @class function

--- Return client geometry.
-- @param arg1 A table with new coordinates, or none.
-- @return A table with client coordinates.
-- @name geometry
-- @class function

--- Return client struts (reserved space at the edge of the screen).
-- @param struts A table with new strut values, or none.
-- @return A table with strut values.
-- @name struts
-- @class function

--- Get or set mouse buttons bindings for a client.
-- @param buttons_table An array of mouse button bindings objects, or nothing.
-- @return A table with all buttons.
-- @name buttons
-- @class function

--- Get or set keys bindings for a client.
-- @param keys_table An array of key bindings objects, or nothing.
-- @return A table with all keys.
-- @name keys
-- @class function

--- Access or set the client tags.
-- @param tags_table A table with tags to set, or none to get the current tags table.
-- @return A table with all tags.
-- @name tags
-- @class function

--- Kill a client.
-- @name kill
-- @class function

--- Swap a client with another one in global client list.
-- @param c A client to swap with.
-- @name swap
-- @class function

--- Raise a client on top of others which are on the same layer.
-- @name raise
-- @class function

--- Lower a client on bottom of others which are on the same layer.
-- @name lower
-- @class function

--- Stop managing a client.
-- @name unmanage
-- @class function

--- Change a xproperty.
-- @param name The name of the X11 property
-- @param value The new value for the property
-- @name set_xproperty
-- @class function

--- Get the value of a xproperty.
-- @param name The name of the X11 property
-- @name get_xproperty
-- @class function

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
-- @return The number of client objects alive.
-- @name instances
-- @class function
