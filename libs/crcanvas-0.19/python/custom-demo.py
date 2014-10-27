#! /usr/bin/env python

import random, math
import gtk.gdk as gdk
import gtk
from crcanvas import *
import cairo

"""This program demonstrates how to create any arbitrary custom canvas item. For
object oriented style programming, the Item or Path may be subclassed.  For
functional style programming, paint and calculate-bounds or make-path
functions may be connected as gobject callbacks."""

if version < (0,11):
    raise "This program requires crcanvas version 0.11 or greater."


def pick_color():
    return random.choice(["yellow", "green", "cyan", "blue", "magenta"])

class Blob1(Item):
    """This class demonstrates how to create an arbitrary custom canvas item.
    Typically the paint, calculate-bounds, and test virtual methods need to be
    overridden.  You can avoid overriding the the test method by setting
    test_rectangle to True; however, the test won't be as accurate."""

    __gsignals__ = {'paint' : 'override', 'test' : 'override', 
            'calculate_bounds' : 'override'}

    path = None

    def __init__(self, **args):
        Item.__init__(self, **args)
        self.color = pick_color()
        self.alpha = random.uniform(.3, 1.0)
        self.line_width = 1.

    def build_path(self, c):
        c.new_path()

        r = random.uniform(10, 100)

        rand = lambda v : random.uniform(v-r, v+r)

        for i in range(0, 360, 45):
            x = r * math.cos(i * math.pi/180.)
            y = r * math.sin(i * math.pi/180.)
            if i == 0:
                c.move_to(x, y)
            else:
                c.curve_to(rand(lastx), rand(lasty), rand(x), rand(y), x, y)
            lastx, lasty = x, y
        c.close_path()
        self.path = c.copy_path()

    def do_paint(self, c):
        color = gdk.color_parse(self.color)
        c.set_source_rgba(color.red/65535., color.green/65535., 
                color.blue/65535., self.alpha)
        c.new_path()
        c.append_path(self.path)
        c.fill_preserve()
        c.set_source_rgb(0, 0, 0)
        c.set_line_width(self.line_width)
        c.stroke()
    def do_calculate_bounds(self, c, bounds, device_bounds):
        if not self.path: self.build_path(c)
        else:
            c.new_path()
            c.append_path(self.path)
        c.set_line_width(self.line_width)
        bounds.x1, bounds.y1, bounds.x2, bounds.y2 =  c.stroke_extents()
        return True
    def do_test(self, c, x, y):
        c.new_path()
        c.append_path(self.path)
        return c.in_fill(x, y) and self or None

class Thing(Path):
    """When constructing an item that is based on a cairo_path_t, there is no
    need to go through the trouble described above.  The Path item 
    does what is shown above and only requires that the make-path virtual
    method be overridden.
    """
    __gsignals__ = {'make_path' : 'override' }
    need_path = True

    def __init__(self, **args):
        Path.__init__(self, **args)
        self.props.fill_color = pick_color()
        self.props.outline_color = pick_color()
        m = self.props.matrix
        m.scale(random.uniform(1, 5), random.uniform(1, 5))
        self.props.matrix = m
    def do_make_path(self, c):
        if self.need_path:
            c.new_path()
            c.text_path('thing')
            return True
        return False

def make_blob2(**args):
    """This shows how to create a arbitrary custom canvas item using functional
    programming techniques."""

    path = []
    color = pick_color()
    alpha = random.uniform(.3, 1.0)

    def build_path(c):
        c.new_path()

        r = random.uniform(10, 100)

        for i in range(0, 405, 45):
            x = r * math.cos(i * math.pi/180.)
            y = r * math.sin(i * math.pi/180.)
            v = random.uniform(-r/3, r/3)
            if i == 0:
                c.move_to(x, y)
            else:
                c.curve_to(v, v, v, v, x, y)
        c.close_path()
        path.append(c.copy_path())
    def on_paint(item, c):
        col = gdk.color_parse(color)
        c.set_source_rgba(col.red/65535., col.green/65535.,
                col.blue/65535., alpha)
        c.new_path()
        c.append_path(path[0])
        c.fill_preserve()
        c.set_source_rgb(0, 0, 0)
        c.set_line_width(item.line_width)
        c.stroke()
    def on_calculate_bounds(item, c, bounds, device_bounds):
        if not path: build_path(c)
        else:
            c.new_path()
            c.append_path(path[0])
        c.set_line_width(item.line_width)
        bounds.x1, bounds.y1, bounds.x2, bounds.y2 =  c.stroke_extents()
        return True
    def on_test(item, c, x, y):
        c.new_path()
        c.append_path(path[0])
        return c.in_fill(x, y) and item or None

    item = Item(**args)
    item.connect('paint', on_paint)
    item.connect('calculate_bounds', on_calculate_bounds)
    item.connect('test', on_test)
    item.line_width = 1

    return item

def make_face(**args):
    """The easiest way to make custom items is through composition using
    pre-existing stock canvas items."""

    outline = Ellipse(**args)
    d = random.uniform(10, 80)
    outline.set(width = d, height = d, test_fill = True,
            outline_color = 'black', line_width = 1)

    r = d/2

    outline.add(Ellipse(width = d*.2, height = d*.2, x = -r*.4, y = -r*.4, 
        fill_color = 'blue'))
    outline.add(Ellipse(width = d*.2, height = d*.2, x = r*.4, y = -r*.4, 
        fill_color = 'blue'))
    outline.add(Ellipse(width = d*.2, height = d*.2, fill_color = 'green'))

    points = []
    for a in range(30, 160, 10):
        points += [r*.6*math.cos(a*math.pi/180), r*.6*math.sin(a*math.pi/180)]
    outline.add(Line(points = points, outline_color = 'red', line_width = 3, 
        close = False))

    return outline


def add_item(w, root):
    last_msec = [0]

    def set_line_width(item, width):
        if hasattr(item.props, 'line_width'):
            item.props.line_width = width
        else:
            item.line_width = width
            item.request_update()

    def on_event(item, event, ct, pick_item, root):
        state = False

        if event.type == gdk.BUTTON_PRESS:
            if event.button == 1:
                item.x, item.y = event.x, event.y
                item.dragging = True
            elif event.button == 3:
                root.remove(item)
            state = True
        elif event.type == gdk.MOTION_NOTIFY:
            if abs(event.time - last_msec[0]) < 100: return False
            last_msec[0] = event.time

            if item.dragging and event.state & gdk.BUTTON1_MASK:
                new_x, new_y = event.x, event.y
                m = item.matrix
                m.translate(new_x - item.x, new_y - item.y)
                item.matrix = m
                state = True
        elif event.type == gdk.ENTER_NOTIFY:
            set_line_width(item, 3)
            item.dragging = False
            state = True
        elif event.type == gdk.LEAVE_NOTIFY:
            set_line_width(item, 1)
            state = True
        return state

    construct = random.choice([Blob1, Thing, make_blob2, make_face])
    item = construct(x = random.uniform(20, 380), y = random.uniform(20, 380))
    item.connect('event', on_event, root)
    root.add(item)

window = gtk.Window()
window.connect('destroy', gtk.main_quit)

vbox = gtk.VBox()
window.add(vbox)

label = gtk.Label("Drag - move object.\nRight click - delete object")
vbox.pack_start(label)

canvas = Canvas()
canvas.set_size_request(400, 400)
vbox.pack_start(canvas)

hbox = gtk.HBox()
vbox.pack_start(hbox)

button = gtk.Button("Add an object")
button.connect("clicked", add_item, canvas.props.root)
hbox.add(button)

button = gtk.Button("Quit")
button.connect("clicked", gtk.main_quit)
hbox.add(button)

window.show_all()
gtk.main()
