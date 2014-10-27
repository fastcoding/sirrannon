#!/usr/bin/env python
import sys, os
if os.name == 'posix':
    sys.path.append('libs/crcanvas-0.19/python/')

# import and versions
import pygtk
pygtk.require("2.0")
import gtk
print gtk.pygtk_version
import gtk.glade
import gobject
import os
import os.path
import util

from config import SirannonConfiguration
import canvas
import editor
import command
import construct
import library
from base import SirannonBase, SirannonError, SirannonPalette
from tab import SirannonTab

# Defaults
GLADE_FILE = 'python/newGUI/sirannon.glade'
DEFAULT_XML_FOLDER = 'dat/xml'

class SirannonGTK(SirannonBase):    
    def __init__(self, gladefile=GLADE_FILE):
        SirannonBase.__init__(self, self)
        self.__gladefile = gladefile
        self.__tree = gtk.glade.XML(self.__gladefile)
        self.registerEvents()
        
        # Main window
        self.window = self.__tree.get_widget("Main")
        self.window.connect("delete-event", self.eventQuit)
        self.window.set_title('Sirannon')
        self.window.maximize()
         
        # Notebook with file tabs
        self.__notebook = self.__tree.get_widget("FileTabs")
        self.__tabs = []       
        
        # Dialogs
        self.xml_filter = gtk.FileFilter()
        self.xml_filter.set_name('XML file')
        self.xml_filter.add_pattern('*.xml')
        dialog = self.__tree.get_widget("LoadDialog")
        dialog.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        dialog.add_button(gtk.STOCK_OPEN, gtk.RESPONSE_OK)        
        dialog.set_current_folder(DEFAULT_XML_FOLDER)
        dialog.add_filter(self.xml_filter)        
        dialog = self.__tree.get_widget("SaveDialog")        
        dialog.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        dialog.add_button(gtk.STOCK_SAVE, gtk.RESPONSE_OK)
        dialog.set_current_folder(DEFAULT_XML_FOLDER)
        dialog.add_filter(self.xml_filter)                
        dialog = self.__tree.get_widget("TripleDialog")        
        dialog.add_button(gtk.STOCK_NO, gtk.RESPONSE_NO)
        dialog.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        dialog.add_button(gtk.STOCK_YES, gtk.RESPONSE_YES)        
        dialog = self.__tree.get_widget('RenameDialog')  
        self.renameEntry = gtk.Entry()
        content = dialog.get_content_area()
        content.pack_start(self.renameEntry,fill=False)
        content.show()        
        accel = gtk.AccelGroup()
        self.window.add_accel_group(accel)
        self.undoButton = self.__tree.get_widget('MenuUndo')
        self.undoButton.add_accelerator('activate', accel, ord('z'), gtk.gdk.CONTROL_MASK, gtk.ACCEL_VISIBLE|gtk.ACCEL_LOCKED)
        self.redoButton = self.__tree.get_widget('MenuRedo')
        self.redoButton.add_accelerator('activate', accel, ord('y'), gtk.gdk.CONTROL_MASK, gtk.ACCEL_VISIBLE|gtk.ACCEL_LOCKED)
        
        commandXML = self.__tree.get_widget('CommandXML')
        commandXML.add_filter(self.xml_filter)          
       
        # Simulate a new event
        self.eventNew(None)        
        self.__notebook.remove_page(0)
        gobject.timeout_add(100, self.update)
        
        # Override some settings
        settings = gtk.settings_get_default()
        settings.props.gtk_button_images = True

    def main(self, argv=[]):
        if len(argv) > 0:
            self.openFile(argv[0])
        gtk.main()
    
    def newTab(self, config):
        print 'New context'
        tab = SirannonTab(self, config, self.__gladefile, self.__notebook)
        self.__tabs.append(tab)
        return tab
        
    def closeTab(self, tab):
        pagenumber = self.__tabs.index(tab)
        if tab.doSaveCurrent() != gtk.RESPONSE_OK:
            return True
        self.__notebook.remove_page(pagenumber)
        self.__tabs.pop(pagenumber)   
        if self.__tabs:
            self.switchTab(-1)
        else:
            self.eventQuit()
        return gtk.RESPONSE_OK
    
    def eventReloadTemplates(self, event):
        SirannonConfiguration.loadTemplates()
        for tab in self.__tabs:
            tab.getMenu().reload()
          
    def eventOpen(self, widget):
        dialog = self.__tree.get_widget("LoadDialog")    
        status = dialog.run()
        dialog.hide()
        if status == gtk.RESPONSE_OK:
            filename = dialog.get_filename()
            if filename:
                self.openFile(filename)
        return True           
    
    def openFile(self, filename): 
         # Sanitize name
        root, ext = os.path.splitext(filename)
        if not ext:
            filename += '.xml'
        if not os.path.isfile(filename):
            return util.warning(self, 'File({0}) does not exist'.format(filename))
                    
        # Check for doubles                
        for idx, config in enumerate(self.getConfigs()):
            if config.getFilename() == filename:
                self.switchTab(idx)
                return util.warning(self, 'File "{0}" is already open'.format(filename))
            
        # New config
        config = SirannonConfiguration()
        config.loadFromFile(filename)
                        
        # Check for outdated parameters and ask user for removal                
        if config.proofRead() < 0:
            config.sync()  
            dialog2 = self.getWidget('QuestionDialog')
            dialog2.set_markup('Configuration contains outdated or unkown parameters for some components. Do you wish to remove them?')
            status = dialog2.run()
            dialog2.hide()
            sys.stdout.flush()
            if status == -8:
                config.proofRead(correct=True)
        # Show the opened XML
        self.newTab(config)
        self.switchTab(-1, 1)
        return True
    
    def eventSave(self, widget):
        self.getTab().doSave()
        return True
                
    def eventSaveAs(self, widget):
        self.getTab().doSaveAs()
        return True
     
    def eventNew(self, widget):
        config = SirannonConfiguration()
        config.loadFromDefault()
        self.newTab(config)
        self.switchTab(-1, 1)
        return True
    
    def eventQuit(self, widget=None, other=None):
        for tab in self.__tabs:
            if tab.doSaveCurrent() != gtk.RESPONSE_OK:
                return True
        gtk.main_quit()
        
    def eventUndo(self, button):
        self.getTab().undo()
        return True
    
    def eventRedo(self, button):
        self.getTab().redo()
        return True
        
    def update(self):
        self.redoButton.set_sensitive(self.getConfig().redoable())
        self.undoButton.set_sensitive(self.getConfig().undoable())
        return True
    
    def getConfigs(self):
        return [tab.getConfig() for tab in self.__tabs]
    
    def getConfig(self):
        return self.getTab().getConfig()
    
    def getTab(self):
        return self.__tabs[self.__notebook.get_current_page()]
    
    def getWidget(self, name):
        return self.__tree.get_widget(name)
    
    def switchTab(self, page, innerpage=None):
        self.__notebook.set_current_page(page)
        if innerpage is not None:
            self.__tabs[page].switchTab(innerpage)
    
    def signal_autoconnect(self, event_dict):
        self.__tree.signal_autoconnect(event_dict)
    
if __name__ == '__main__':
    import sys
    gui = SirannonGTK()
    gui.main(sys.argv[1:])
