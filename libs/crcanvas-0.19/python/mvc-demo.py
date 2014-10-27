#! /usr/bin/env python

import math
import gobject, gtk
import gtk.gdk as gdk
import cairo
from crcanvas import *

if version < (0,11):
    raise "This program requires crcanvas version 0.11 or greater."


"""Demonstrates the model-view-controller pattern using crcanvas items.  I am
not sure if there exists any real use case for this sort of thing."""

def update_controllers(controllers, model):
    """There are five controllers.  One in each corner for resizing and one to
    the right for rotating.  This looks at the transform from the model to 
    determine where to place the controllers."""
    x, y, w, h = model.props.x, model.props.y, model.props.width, \
            model.props.height
    x1 = -w/2
    y1 = -h/2
    x2 = w/2
    y2 = h/2
    m = model.matrix
    coords = map(m.transform_point, (x1, x2, x2, x1, x2+20), 
            (y1, y1, y2, y2, y))

    [c.set(x = x, y = y) for c, (x, y) in zip(controllers, coords)]

def rotate_model(model, item, dx, dy):
    angle = math.atan2(item.props.y + dy, item.props.x + dx) - \
            math.atan2(item.props.y, item.props.x)
    matrix = model.matrix
    matrix.rotate(angle)
    model.matrix = matrix
def resize_model(model, id, dx, dy):
    if id == 0:
        model.props.width -= dx
        model.props.height -= dy
    elif id == 1:
        model.props.width += dx
        model.props.height -= dy
    elif id == 2:
        model.props.width += dx
        model.props.height += dy
    elif id == 3:
        model.props.width -= dx
        model.props.height += dy
    model.props.x += dx/2
    model.props.y += dy/2

def event(item, event, matrix, pick_item, model):
    if event.type == gdk.ENTER_NOTIFY:
        item.props.line_width = 3
    elif event.type == gdk.BUTTON_PRESS:
        item.init_x = event.x
        item.init_y = event.y
        return True
    elif event.type == gdk.MOTION_NOTIFY:
        dx = event.x - item.init_x
        dy = event.y - item.init_y
        if item.id == 4: rotate_model(model, item, dx, dy)
        else: resize_model(model, item.id, dx, dy)
        item.update_controllers()
        return True
    elif event.type == gdk.BUTTON_RELEASE:
        pass
    elif event.type == gdk.LEAVE_NOTIFY:
        item.props.line_width = 1
    return False

# The model will be a rectangle.
model = Rectangle(width = 80, height = 30, outline_color = 'black')
model.add(Text(text = "Model", fill_color = 'black', anchor =
    gtk.ANCHOR_CENTER))

# The controllers will be four red rectangles on each corner and a swirl shape
# to the right.
controllers = [Rectangle(width = 7, height = 7, 
        outline_color = 'red', line_width = 1, test_fill = True) \
        for i in range(4)]

# The Path item can be used to render any arbitrary cairo path.
swirl = Path(outline_color = 'red', test_rectangle = True)
def make_path(item, ct):
    ct.new_path()
    ct.move_to(0, 0)
    ct.curve_to(15, 15, -30, -15, 0, -15)
    ct.line_to(-3, -13)
    ct.move_to(0, -15)
    ct.line_to(-2.5, -10)
    return True
swirl.connect("make-path", make_path)

controllers.append(swirl)

#this puts the controllers into position
update_controllers(controllers, model)

for i, c in enumerate(controllers):
    c.id = i
    c.update_controllers = lambda : update_controllers(controllers, model)
    c.connect('event', event, model)

# Lets make four different viewers at various locations
viewers = [Item(x = x, y = y) for x, y in [(50, 50), (300, 300), (50, 300), \
        (300, 50)]]

#put the same model into each of the four viewers.
[v.add(model) for v in viewers]

#put the controller only into the first viewer
[viewers[0].add(c) for c in controllers]

# apply some arbitrary transformations to the viewers.
m = [i.matrix for i in viewers[1:]]
m[0].scale(1,3)
m[1].rotate(45 * math.pi/180)
m[2].scale(3,1)
for v, mx in zip(viewers[1:], m):
    v.matrix = mx

window1 = gtk.Window()
window1.connect('destroy', gtk.main_quit)

canvas = Canvas()
canvas.set_size_request(500, 500)

window1.add(canvas)

[canvas.props.root.add(v) for v in viewers]

window1.show_all()


# Now another window with some more viewers sprinkled around.
locations = [(25, 25), (25, 25 ), (100, 25), (100, 25), (62, 62), 
            (25, 85), (43, 92), (62, 100), (81, 92), (100, 85)]

m = cairo.Matrix(.25, 0, 0, .25, 0, 0)
viewers = [Item(matrix = m) for i in locations]
[v.set(x = x, y = y) for v, (x, y) in zip(viewers, locations)]
for i in viewers[1], viewers[3]:
    m = i.matrix
    m.rotate(math.pi/2)
    i.matrix = m
[v.add(model) for v in viewers]

window2 = gtk.Window()
window2.connect('destroy', gtk.main_quit)
canvas = Canvas(auto_scale = True, maintain_center = True)
canvas.set_size_request(125, 125)
window2.add(canvas)
[canvas.props.root.add(v) for v in viewers]
window2.show_all()
window2.move(gdk.screen_width() - window2.allocation.width, 0)

def spin(*args):
    def rotate(v, a):
        m = v.matrix
        m.rotate(a)
        v.matrix = m
    [rotate(v, .5) for v in viewers[:2]]
    [rotate(v, -.5) for v in viewers[2:4]]
    return True
gobject.timeout_add(200, spin)

gtk.main()

