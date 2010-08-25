--- awesome image API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("image")

--- Image objects.
-- @field width The image width. Immutable.
-- @field height The image height. Immutable.
-- @field alpha Boolean indicating if the image alpha channel is present.
-- @class table
-- @name image

--- Performs 90 degree rotations on the current image. Passing 0 orientation does not rotate, 1
-- rotates clockwise by 90 degree, 2, rotates clockwise by 180 degrees, 3 rotates clockwise by 270
-- degrees.
-- @param rotation The rotation to perform.
-- @name rotate
-- @class function

--- Rotate an image with specified angle radians and return a new image.
-- @param angle The angle in radians.
-- @return A rotated image.
-- @name rotate
-- @class function

--- Crop an image to the given rectangle.
-- @param x The top left x coordinate of the rectangle.
-- @param y The top left y coordinate of the rectangle.
-- @param width The width of the rectangle.
-- @param height The height of the rectangle.
-- @return A cropped image.
-- @name crop
-- @class function

--- Crop the image to the given rectangle and scales it.
-- @param x The top left x coordinate of the source rectangle.
-- @param y The top left y coordinate of the source rectangle.
-- @param width The width of the source rectangle.
-- @param height The height of the source rectangle.
-- @param dest_width The width of the destination rectangle.
-- @param dest_height The height of the destination rectangle.
-- @param A cropped image.
-- @name crop_and_scale
-- @class function

--- Draw a pixel in an image.
-- @param x The x coordinate of the pixel to draw.
-- @param y The y coordinate of the pixel to draw.
-- @param The color to draw the pixel in.
-- @name draw_pixel
-- @class function

--- Draw a line in an image.
-- @param x1 The x1 coordinate of the line to draw.
-- @param y2 The y1 coordinate of the line to draw.
-- @param x2 The x2 coordinate of the line to draw.
-- @param y2 The y2 coordinate of the line to draw.
-- @param color The color to draw the line in.
-- @name draw_line
-- @class function

--- Draw a rectangle in an image.
-- @param x The x coordinate of the rectangles top left corner.
-- @param y The y coordinate of the rectangles top left corner.
-- @param width The width of the rectangle.
-- @param height The height of the rectangle.
-- @param fill True to fill the rectangle, false otherwise.
-- @param color The color to draw the rectangle with.
-- @name draw_rectangle
-- @class function

--- Draw a rectangle in an image with gradient color.
-- @param x The x coordinate of the rectangles top left corner.
-- @param y The y coordinate of the rectangles top left corner.
-- @param width The width of the rectangle.
-- @param height The height of the rectangle.
-- @param colors A table with the color to draw the rectangle. You can specified the color
-- distance from the previous one by setting t[color] = distance.
-- @param angle The angle of the gradient.
-- @name draw_rectangle_gradient
-- @class function

--- Draw a circle in an image.
-- @param x The x coordinate of the center of the circle.
-- @param y The y coordinate of the center of the circle.
-- @param width The horizontal amplitude.
-- @param height The vertical amplitude.
-- @param fill True if the circle should be filled, false otherwise.
-- @param color The color to draw the circle with.
-- @name draw_circle
-- @class function

--- Saves the image to the given path. The file extension (e.g. .png or .jpg) will affect the
-- output format.
-- @param path An image path.
-- @name save
-- @class function

--- Insert one image into another.
-- @param image The image to insert.
-- @param offset_x The x offset of the image to insert (optional).
-- @param ofsset_y The y offset of the image to insert (optional).
-- @param offset_h_up_right The horizontal offset of the upper right image corner (optional).
-- @param offset_v_up_right The vertical offset of the upper right image corner (optional).
-- @param offset_h_low_left The horizontal offset of the lower left image corner (optional).
-- @param offset_v_low_left The vertical offset of the lower left image corner (optional).
-- @param source_x The x coordinate of the source rectangle (optional).
-- @param source_y The y coordinate of the source rectangle (optional).
-- @param source_width The width of the source rectangle (optional).
-- @param source_height The height of the source rectangle (optional).
-- @name insert
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
