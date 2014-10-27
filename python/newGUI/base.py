import gtk

class SirannonError(Exception):
    def __init__(self):
        Exception.__init__(self)
        
class SirannonBase:
    def __init__(self, ctx):
        self.ctx = ctx
        
    def save(self):
        pass
    
    def load(self):
        pass     
        
    def registerWidgets(self, *names):
        for name in names:
            setattr(self, name, self.ctx.getWidget(name))
            
    def registerEvents(self):
        event_dict = dict( [ (sattr, getattr(self, sattr)) 
                            for sattr in self.__class__.__dict__ if sattr.find('event') == 0 ] )
        self.ctx.signal_autoconnect(event_dict)
        
class SirannonPalette(gtk.ToolPalette):
    def __init__(self):
        gtk.ToolPalette.__init__(self)
        self.__groups = {}
        
    def addExpander(self, name, *choices, **kwargs):
        options = {}
        group = gtk.ToolItemGroup(name)
        self.add(group)
        group.set_collapsed(True)        
        item = gtk.ToolItem()
        group.insert(item, -1)
        group.child_set_property(item, 'new-row', False)
        group.child_set_property(item, 'expand', False)        
        table = gtk.Table()
        table.resize(len(choices), 2)
        table.set_homogeneous(True)
        item.add(table)
        radio_group = None
        for i, (choice, mode) in enumerate(choices):            
            entry = None
            radio = gtk.RadioButton(group=radio_group, label=choice)
            if not mode:                                
                table.attach(radio, 0, 2, i, i+1)
            else:
                if mode == 'entry':
                    entry = gtk.Entry()
                else:
                    entry = gtk.FileChooserButton('Select a file...')
                table.attach(radio, 0, 1, i, i+1)
                table.attach(entry, 1, 2, i, i+1)
            options[choice] = entry         
            if not radio_group:
                radio_group = radio
        self.show_all()
        self.__groups[name] = radio_group, options
        if 'expanded' in kwargs:
            group.set_collapsed(not kwargs['expanded'])
        return group
        
    def getChoice(self, name=None):
        if not name:
            name = self.__groups.keys()[0]
        group, options = self.__groups[name]
        for radio in group.get_group():
            if radio.get_active():
                choice = radio.get_label()
                entry = options[choice]
                if isinstance(entry, gtk.Entry):
                    return choice, entry.get_text()
                elif isinstance(entry, gtk.FileChooserButton):
                    return choice, entry.get_filename()
                else:
                    return choice, choice
