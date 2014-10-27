import gtk
import gobject
import os.path

from base import SirannonBase
import construct
import canvas
import editor
import command
import library

class SirannonTab(SirannonBase):
    def __init__(self, master, config, gladefile, notebook):
        SirannonBase.__init__(self, self)        
        self.__config = config
        self.__master = master
        self.__tree = gtk.glade.XML(gladefile, 'Tabs')
        
        # Tab label
        box = gtk.HBox()
        self.__label = gtk.Label(str=self.__config.getName())
        box.pack_start(self.__label, False, False, 0)
        button = gtk.Button()
        box.pack_start(button, False, False, 0)
        image = gtk.image_new_from_stock(gtk.STOCK_CLOSE, gtk.ICON_SIZE_MENU)
        button.add(image)
        button.connect("clicked", self.__close)
        button.set_relief(gtk.RELIEF_NONE)
        box.show_all()
        
        # Tab notebook
        self.__notebook = self.__tree.get_widget('Tabs')
        self.__notebook.show_all()
        
        # New page
        self.__page = notebook.append_page(self.__notebook, box)

        # Controller classes
        self.__draw = canvas.SirannonDraw(self, self.getWidget("Canvas"), self.getWidget("Properties")) 
        self.editor = editor.SirannonEditor(self, self.getWidget("XMLEditor"))
        self.run = command.SirannonCommand(self)
        self.construct = construct.SirannonConstruct(self)
        self.library = library.SirannonLibrary(self)
        self.__tabs = (self.construct, self.__draw, self.editor, self.run, self.library)
        self.__tab = self.construct        
        
        # Events
        self.registerEvents()
        gobject.timeout_add(100, self.setTitle)
        
    def getRenameEntry(self): 
        return self.__master.renameEntry
    
    def getCanvas(self):
        return self.__draw.getCanvas()
    
    def getProperties(self):
        return self.__draw.getProperties()
    
    def getMenu(self):
        return self.__draw.getMenu()
        
    def getWidget(self, name):
        widget = self.__tree.get_widget(name)
        if not widget:
            return self.__master.getWidget(name)
        return widget
    
    def getConfig(self):
        return self.__config
    
    def signal_autoconnect(self, event_dict):
        self.__tree.signal_autoconnect(event_dict)
          
    def doSave(self):
        filename = self.__config.getFilename()
        if not filename:
            return self.doSaveAs()
        else:
            self.save()
            self.__config.saveToFile(filename)
        return gtk.RESPONSE_OK
    
    def doSaveAs(self):
        dialog = self.__master.getWidget("SaveDialog")    
        status = dialog.run()
        dialog.hide()
        if status == gtk.RESPONSE_OK:
            filename = dialog.get_filename()
            if filename:
                root, ext = os.path.splitext(filename)
                if not ext:
                    filename += '.xml'
                self.save()
                self.__config.saveToFile(filename)
                return gtk.RESPONSE_OK
        return gtk.RESPONSE_CANCEL
    
    def doSaveCurrent(self):
        if self.__config.isDirty():
            dialog = self.__master.getWidget("TripleDialog")
            filename = self.__config.getFilename()
            if filename:
                dialog.set_markup("Do you want to save '{0}'?".format(filename))
            else:
                dialog.set_markup("Do you want to save '{0}'?".format(self.__config.getName()))    
            status = dialog.run()
            dialog.hide()
            if status == gtk.RESPONSE_YES:
                return self.doSave()
            elif status == gtk.RESPONSE_CANCEL:
                return gtk.RESPONSE_CANCEL
            else:
                return gtk.RESPONSE_OK
        else:
            return gtk.RESPONSE_OK
              
    def eventTabSwitch(self, widget, page, pagenumber):
        print(self, 'tab switch')
        if self.__tab:
                self.__tab.save()
        self.__tab = self.__tabs[pagenumber]
        if self.__tab:            
            self.__tab.load()
        return True
            
    def undo(self):
        print(self, 'undo')
        if not self.__config.redoable(): # At the edge we must save the state because the canvas is unsynchronized while editing
            self.save()
        if self.__config.undoable():              
            undo, redo = self.__config.undo()
            self.load()
    
    def redo(self):
        print(self, 'redo')
        if self.__config.redoable():              
            undo, redo = self.__config.redo()
            self.load()
    
    def sync(self, save=True):        
        if save:
            self.save()
        self.__config.sync()
    
    def load(self):        
        print(self, 'loading')
        self.__tab.load()
            
    def save(self):
        print(self, 'saving')
        self.__tab.save()
            
    def switchTab(self, page):       
        self.__notebook.set_current_page(page)
        
    def setTitle(self):
        file = self.__config.getName()
        if self.__config.isDirty():
            file += '*'
        self.__label.set_text(file)
        return True
        
    def __repr__(self):
        return 'Tab ({0}) - {1}'.format(self.__page, self.__config.getName())
    
    def __close(self, event):
        self.__master.closeTab(self)