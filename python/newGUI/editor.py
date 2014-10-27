import gtk
import util
from base import SirannonBase, SirannonError

class SirannonEditor(SirannonBase):
    def __init__(self, ctx, widget):
        SirannonBase.__init__(self, ctx)
        self.registerEvents()
        self.widget = widget
        self.widget.get_buffer().connect("changed", self.eventEdit)
        
    def eventEdit(self, buffer):
        print 'Text changed'

    def load(self):
        self.orig = self.ctx.getConfig().toFormattedString()
        self.widget.get_buffer().set_text(self.orig)
        
    def save(self):
        buffer = self.widget.get_buffer()
        curr = buffer.get_text(buffer.get_start_iter(), buffer.get_end_iter())
        if curr != self.orig:            
            try:
                self.ctx.getConfig().updateFromString(curr)
            except:
                if util.warning(self.ctx, 'XML configuration contains errors. Do you wish to reload the original configuration?') == gtk.RESPONSE_OK:
                    self.ctx.getConfig().loadFromString(self.orig)
