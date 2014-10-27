import pygtk
pygtk.require("2.0")
import gtk
import subprocess
import shlex
import re
import gobject
import threading
import time
import Queue
import sys
import platform
from base import SirannonBase

DEFAULT_BIN_WIN = 'sirannon.exe'
DEFAULT_BIN = '/usr/local/bin/sirannon'
#DEFAULT_BIN = 'build/src/sirannon'
DEFAULT_GUI_XML = 'python/gui/dummy.xml'
DEFAULT_SHELL_FILE = 'python/gui/output.txt'

class SirannonCommand(SirannonBase):
    def __init__(self, ctx):
        SirannonBase.__init__(self, ctx)
        self.registerEvents()
        self.widget = self.ctx.getWidget('Run')
        self.console = self.ctx.getWidget('Console')
        self.consoleWidget = self.ctx.getWidget('ConsoleWidget')
        self.commandBin = self.ctx.getWidget('CommandBin')
        self.commandArgs = self.ctx.getWidget('CommandArgs')
        self.commandXML = self.ctx.getWidget('CommandXML')
        self.commandFlags = self.ctx.getWidget('CommandFlags')
        self.commandFlags.set_text('-vv')
        self.consoleWidget.connect('notify::expanded', self.eventExpanderToggle)
        self.buttonPlay = self.ctx.getWidget('buttonPlay')
        self.buttonStop = self.ctx.getWidget('buttonStop')
        self.buttonStop.set_sensitive(False)
        self.process = None
        self.args = {}
        self.shell_file = None
        self.shell_str = ''
        self.active = False        
        buffer = self.console.get_buffer()
        self.mark_end = buffer.create_mark('end', buffer.get_end_iter(), False )
        self.console.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse("black"))
        self.console.modify_text(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
        self.sanitize()
        
    def __del__(self):
        if self.process:
            print 'Killing process'
            self.process.kill()
            self.process.wait()
        
    def load(self):
        config = self.ctx.getConfig()      
        old_args = dict([(x, self.args[x].get_text()) for x in self.args.iterkeys()])        
        for child in self.commandArgs.get_children():
            self.commandArgs.remove(child)
        
        # Look for dollar signs
        new_args = {}
        for component in config.getComponents():            
            for param in config.getParams(component):
                val = param.getAttribute('val')
                search = re.search(r'\$(\d+)', val)
                if search:
                    arg = int(search.group(1))
                    if arg not in new_args:
                        new_args[arg] = []
                    new_args[arg].append((component.getAttribute('name'), param.getAttribute('name')))       

        # Rebuild the matrix
        self.args = {}
        if new_args:
            max_arg = max(new_args.iterkeys())
            i = 0
            for arg in range(1, max_arg+1):
                if arg not in new_args:
                    continue
                label = gtk.Label()
                label.set_text('Argument {0}'.format(arg))
                self.commandArgs.attach(label, 0, 1, i, i+1)
                
                label = gtk.Label()
                s = ''
                for component, param in new_args[arg]:
                    s += '{0}.{1}, '.format(component, param)
                label.set_text(s[:len(s)-2])
                self.commandArgs.attach(label, 1, 2, i, i+1)
                
                entry = gtk.Entry()
                if arg in old_args:
                    entry.set_text(old_args[arg])
                self.args[arg] = entry
                self.commandArgs.attach(entry, 2, 3, i, i+1)
                i += 1
        else:
            label = gtk.Label()
            label.set_text('No $ signs found in configuration')
            self.commandArgs.attach(label, 0, 3, 0, 1)
        self.commandArgs.show_all()
        
        filename = self.ctx.getConfig().getFilename()
        if filename:
            self.commandXML.set_filename(filename)
            
        command = config.getCommand(create=False)
        if command:
            self.commandBin.set_filename(command.getAttribute('binary'))
            if command.getAttribute('configuration'):
                self.commandXML.set_filename(command.getAttribute('configuration'))
            self.commandFlags.set_text(command.getAttribute('flags'))            
            for arg in command.childNodes:
                pos = int(arg.getAttribute('pos'))
                if pos in self.args:
                    self.args[pos].set_text(arg.getAttribute('val'))
    
    def save(self):
        config = self.ctx.getConfig()
        config.forceDirty()
        command = config.getCommand(create=True)        
        command.setAttribute('binary', self.commandBin.get_filename())
        command.setAttribute('configuration', self.commandXML.get_filename())
        command.setAttribute('flags', self.commandFlags.get_text())
        
        for child in command.childNodes:
            command.removeChild(child)
        for arg, entry in self.args.iteritems():
            node = config.createElement('arg')
            command.appendChild(node)
            node.setAttribute('pos', str(arg))
            node.setAttribute('val', entry.get_text())

    def sanitize(self):
        sys.stdout.flush()
        if not self.commandBin.get_filename():
            if platform.system() == 'Windows':
                self.commandBin.set_filename(DEFAULT_BIN_WIN)                
            else:
                self.commandBin.set_filename(DEFAULT_BIN)
    
    def eventExpanderToggle(self, expander, extra):
        if expander.get_expanded():
            self.widget.set_child_packing(expander, True, True, 5, gtk.PACK_START)
        else:
            self.widget.set_child_packing(expander, False, False, 5, gtk.PACK_START)
            
    def eventPlay(self, widget):
        if self.active:
            return
        self.sanitize()
        
        config = self.ctx.getConfig()
        
        filename = self.commandXML.get_filename()        
        if filename:
            if filename == config.getFilename():
                self.ctx.doSave()
        else:
            filename = config.getFilename()
            if not filename:
                if self.ctx.doSaveAs() != gtk.RESPONSE_OK:
                    return
                filename = config.getFilename()
                
        self.commandXML.set_filename(filename)
            
        args = [self.commandBin.get_filename()]
        if self.commandFlags.get_text():
            args += shlex.split(self.commandFlags.get_text())
        args.append(filename)
        if self.args:
            max_args = max(self.args.iterkeys())
            for arg in range(1, max_args+1):
                if arg in self.args:
                    args.append(self.args[arg].get_text())
                else:
                    args.append('')
        shell = ' '.join(args)        

        # create the child process
        if platform.system() != 'Windows':
            self.process = subprocess.Popen(args, stdout=subprocess.PIPE, 
                                        stderr=subprocess.PIPE, bufsize=-1, close_fds=True)
        else:
            self.process = subprocess.Popen(args, stdout=subprocess.PIPE, 
                                        stderr=subprocess.PIPE, bufsize=-1)        
        import fcntl, os
        fcntl.fcntl(self.process.stderr, fcntl.F_SETFL, os.O_NONBLOCK)        
        
        # create a thread to read the output of the process
        gobject.timeout_add(200, self.consoleCycle)
        
        # Reset console
        self.consoleWidget.set_expanded(True)
        self.console.get_buffer().set_text('')
        
        self.active = True
        self.buttonStop.set_sensitive(True)
        self.buttonPlay.set_sensitive(False)
        
    def eventStop(self, widget=None):
        if not self.active:
            return
        self.active = False
        if self.process.poll() is None:
            self.process.kill()
            self.process.wait()
        self.printShell()
        self.buttonStop.set_sensitive(False)
        self.buttonPlay.set_sensitive(True)
        
    def consoleCycle(self):
        if not self.active:
            return False
        self.printShell()
        if self.process.poll() is not None:
            self.eventStop()    
        return True
    
    def printShell(self):
        try:
            lines = self.process.stderr.read(1000000)
        except:
            return
        buffer = self.console.get_buffer()
        buffer.insert(buffer.get_iter_at_mark(self.mark_end), lines)
        self.console.scroll_to_mark(self.mark_end, 0.)
        