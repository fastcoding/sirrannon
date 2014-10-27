import pygtk
pygtk.require("2.0")
import gtk
import cairo
import crcanvas
import math
import util
import gobject
import pango
from base import SirannonBase

# Constannts
MODE_RESIZE = 0
MODE_ROTATE = 1
MODE_CONNECT = 2
MODE_MOVE = 3
ARROW_H = 0
ARROW_V = 1
ARROWS = {}
SOUTH, WEST, NORTH, EAST = 0, 1, 2, 3

# Form factors
MODEL_DEFAULT_WIDTH = 208
MODEL_DEFAULT_HEIGHT = 117
MODEL_BORDER_COLOR = 'black'
MODEL_COLOR = 'grey'
MODEL_TEXT_COLOR = 'black'
MODEL_BORDER_NORMAL = 1
MODEL_BORDER_HIGHLIGHT = 3
TEXT_PADDING = 5
CONTROLLER_DEFAULT = 6
CONTROLLER_COLOR = 'red'
CONTROLLER_WIDTH = 7
CONTROLLER_HEIGHT = 7
CONTROLLER_BORDER_NORMAL = 1
CONTROLLER_COORDS = ((-1,-1), (-1,1), (1,-1), (1,1), (0,-1), (1,0), (0,1), (-1,0), (0.8,0.8))
CONTROLLER_ORIENTATION = ( NORTH, NORTH, NORTH, NORTH, NORTH, EAST, SOUTH, WEST, NORTH )
CONTROLLER_FORM = (crcanvas.Rectangle, crcanvas.Rectangle, crcanvas.Rectangle, crcanvas.Rectangle, 
                   crcanvas.Rectangle, crcanvas.Rectangle, crcanvas.Rectangle, crcanvas.Rectangle,
                   crcanvas.Ellipse)

CONTROLLER_NORTH = 4
CONTROLLER_EAST = 5
CONTROLLER_SOUTH = 6
CONTROLLER_WEST = 7
CONTROLLER_COLOR = ('red', 'red', 'red', 'red', 'blue', 'blue', 'blue', 'blue', 'green')
CONTROLLER_MODE = (MODE_RESIZE, MODE_RESIZE, MODE_RESIZE, MODE_RESIZE,
                     MODE_CONNECT, MODE_CONNECT, MODE_CONNECT, MODE_CONNECT,
                     MODE_ROTATE)
ARROW_COLOR = 'blue'
ARROW_WIDTH = 2
ARROW_WIDTH_HIGHLIGHT = 3
ARROW_LENGTH = CONTROLLER_HEIGHT
ARROW_HEAD_WIDTH = 50
ARROWS[ARROW_H,ARROW_H] = ((0.0,0.0), (0.5,0.0), (0.5,1.0), (1.0,1.0))
ARROWS[ARROW_H,ARROW_V] = ((0.0,0.0), (1.0,0.0), (1.0,1.0))
ARROWS[ARROW_V,ARROW_V] = ((0.0,0.0), (0.0,0.5), (1.0,0.5), (1.0,1.0))
ARROWS[ARROW_V,ARROW_H] = ((0.0,0.0), (0.0,1.0), (1.0,1.0))

PROPERTIES_YPAD = 5
PROPERTIES_XPAD = 3

class SirannonArrow(SirannonBase):    
    def __init__(self, ctx, widget):
        SirannonBase.__init__(self, ctx)
        self.registerEvents()
        self.widget = widget
        self.h0 = None
        self.h1 = None
        self.line = crcanvas.Line(line_width=ARROW_WIDTH, outline_color=ARROW_COLOR, close=False)
        self.widget.root.add(self.line)
        self.arrow = crcanvas.Arrow(point_id=-2, fatness=7, scaleable=True, angle=0, length=ARROW_LENGTH, depth=0, outline_color=ARROW_COLOR, line_width=ARROW_WIDTH)
        self.line.add(self.arrow)
        self.arrow.connect_parent(self.line)        
        self.line.connect('event', self.event)
    
    def event(self, widget, event, matrix, pick_item):       
        if event.type == gtk.gdk.ENTER_NOTIFY:
            self.line.props.line_width = ARROW_WIDTH_HIGHLIGHT                
        elif event.type == gtk.gdk.LEAVE_NOTIFY:
            self.line.props.line_width = ARROW_WIDTH
        elif event.type == gtk.gdk.BUTTON_PRESS:
            self.ctx.getProperties().show_route(self.begin.master.name, self.end.master.name)
        return False

    def clear(self):
        self.widget.root.remove(self.line)
        self.begin.disconnect(self.h0)
        if self.h1:
            self.end.disconnect(self.h1)
            
    def set_begin(self, controller):
        if self.h0:
            self.begin.disconnect(self.h0)
        self.begin = controller
        self.h0 = controller.connect("request-update", self.update)
    
    def set_end(self, controller):
        if self.h1:
            self.end.disconnect(self.h1)
        self.end = controller
        self.h1 = controller.connect("request-update", self.update)
        self.update_points(self.begin.props.x, self.begin.props.y, self.begin.master.getOrientation(self.begin),
                      self.end.props.x, self.end.props.y, self.end.master.getOrientation(self.end),
                      self.end.master.model.props.width, self.end.master.model.props.height)
        
    def set_refined(self, begin_component, end_component):
        w = begin_component.model.props.width                     
        dx = end_component.model.props.x - begin_component.model.props.x
        dy = end_component.model.props.y - begin_component.model.props.y
        h = begin_component.model.props.height
        
        # Ugly heuristic (if a component has more than one route from taper it out horizontally)
        taper = len(self.ctx.getConfig().getRoutesFrom(begin_component.name)) == 1        
        if dx > w:            
            if dy > h:
                if taper:
                    controllers = CONTROLLER_EAST, CONTROLLER_NORTH
                else:
                    controllers = CONTROLLER_SOUTH, CONTROLLER_WEST
            elif dy > -h:
                controllers = CONTROLLER_EAST, CONTROLLER_WEST
            else:
                if taper:
                    controllers = CONTROLLER_EAST, CONTROLLER_SOUTH
                else:
                    controllers = CONTROLLER_NORTH, CONTROLLER_WEST            
        elif dx > -w:
            if dy > h:
                controllers = CONTROLLER_SOUTH, CONTROLLER_NORTH
            elif dy > h:
                controllers = CONTROLLER_SOUTH, CONTROLLER_NORTH
            else:
                controllers = CONTROLLER_NORTH, CONTROLLER_SOUTH
        else:
            if dy > h:
                if taper:
                    controllers = CONTROLLER_WEST, CONTROLLER_NORTH
                else:
                    controllers = CONTROLLER_SOUTH, CONTROLLER_EAST
            elif dy > -h:
                controllers = CONTROLLER_WEST, CONTROLLER_EAST
            else:
                if taper:
                    controllers = CONTROLLER_WEST, CONTROLLER_SOUTH
                else:
                    controllers = CONTROLLER_NORTH, CONTROLLER_EAST
        
        begin, end = controllers
        self.set_begin(begin_component.controllers[begin])
        self.set_end(end_component.controllers[end])    
        
    def set_middle(self, x1, y1, m1):
        self.update_points(self.begin.props.x, self.begin.props.y, self.begin.master.getOrientation(self.begin), x1, y1, m1, MODEL_DEFAULT_WIDTH, MODEL_DEFAULT_HEIGHT)
    
    def update(self, event):
        self.update_points(self.begin.props.x, self.begin.props.y, self.begin.master.getOrientation(self.begin),
                           self.end.props.x, self.end.props.y, self.end.master.getOrientation(self.end),
                           self.end.master.model.props.width, self.end.master.model.props.height)
        
    def update_points(self, x0, y0, m0, x1, y1, m1, w1, h1):
        import arrow
        w0 = self.begin.master.model.props.width
        h0 = self.begin.master.model.props.height
        form = arrow.getPoints(x0, y0, m0, w0, h0, x1, y1, m1, w1, h1)
        d = (arrow.Vector(x1, y1) - arrow.Vector(x0, y0)).abs() # absolute difference vector
        points = []
        points.append(x0)
        points.append(y0)
        p = arrow.Vector(x0, y0)
        for a, b, r in form:            
            p = p + d * r * a + r * b            
            points.append(p.x)
            points.append(p.y)
        x2, y2 = points[-2], points[-1]
        u = (arrow.Vector(x2, y2) - arrow.Vector(x1, y1)).unitize()
        v = arrow.Vector(x1, y1) + u * ARROW_LENGTH
        points.append(v.x)
        points.append(v.y)        
        points.append(x1)
        points.append(y1)
        self.line.props.points = points
        
    def save(self, gfx):
        gfx.setAttribute('from', str(self.begin.id))
        gfx.setAttribute('to', str(self.end.id))

class SirannonComponent(SirannonBase):    
    def __init__(self, ctx, widget, name, type, x=0, y=0, w=MODEL_DEFAULT_WIDTH, h=MODEL_DEFAULT_HEIGHT, r=0.0):
        # Master
        SirannonBase.__init__(self, ctx)
        self.registerEvents()
        self.widget = widget
        self.name = name
        self.type = type
        self.sync = True

        # The model will be a rectangle.
        self.model = crcanvas.Rectangle(x=x, y=y, width=w, height=h, outline_color=MODEL_BORDER_COLOR, fill_color=MODEL_COLOR)
        self.text = crcanvas.Text(fill_color=MODEL_TEXT_COLOR, anchor=gtk.ANCHOR_CENTER, use_markup=True)
        self.model.add(self.text)
        self.text.props.text = '{0}\n(<i>{1}</i>)'.format(self.name, self.type)
        self.model.mode = MODE_MOVE
        self.model.id = -1
        self.model.connect('event', self.event)
        
        matrix = self.model.matrix
        matrix.rotate(r)
        self.model.matrix = matrix        
         
        # The controllers
        self.controllers = [form(width=CONTROLLER_WIDTH,
                                  height=CONTROLLER_HEIGHT,
                                  outline_color=color,
                                  line_width=CONTROLLER_BORDER_NORMAL,
                                  test_fill=True) \
                             for form, color in zip(CONTROLLER_FORM, CONTROLLER_COLOR)]       

        # Position the controllers
        for i, controller in enumerate(self.controllers):
            controller.id = i
            controller.mode = CONTROLLER_MODE[i]
            controller.orientation = CONTROLLER_ORIENTATION[i]
            controller.master = self
            controller.connect('event', self.event)
            
        # Check
        w, h = self.min_size()
        self.model.props.width = max(self.model.props.width, w)
        self.model.props.height = max(self.model.props.height, h)       
        self.update_controllers()
            
        # Add
        self.widget.canvas.props.root.add(self.model)
        for controller in self.controllers:           
            self.widget.canvas.props.root.add(controller)
            
    def rename(self, new):
        self.name = new
        self.text.props.text = '{0}\n(<i>{1}</i>)'.format(self.name, self.type)
        
    def clear(self):
        self.widget.root.remove(self.model)
        for controller in self.controllers:            
            self.widget.root.remove(controller)
            
    def bounds(self):
        x = self.model.props.x
        y = self.model.props.y
        w = self.model.props.width
        h = self.model.props.height        
        return x - w/2 - CONTROLLER_WIDTH, y - h/2 - CONTROLLER_HEIGHT, x + w/2 + CONTROLLER_WIDTH, y + h/2 + CONTROLLER_HEIGHT
    
    def get_size(self):
        return self.model.props.width, self.model.props.height
    
    def update_controllers(self):
        w, h = self.model.props.width, self.model.props.height
        for controller, (sx, sy) in zip(self.controllers, CONTROLLER_COORDS):            
            matrix = self.model.matrix
            matrix.translate(sx*w/2, sy*h/2)
            controller.matrix = matrix
            
    def set_model(self, x=None, y=None):
        if x is not None:
            self.model.props.x = x
        if y is not None:
            self.model.props.y = y
        self.update_controllers()

    def move_model(self, dx, dy):
        dx, dy = self.model.matrix.transform_distance(dx, dy)
        self.model.props.x += dx
        self.model.props.y += dy
        self.ctx.getCanvas().updateScrollRegion()
    
    def rotate_model(self, dx, dy):
        import math
        item = self.controllers[MODE_ROTATE]
        angle = math.atan2(item.props.y + dy, item.props.x + dx) - \
                math.atan2(item.props.y, item.props.x)
        matrix = self.model.matrix
        matrix.rotate(angle)
        self.model.matrix = matrix
        
    def resize_model(self, controller, dx, dy):
        dx, dy = self.model.matrix.transform_distance(dx, dy)        
        sx, sy = CONTROLLER_COORDS[controller.id]
        self.model.props.width += sx * dx
        self.model.props.height += sy * dy
        wmin, hmin = self.min_size()
        if self.model.props.width < wmin:
            self.model.props.width = wmin
        else:
            self.model.props.x += dx / 2
        if self.model.props.height < hmin:
            self.model.props.height = hmin
        else:
            self.model.props.y += dy / 2
    
    def match(self, x2, y2):
        for controller in self.controllers:
            x0, y0, x1, y1 = controller.get_bounds()
            x0, y0 = controller.matrix.transform_point(x0, y0)
            x1, y1 = controller.matrix.transform_point(x1, y1)
            if x0 <= x2 and x2 <= x1 and y0 <= y2 and y2 <= y1:
                return controller
        return None        

    def event(self, widget, event, matrix, pick_item):       
        if event.type == gtk.gdk.ENTER_NOTIFY:
            self.model.props.line_width = MODEL_BORDER_HIGHLIGHT
            
        elif event.type == gtk.gdk.BUTTON_PRESS:         
            widget.init_x = event.x
            widget.init_y = event.y
            self.ctx.getProperties().show_component(self.name)
        
        elif event.type == gtk.gdk.BUTTON_RELEASE:
            self.sync = True
        
        elif event.type == gtk.gdk.MOTION_NOTIFY:
            dx = event.x - widget.init_x
            dy = event.y - widget.init_y
            if widget.mode not in (MODE_MOVE, MODE_ROTATE, MODE_RESIZE):
                return False
            if self.sync:
                self.ctx.sync()
                self.sync = False
            if widget.mode == MODE_MOVE:
                self.move_model(dx, dy)
            elif widget.mode == MODE_ROTATE:
                self.rotate_model(dx, dy)
            elif widget.mode == MODE_RESIZE:            
                self.resize_model(widget, dx, dy)  
            self.update_controllers()
                
        elif event.type == gtk.gdk.LEAVE_NOTIFY:
            self.model.props.line_width = MODEL_BORDER_NORMAL
        return False
    
    def getOrientation(self, controller):
        import math, arrow
        xx, yx, xy, yy, tx, ty = self.model.matrix
        angle = math.atan2(xy, xx)
        shift = - angle / math.pi * 2. - 0.5
        return arrow.rot(controller.orientation, math.ceil(shift)) 
    
    def save(self, gfx):
        gfx.setAttribute('x', str(self.model.props.x))
        gfx.setAttribute('y' ,str(self.model.props.y))
        gfx.setAttribute('w', str(self.model.props.width))
        gfx.setAttribute('h', str(self.model.props.height))
        gfx.setAttribute('r', str(math.acos(self.model.matrix.transform_distance(1, 0)[0])))        
       
    def min_size(self):
        local = crcanvas.Bounds()
        device = crcanvas.DeviceBounds()
        self.text.calculate_bounds(local, device)
        w = local.x2 - local.x1
        h = local.y2 - local.y1        
        return w + TEXT_PADDING, h + TEXT_PADDING
    
    def get_angle(self):
        xx, yx, xy, yy, tx, ty = self.model.matrix
        return math.atan2(xy, xx)
    
class SirannonProperties(SirannonBase):  
    def __init__(self, ctx, widget):
        SirannonBase.__init__(self, ctx)
        self.registerEvents()
        self.widget = widget
        self.name = ()
        self.button = None     
    
    def get_value(self, obj):
        if isinstance(obj, gtk.ComboBox):
            if obj.get_active():
                return 'True'
            else:
                return 'False'
        else:
            return obj.get_text()
        
    def clear(self):
        for child in self.widget.get_children():
            self.widget.remove(child)
        
    def getObj(self):
        if len(self.name) == 1:
            return self.ctx.getConfig().getComponent(*self.name)
        else:
            return self.ctx.getConfig().getRoute(*self.name)
            
    def show_component(self, name):
        self.save()
        self.clear()
        config = self.ctx.getConfig()
        component = config.getComponent(name)
        self.name = (name,)
        type = component.getAttribute('type')
        template_params = util.dictify(config.getTemplateParams(type))        
        params = config.getParams(component)
        self.widget.resize(len(params)+4, 2)
        
        i = 0        
        label = gtk.Label('name')
        label.set_alignment(0.,0.5)
        self.widget.attach(label, 0, 1, i, i+1, ypadding=PROPERTIES_YPAD)                
        self.button = gtk.Button()
        self.button.set_label(name)
        self.button.connect('clicked', self.eventRename)
        self.widget.attach(self.button, 1, 2, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)
        i += 1
        
        label = gtk.Label('type')
        label.set_alignment(0.,0.5)
        self.widget.attach(label, 0, 1, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)                
        entry = gtk.Button()
        entry.set_label(type)
        self.widget.attach(entry, 1, 2, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)
        i += 1
        
        sep = gtk.HSeparator()
        self.widget.attach(sep, 0, 2, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)
        i += 1
        
        for param in params:            
            name = param.getAttribute('name')
            ptype = param.getAttribute('type')
            str = '{0} ({1})'.format(name, ptype)
            label = gtk.Label(str)
            label.set_alignment(0.,0.5)
            #label.set_ellipsize(pango.ELLIPSIZE_END)
            if name in template_params:
                label.set_tooltip_text(template_params[name].getAttribute('desc'))
            else:
                label.set_tooltip_text('No description available')
            self.widget.attach(label, 0, 1, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)                
            if ptype == 'bool':
                entry = gtk.combo_box_new_text()                
                entry.append_text('False')
                entry.append_text('True')
                val = util.bool(param.getAttribute('val'))
                if val:
                    entry.set_active(1)
                else:
                    entry.set_active(0)
                entry.connect('changed', self.eventEdit, name)
            else:
                entry = gtk.Entry()
                entry.set_alignment(0.)
                entry.set_text(param.getAttribute('val'))
                entry.connect('changed', self.eventEdit, name)
            
            self.widget.attach(entry, 1, 2, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)
            i += 1
            
        sep = gtk.HSeparator()
        self.widget.attach(sep, 0, 2, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)
        i += 1
            
        self.desc = gtk.TextView()
        self.desc.set_wrap_mode(gtk.WRAP_WORD)
        str = self.ctx.getConfig().getComponentDesc(type)
        if not str:
            str = 'No description available.'
        self.desc.get_buffer().set_text(str)
        self.desc.set_editable(False)
        self.widget.attach(self.desc, 0, 2, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD )
        
        self.widget.show_all()
        self.widget.set_reallocate_redraws(True)
        gobject.timeout_add( 100, self.widget.check_resize)
            
    def show_route(self, begin, end):
        self.save()
        self.clear()
        component = self.ctx.getConfig().getRoute(begin, end)
        self.name = (begin, end)
        self.widget.resize(3, 2)
        
        for i,str in enumerate(('from', 'to')):
            label = gtk.Label(str)
            label.set_alignment(0.,0.5)
            self.widget.attach(label, 0, 1, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)                
            entry = gtk.Entry()
            entry.set_alignment(0.)
            entry.set_text(component.getAttribute(str))
            entry.editable = False
            self.widget.attach(entry, 1, 2, i, i+1, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)
        
        label = gtk.Label('xroute')
        label.set_alignment(0.,0.5)
        self.widget.attach(label, 0, 1, 2, 3, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)                
        entry = gtk.Entry()
        entry.set_text(component.getAttribute('xroute'))
        entry.connect('changed', self.eventEditRoute)
        self.widget.attach(entry, 1, 2, 2, 3, xpadding=PROPERTIES_XPAD, ypadding=PROPERTIES_YPAD)
        self.widget.show_all()
        
    def load(self):
        self.clear()        
        # Restore previous
        if len(self.name) == 2:
            route = self.ctx.getConfig().getRoute(*self.name)
            if route:
                self.show_route(*self.name)
        elif len(self.name) == 1:
            component = self.ctx.getConfig().getComponent(*self.name)
            if component:
                self.show_component(*self.name)
       
    def save(self):
        pass
            
    def eventClear(self, widget):
        pass
    
    def eventDelete(self, widget):        
        if self.name:
            self.ctx.sync()
            if len(self.name) == 1:
                self.ctx.getCanvas().removeComponent(*self.name)
            else:
                self.ctx.getCanvas().removeRoute(*self.name)
            self.clear()
        return True
    
    def eventEdit(self, widget, name):       
        config = self.ctx.getConfig()
        param = config.getParam(self.getObj(), name)
        new = self.get_value(widget)
        old = param.getAttribute('val')
        if new != old:
            config.sync() # CAVEAT INVALIDATES CURRENT PARAM
            param = config.getParam(self.getObj(), name)
            param.setAttribute('val', new)
            
    def eventEditRoute(self, widget):
        config = self.ctx.getConfig()
        old = self.getObj().getAttribute('xroute')
        new = widget.get_text()
        if new != old:
            config.sync()
            self.getObj().setAttribute('xroute', new)
    
    def eventRename(self, widget):
        dialog = self.ctx.getWidget('RenameDialog')
        dialog.set_markup('New name for the component.')
        entry = self.ctx.getRenameEntry()
        old = self.name[0]
        entry.set_text(old)
        entry.show()
        while(True):
            status = dialog.run()
            dialog.hide()
            if status == gtk.RESPONSE_OK:
                new =  entry.get_text()
                if new != old:
                    if self.ctx.getCanvas().renameComponent(old, new) < 0:
                        dialog.set_markup('Name already exists. Choose a new unique name for the component.')
                        continue
                    else:
                        self.button.set_label(new)
                        self.button.pressed()
                        self.button.released()
                        self.name = (new,)
            break
        
class SirannonMenu(SirannonBase):
    def __init__(self, ctx):
        SirannonBase.__init__(self, ctx)
        self.registerEvents()
        self.menu = gtk.Menu()
        self.reload()
        
    def reload(self):
        for child in self.menu.get_children():
            self.menu.remove(child)
            
        templates = self.ctx.getConfig().getTemplates()
        abstracts = [item for item in sorted(templates, key=lambda x: x.getAttribute('name')) if item.getAttribute('abstract').lower() == 'true']
        for i, abstract in enumerate(abstracts):
            type = abstract.getAttribute('name')
            if type == 'core': # Do not display core
                continue
            item = gtk.MenuItem(label=type)
            submenu = gtk.Menu()
            item.set_submenu(submenu)
            self.menu.attach(item, 0, 1, i, i+1)
            components = [item for item in sorted(templates, key=lambda x: x.getAttribute('name')) if item.getAttribute('type') == type]
            for j, component in enumerate(sorted(components)):
                name = component.getAttribute('name')
                item = gtk.MenuItem(label=name)
                submenu.attach(item, 0, 1, j, j+1)
                item.connect('activate', self.newComponent, name)
        self.menu.show_all()
                
    def newComponent(self, widget, type):
        self.ctx.getCanvas().createComponent(type)

    def eventNewComponent(self, widget):
        self.menu.popup(None, None, None, 0, 0)
        
    def show(self):
        self.menu.popup(None, None, None, 0, 0)
        
class SirannonCanvas(SirannonBase):
    def __init__(self, ctx, master, widget):
        SirannonBase.__init__(self, ctx)
        self.registerEvents()
        self.master = master
        self.widget = widget
        self.components = {}
        self.arrows = {}
        self.line = None        
        self.canvas = crcanvas.Canvas(auto_scale=True, maintain_center=True, maintain_aspect=True, show_less=False)
        self.widget.add(self.canvas)
        self.root = self.canvas.props.root
        self.root.connect('event', self.eventClickComponent)
        self.canvas.connect('event', self.eventClickCanvas)
        #self.canvas.set_repaint_mode(True)
        self.fresh = True
        self.canvas.set_max_scale_factor(5.0, 5.0)
        self.canvas.set_min_scale_factor(0.2, 0.2)        
        
    def bounds(self):
        # True bounds
        xmin, ymin = float("inf"), float("inf")
        xmax, ymax = -float("inf"), -float("inf")       
        for component in self.components.values():
            x1, y1, x2, y2 = component.bounds()
            if x1 < xmin: xmin = x1
            if y1 < ymin: ymin = y1
            if x2 > xmax: xmax = x2
            if y2 > ymax: ymax = y2            
            
        # Convert into centered coords
        w = xmax - xmin
        h = ymax - ymin
        cx = (xmin + xmax) * .5
        cy = (ymin + ymax) * .5
               
        # Force into widget aspect
        x, y, w_out, h_out = self.widget.get_allocation()
        scale = float(w_out) / float(h_out) * h / w
        if scale > 1.0:
            w *= scale
        else:
            h /= scale
            
        # Add some padding
        w *= 1.2
        h *= 1.2
        return cx, cy, w, h
    
    def eventAlignVer(self, button):
        if self.components:
            self.ctx.sync()
            xtot = sum([c.model.props.x for c in self.components.itervalues()])
            xtot /= len(self.components)        
            for c in self.components.itervalues():
                c.set_model(x=xtot)
            self.canvas.queue_repaint()
        return True
    
    def eventAlignHor(self, button):
        if self.components:
            self.ctx.sync()
            ytot = sum([c.model.props.y for c in self.components.itervalues()])
            ytot /= len(self.components)        
            for c in self.components.itervalues():
                c.set_model(y=ytot)
            self.canvas.queue_repaint()
        return True
    
    def eventZoomNormal(self, widget):
        if not self.components:
            return
        cx, cy, w, h = self.bounds()
        self.canvas.center_scale(cx, cy, w, h)        
        self.canvas.set_scroll_region(cx-2*w, cy-2*h, cx+2*w, cy+2*h)

    def eventZoomIn(self, widget):        
        self.canvas.zoom(1.5, 0.)
        
    def eventZoomOut(self, widget):
        self.canvas.zoom(2./3., 0.)
        
    def updateScrollRegion(self):
        cx, cy, w, h = self.bounds()
        x1, y1, x2, y2 = self.canvas.get_viewport()
        w = max(w, x2-x1)
        h = max(h, y2-y1)
        self.canvas.set_scroll_region(cx-2*w, cy-2*h, cx+2*w, cy+2*h)
        
    def eventClickCanvas(self, widget, event):
        if event.type == gtk.gdk.BUTTON_PRESS and event.button == 3:
            print 'Right click'
            self.master.getMenu().show()
        
    def eventClickComponent(self, widget, event, matrix, pick_item):            
        if event.type == gtk.gdk.BUTTON_PRESS:
            if hasattr(pick_item, 'mode') and pick_item.mode == MODE_CONNECT:
                self.begin_line(pick_item, event)
            print pick_item
            if not pick_item:
                print 'Empty'
            if pick_item == self.canvas:
                print 'Slef'      
        elif event.type == gtk.gdk.MOTION_NOTIFY:
            if self.line:
                self.move_line(event)        
        elif event.type == gtk.gdk.BUTTON_RELEASE:
            if self.line:
                self.end_line(event)                
        return False
    
    def clear(self):
        for component in self.components.values():
            component.clear()
        self.components.clear()
        for arrow in self.arrows.values():
            arrow.clear()
        self.arrows.clear()
        self.canvas.queue_repaint()
        
    def begin_line(self, controller, event):
        self.begin, controller = self.match(event.x, event.y)
        if not controller:
            return
        self.line = SirannonArrow(self.ctx, self)
        self.line.set_begin(controller)
        
    def move_line(self, event):
        import arrow
        e = arrow.Vector(event.x, event.y)
        d_min = 1.e+99
        closest = None
        # Determine the controller orientation based on the closed controller
        for component in self.components.values():
            for controller in component.controllers:
                d = (arrow.Vector(controller.props.x, controller.props.y) - e).norm()
                if controller.mode != MODE_CONNECT:
                    continue
                if d < d_min:
                    closest = controller
                    d_min = d
                    
        if closest == self.line.begin or d_min > 10000.:
            # We are near the original, take its opposite
            o = arrow.flip(closest.orientation)
            self.line.set_middle(event.x, event.y, o)
        else:
            self.line.set_middle(event.x, event.y, closest.orientation)
        
    def end_line(self, event):
        self.end, controller = self.match(event.x, event.y)
        if controller and controller.mode == MODE_CONNECT:
            if (self.begin.name, self.end.name) in self.arrows:
                self.line.clear()
            elif self.begin.name == self.end.name:
                self.line.clear()
            else:
                self.line.set_end(controller)
                self.createRoute(self.begin.name, self.end.name, self.line)
        else:
            self.line.clear()
        self.line = None
    
    def load(self):
        self.clear()
        print 'Load canvas'      
        
        self.old = False
        for component in self.ctx.getConfig().getElements('component'):
            self.drawComponent(component)
            
        for route in self.ctx.getConfig().getElements('route'):
            self.drawRoute(route)
            #util.warning('Ignoring invalid route')
            
        if self.old:
            #util.warning(self.ctx, 'Upgraded XML to new GUI')
            self.ctx.getConfig().forceDirty()
        if self.fresh:
            gobject.timeout_add(100, self.eventZoomNormal, None)
        self.fresh = False  
        self.canvas.show_all()
    
    def save(self):
        for name in self.components.keys():            
            component = self.ctx.getConfig().getComponent(name)
            gfx = self.ctx.getConfig().getChild(component, 'gfx')
            self.components[name].save(gfx)
            
        for begin, end in self.arrows.keys():            
            route = self.ctx.getConfig().getRoute(begin, end)
            gfx = self.ctx.getConfig().getChild(route, 'gfx')
            self.arrows[begin, end].save(gfx)
            
        self.ctx.getProperties().save()

    def match(self, x, y):
        for component in self.components.values():
            controller = component.match(x,y)
            if controller:
                return component, controller
        return None, None

    def renameComponent(self, name, new):
        if new in self.components.keys():
            return -1
        self.ctx.sync()
        component = self.components[name]
        del self.components[name]
        self.components[new] = component
        xml_component = self.ctx.getConfig().getComponent(name)
        xml_component.setAttribute('name', new)
        component.rename(new)
        
        for (begin, end), route in self.arrows.iteritems():
            if begin == name:
                del self.arrows[begin, end]
                self.arrows[new, end] = route
                xml_route = self.ctx.getConfig().getRoute(begin, end)
                xml_route.setAttribute('from', new)
            elif end == name:
                del self.arrows[begin, end]
                self.arrows[begin, new] = route
                xml_route = self.ctx.getConfig().getRoute(begin, end)
                xml_route.setAttribute('to', new)
        return 0
    
    def removeComponent(self, name):
        # Remove from GUI
        component = self.components[name]
        component.clear()
        del self.components[name]
        
        # Remove from config
        config = self.ctx.getConfig()        
        xml_component = config.getComponent(name)
        config.delElement(xml_component)
        
        # Clear arrows
        for begin, end in self.arrows.keys():
            if begin == name or end == name:
                self.removeRoute(begin, end)                
        self.canvas.queue_repaint()
                
    def removeRoute(self, begin, end):
        # Remove from GUI
        arrow = self.arrows[begin, end]
        arrow.clear()
        del self.arrows[begin, end]
        
        # Remove from config
        config = self.ctx.getConfig()        
        xml_route = config.getRoute(begin, end)
        config.delElement(xml_route)
        self.canvas.queue_repaint()

    def drawComponent(self, component):     
        # Dimensions
        gfxs = component.getElementsByTagName("gfx")
        if not gfxs:
            x, y, w, h, r = 0., 0., MODEL_DEFAULT_WIDTH, MODEL_DEFAULT_HEIGHT, 0.
            self.old = True            
        else:
            gfx = gfxs[0]
            x = float(gfx.getAttribute("x"))
            y = float(gfx.getAttribute("y"))
            w = float(gfx.getAttribute("w"))
            h = float(gfx.getAttribute("h"))
            r = 0.
            if gfx.hasAttribute("r"):
                r = float(gfx.getAttribute("r"))
            else:
                x += w/2
                y += h/2
        name = str(component.getAttribute("name"))
        type = str(component.getAttribute("type"))
        
        # Draw
        self.components[name] = SirannonComponent(self.ctx, self, name, type, x, y, w, h, r)
        
    def drawRoute(self, route):
        begin = str(route.getAttribute('from'))
        end = str(route.getAttribute('to'))
        begin_component = self.components[begin]
        end_component = self.components[end]
        xroute = int(route.getAttribute('xroute'))        
        gfx = route.getElementsByTagName("gfx")        
        arrow = SirannonArrow(self.ctx, self)
        
        if gfx:
            begin_controller_id = int(gfx[0].getAttribute('from'))
            end_controller_id = int(gfx[0].getAttribute('to'))
            arrow.set_begin(self.components[begin].controllers[begin_controller_id])
            arrow.set_end(self.components[end].controllers[end_controller_id])
        else:
            arrow.set_refined(begin_component, end_component)
            self.old = True
        
        self.arrows[begin, end] = arrow
        
    def createRoute(self, begin, end, arrow):
        self.ctx.sync()
        self.arrows[begin, end] = arrow        
        self.ctx.getConfig().createRoute(begin, end, 0)
        
    def createComponent(self, type):
        self.ctx.sync()
        screen, x, y, mask = gtk.gdk.display_get_default().get_pointer()
        x0, y0, w, h = self.canvas.get_allocation()
        config = self.ctx.getConfig()
        matrix = self.root.matrix
        matrix.invert()
        x, y = matrix.transform_point(x - x0, y - y0)
        name = config.uniqueName(type)
        self.components[name] = SirannonComponent(self.ctx, self, name, type, x, y)
        config.createComponent(name, type)

class SirannonDraw(SirannonBase):
    def __init__(self, ctx, canvas_widget, properties_widget):
        SirannonBase.__init__(self, ctx)
        self.registerEvents()
        self.__props = SirannonProperties(ctx, properties_widget)
        self.__menu = SirannonMenu(ctx)
        self.__canvas = SirannonCanvas(ctx, self, canvas_widget)
        
    def getMenu(self):
        return self.__menu
    
    def getProperties(self):
        return self.__props 
    
    def getCanvas(self):
        return self.__canvas
        
    def save(self):
        self.__canvas.save()
        self.__props.save()
        
    def load(self):
        self.__canvas.load()
        self.__props.load()
        