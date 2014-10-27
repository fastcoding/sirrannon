import pygtk
pygtk.require("2.0")
import gtk

import util

MODE_RTP = 1
MODE_RTSP = 2
MODE_TS = 4

import canvas
from base import SirannonBase, SirannonPalette

class SirannonConstruct(SirannonBase):
    def __init__(self, ctx):
        SirannonBase.__init__(self, ctx)
        self.registerEvents()
        self.registerWidgets('cstTable')
        self.prev = None
        self.prev2 = None
        self.current = None
        self.x = canvas.MODEL_DEFAULT_WIDTH * 1.2
        self.y = canvas.MODEL_DEFAULT_WIDTH * 1.2
        
        self.source = self.source = SirannonPalette()
        self.source.addExpander('Source', ('File', 'file'), ('URL', 'entry'), expanded=True)
        self.video = SirannonPalette()
        self.video.addExpander('Video Transcoding', ('No', None), ('H264', None), ('WebM', None), ('M2V', None), ('Other', 'entry'))
        self.audio = SirannonPalette()
        self.audio.addExpander('Audio Transcoding', ('No', None), ('M4A', None), ('Vorbis', None), ('MP3', None), ('Other', 'entry'))
        self.pack = SirannonPalette()
        self.pack.addExpander('Packetization', ('Default', None), ('No', None), ('Other', 'entry'))
        self.mux = SirannonPalette()
        self.mux.addExpander('Multiplexing', ('No', None), ('Basic', None), ('MPEG-2 Transport Stream (.ts)', None), ('Flash Video (.flv)', None), ('MPEG-4 (.mp4, .mov)', None), expanded=True)
        self.impair = SirannonPalette()
        self.impair.addExpander('Impair', ('No', None), ('Random', 'entry'), ('Gilbert', 'entry'))
        self.sink = SirannonPalette()
        self.sink.addExpander('Sink', ('File', 'file'), ('RTP', 'entry'), ('UDP', 'entry'), expanded=True)
        self.cstTable.attach(self.source, 0, 1, 0, 1)
        self.cstTable.attach(self.mux, 1, 2, 0, 1)
        self.cstTable.attach(self.sink, 2, 3, 0, 1)
        self.cstTable.attach(self.pack, 0, 1, 1, 2)
        self.cstTable.attach(self.video, 1, 2, 1, 2)
        self.cstTable.attach(self.audio, 2, 3, 1, 2)
        self.cstTable.attach(self.impair, 0, 1, 2, 3)
        self.cstTable.show_all()
           
    def findActive(self, button):
        group = button.get_group()
        for button in group:
            if button.get_active():
                return button.get_name()
        return None
    
    def create(self, name, type):
        if not name or not type:
            raise(RuntimeError('{0} - {1}'.format(name, type)))
        self.prev = self.current
        self.current = name
        component = self.ctx.getConfig().createComponent(name, type)
        gfx = self.ctx.getConfig().getChild(component, 'gfx')
        gfx.setAttribute('x', str(self.x))
        gfx.setAttribute('y', str(self.y))
        gfx.setAttribute('w', str(canvas.MODEL_DEFAULT_WIDTH))
        gfx.setAttribute('h', str(canvas.MODEL_DEFAULT_HEIGHT))
        gfx.setAttribute('r', '0')
        self.x += canvas.MODEL_DEFAULT_WIDTH * 1.2
        return component
    
    def route(self, route=0):
        self.ctx.getConfig().createRoute(self.prev, self.current, route)
        if self.prev2 and self.prev != self.prev2:
            self.ctx.getConfig().createRoute(self.prev2, self.current, route)
            self.prev2 = ''
    
    def eventApply(self, button):
        config = self.ctx.getConfig()
        config.updateFromDefault()     
        self.x = 100
        mode = 0
        
        is_RTP = False
        is_TS = False
        is_File = False
        
        option, mux = self.mux.getChoice()
        if mux == 'MPEG-2 Transport Stream (.ts)':
            is_TS = True
        option, sink = self.sink.getChoice()
        if option == 'File':
            is_File = True
        
        # Create source
        option, source = self.source.getChoice()
        if option == 'File':
            component = self.create('source', 'ffmpeg-reader')
            if not source:
                return self.bail('Field Source.File: No such file')
            config.setParam(component, 'repeat-parameter-sets', str(True))
            config.setParam(component, 'filename', source)
        else:
            try:
                suffix, unused = source.split('://')
            except:
                return self.bail('Field Source.URL: Invalid URL')                
            try:
                component = self.create('source', '{0}-client'.format(suffix.upper()))
                config.setParam(component, 'url', source)
            except:
                return self.bail('Field Source.URL: No client protocol {0}'.format(suffix.upper()))
    
        # Transcode
        option, codec = self.video.getChoice()
        if option != 'No':
            if not codec:
                return self.bail('Field Video_Transcoding.Other is empty')
            component = self.create('video-transcoder', 'transcoder-video')
            config.setParam(component, 'output-codec', codec)
            self.route(100)
            
        # Transcode audio
        option, codec = self.audio.getChoice()
        if option != 'No':
            if not codec:
                return self.bail('Field Audio_Transcoding.Other is empty')
            self.current = 'source'
            component = self.create('audio-transcoder', 'transcoder-audio')
            self.prev2 = 'audio-transcoder'
            config.setParam(component, 'output-codec', codec)
            self.route(200)
            
        # Packetizer
        option, pack = self.pack.getChoice()
        if option != 'No':
            component = None
            if option == 'Default':                
                if is_TS:
                    component = self.create('packetizer', 'default-packetizer')
                    param = config.getParam(component, 'type')
                    param.setAttribute('val', 'PES')
                elif not is_File:
                    component = self.create('packetizer', 'default-packetizer')
            elif option == 'Sirannon':
                component = self.create('packetizer', 'sirannon-packetizer')
            elif option == 'Other':
                component = self.create('packetizer', '{0}-packetizer'.format(pack.upper()))
                if not config.getParams(component):
                    return self.bail('Field Packetizer.Other: No packetizer for codec({0})'.format(pack))
            if component:
                self.route()       
            
        # Multiplexer
        option, mux = self.mux.getChoice()
        if mux != 'No':
            if mux == 'Basic':
                component = self.create('multiplexer', 'std-multiplexer')
            elif mux == 'MPEG-2 Transport Stream (.ts)':
                component = self.create('multiplexer', 'TS-multiplexer')
            else:
                component = self.create('multiplexer', 'std-multiplexer')
            self.route()
            
        # Impair
        option, impair = self.impair.getChoice()
        if option != 'No':
            if option == 'Random':
                component = self.create('impair', 'random-classifier')
                config.setParam(component, 'P', impair)  
            elif option == 'Gilbert':
                component = self.create('impair', 'gilbert-classifier')
                try:
                    a, b, c, d = impair.split(',')
                except:
                    return self.bail('Field Impair.Gilbert: Invalid comma seperated parameter list: alpha, beta, gamma, delta')
                config.setParam(component, 'alpha', a)
                config.setParam(component, 'beta', b)
                config.setParam(component, 'gamma', c)
                config.setParam(component, 'delta', d)
            config.setParam(component, 'discard', 'True')            
            self.route()
            
        # Scheduler
        option, sink = self.sink.getChoice()
        if option != 'File':
            component = self.create('scheduler', 'frame-scheduler')
            self.route()
            
        # Sink
        option, sink = self.sink.getChoice()
        if option == 'File':
            if not sink:
                return self.bail('Field Sink.File: No such file')
            if mux == 'No' or mux == 'Basic' or mux == 'MPEG-2 Transport Stream (.ts)':
                component = self.create('writer', 'basic-writer')
                config.setParam(component, 'filename', sink)
            else:
                muxx = ''
                if mux == 'Flash Video (.flv)':
                    muxx = 'flv'
                elif mux == 'MPEG-4 (.mp4, .mov)':
                    muxx = 'mov'
                elif mux == 'WebM (.webm)':
                    muxx = 'webm'
                component = self.create('writer', 'ffmpeg-writer')
                config.setParam(component, 'filename', sink)
                config.setParam(component, 'format', muxx)            
            
        elif option == 'RTP':
            if not sink:
                return self.bail('Field Sink.RTP: Invalid destination address')
            component = self.create('RTP-transmitter', 'RTP-transmitter')
            config.setParam(component, 'destination', sink)
            
        elif button == 'UDP':
            if not sink:
                return self.bail('Field Sink.UDP: Invalid destination address')
            component = self.create('UDP-transmitter', 'UDP-transmitter')
            config.setParam(component, 'destination', sink)
        else:
            raise(RuntimeError('No sink'))            
        self.route()
            
        # Real sink
        component = self.create('sink', 'sink')
        self.route()        
                
        self.ctx.switchTab(1)
        self.ctx.getCanvas().eventZoomNormal(None)
            
    def bail(self, error):
        self.ctx.getConfig().purge()
        util.warning(self.ctx, error)
        