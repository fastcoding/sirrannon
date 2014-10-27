import gtk
from base import SirannonBase

PALETTE_BORDER_WIDTH = 3

class SirannonLibrary(SirannonBase):
    def __init__(self, ctx):
        SirannonBase.__init__(self, ctx)
        self.registerWidgets('libDesc', 'libTable', 'libPalette')
        
        config = self.ctx.getConfig()
        templates = config.getTemplates()      
        self.dLib = {}
        
        abstracts = [item for item in sorted(templates, key=lambda x: x.getAttribute('name')) if item.getAttribute('abstract').lower() == 'true']
        for abstract in abstracts:                        
            type = abstract.getAttribute('name')            
            group = gtk.ToolItemGroup(type)
            group.set_collapsed(True)
            self.libPalette.add(group)
            
            components = [item for item in sorted(templates, key=lambda x: x.getAttribute('name')) if item.getAttribute('type') == type]
            for j, component in enumerate(sorted(components)):
                name = component.getAttribute('name')
                
                item = gtk.ToolItem()
                group.insert(item, -1)
                button = gtk.Button(label=name)
                button.connect("clicked", self.componentSelected, name)
                item.add(button)
                
                params = config.getTemplateParams(name)
                self.dLib[name] = component, params
            group.show_all()               

    
    def componentSelected(self, button, name):
        config = self.ctx.getConfig()        
        component, params = self.dLib[name]        
        for child in self.libTable.get_children():
            self.libTable.remove(child)
            
        label = gtk.Label()
        label.set_markup('<b><big>Name</big></b>')
        self.libDesc.attach(label, 0, 1, 0, 1)
        label = gtk.Label()
        label.set_markup('<b><big>Type</big></b>')
        self.libDesc.attach(label, 0, 1, 1, 2)
        label = gtk.Label()
        label.set_markup('<b><big>Properties</big></b>')
        self.libDesc.attach(label, 0, 1, 2, 3)
        label = gtk.Label()
        label.set_markup('<b><big>Description</big></b>')
        self.libDesc.attach(label, 0, 1, 3, 4)
        entry1 = gtk.Entry()
        entry1.set_text(name)
        entry1.set_editable(False)
        self.libDesc.attach(entry1, 1, 2, 0, 1)
        entry2 = gtk.Entry()
        entry2.set_text(component.getAttribute('type'))
        entry2.set_editable(False)
        self.libDesc.attach(entry2, 1, 2, 1, 2)
        entry3 = gtk.Entry()
        entry3.set_text('N/A')
        entry3.set_editable(False)
        self.libDesc.attach(entry3, 1, 2, 2, 3)        
        text = gtk.TextView()
        text.get_buffer().set_text(component.getAttribute('desc'))
        text.set_wrap_mode(gtk.WRAP_WORD)
        text.set_editable(False)
        self.libDesc.attach(text, 1, 2, 3, 4)
        self.colorize(0, entry1, entry2, entry3, text)
        self.libDesc.show_all()

        label = gtk.Label()
        label.set_markup('<b><big>Name</big></b>')
        self.libTable.attach(label, 0, 1, 0, 1)
        label = gtk.Label(str='Type')
        label.set_markup('<b><big>Type</big></b>')
        self.libTable.attach(label, 1, 2, 0, 1)
        label = gtk.Label(str='Default')
        label.set_markup('<b><big>Default</big></b>')
        self.libTable.attach(label, 2, 3, 0, 1)
        label = gtk.Label(str='Description')
        label.set_markup('<b><big>Description</big></b>')
        self.libTable.attach(label, 3, 4, 0, 1)
            
        for i, param in enumerate(params):
            label1 = gtk.Entry()
            label1.set_text(param.getAttribute('name'))
            label1.set_editable(False)
            self.libTable.attach(label1, 0, 1, i+1, i+2)
            label2 = gtk.Entry()
            label2.set_text(param.getAttribute('type'))
            label2.set_editable(False)
            self.libTable.attach(label2, 1, 2, i+1, i+2)
            label3 = gtk.Entry()
            label3.set_text(param.getAttribute('default'))
            label3.set_editable(False)
            self.libTable.attach(label3, 2, 3, i+1, i+2)
            text = gtk.TextView()
            text.get_buffer().set_text(param.getAttribute('desc'))
            text.set_wrap_mode(gtk.WRAP_WORD)
            text.set_editable(False)
            self.colorize(i, label1, label2, label3, text)
            self.libTable.attach(text, 3, 4, i+1, i+2)
            
        self.libTable.show_all()
        
    def colorize(self, index, *items):
        colors = ('Light Gray', 'Light Steel Blue')
        for i, item in enumerate(items):
            color = colors[(index+i)%2]
            item.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse(color))        
        