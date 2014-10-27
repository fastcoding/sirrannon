from Tkinter import*
from time import time, localtime, strftime
	
class XSettings(Toplevel):
	
	def __init__( self, oMaster, *args, **kwargs ) :
		Toplevel.__init__( self, *args, **kwargs )
		
		self.title('Settings')		
		self._i = 0
		self.protocol( 'WM_DELETE_WINDOW', self.hide )		
		self._sEvent = '<<XSettings%d>>' % id(self)
		self.withdraw()
		self.oMaster = oMaster
	
	def update( self, oCommand ):
		self.oMaster.bind( self._sEvent, oCommand )
		
	def hide( self, oEvent=None ):
		self.withdraw()		
		self.oMaster.event_generate( self._sEvent )
		
	def show( self, oEvent=None ):
		self.deiconify()

	def test( self, oEvent=None ):
		print 'Joy!'
		
	def getEvent(self):
		return self._sEvent	
		
	def add( self, sSetting, sDesc, oValue ):
		## Does it already exist?
		if hasattr( self, sSetting ): return
		
		## Create an entry
		x = XEntry( self, '%s (%s)' % ( sSetting, sDesc ) )
		y,z = x.widgets()
		y.grid( row=self._i, column=0, padx=5, pady=5, sticky=W )
		z.grid( row=self._i, column=1, padx=5, pady=5, sticky=W )
		self._i += 1
		
		## Default
		x.set( str(oValue) )
		
		## Remember var
		setattr( self, sSetting, x )
		
	def get( self, sSetting ):
		## Does it exist?
		if not hasattr( self, sSetting ):
			raise ValueError, 'XSettings doesn\'t have a setting "%s"' % sSetting		
		return getattr( self, sSetting ).get()
	
	def set(self, sSetting, oVal ):
		if not hasattr( self, sSetting ):
			raise ValueError, 'XSettings doesn\'t have a setting "%s"' % sSetting		
		getattr( self, sSetting ).set( str(oVal) )
		
class XEntry:
	
	def __init__(self, pMaster, sText, sValue='', sTip=''):
		'''Creates a Label with string sText and an Entry with string sValue'''		
		self.pLabel = Label(pMaster, text=sText)
		self.oText = StringVar()
		self.pEntry = Entry(pMaster, textvariable=self.oText)
		self.oText.set(sValue)
		
		## Add event listeners for hovering over the label
		if sTip:
			self.oTooltip = ToolTip(self.pLabel, msg=sTip, delay=.5, follow=True)
					
	def disable(self):
		self.pEntry['state']=DISABLED
		
	def widgets(self):
		'''Returns the tuple (Label, Entry) which you need to map yourself'''
		return self.pLabel, self.pEntry
		 
	def get(self):
		'''Get the current string in the entry'''
		return self.oText.get()
	
	def set(self, val):
		'''Set the current string in the entry'''
		self.oText.set(val)		
		
	def showTip(self, event):
		pass
	
	def hideType(self, event):
		pass
			
class XOption:	
	def __init__(self, pMaster, sText, tOpt=('true', 'false'), sValue='true', sTip=''):
		self.pLabel = Label(pMaster, text=sText)
		self.pMenu = Menubutton(pMaster, text=tOpt[0])
		self.pMenu.menu = Menu(self.pMenu, tearoff=0)
		self.pMenu['menu'] = self.pMenu.menu
		self.oVar = StringVar()
		i = 0
		for sOpt in tOpt:
			self.pMenu.menu.add_radiobutton(label=sOpt, variable=self.oVar, value=tOpt[i], command=self.__select)
			i += 1
		self.oVar.set(sValue)
		self.pMenu['text'] = self.oVar.get()		
		if sTip:
			self.oTooltip = ToolTip(self.pLabel, msg=sTip, delay=.5, follow=True)
	
	def get(self):
		'''Get the current value of the option'''
		return self.oVar.get()
	
	def set(self, val):
		'''Set the current value of the option'''
		self.oVar.set(val)
		self.pMenu['text'] = self.oVar.get()
		
	def widgets(self):
		'''Returns the tuple (Label, Menu) which you need to map yourself'''
		return self.pLabel, self.pMenu
									
	def __select(self):		
		'''Internal function: handles a new selection'''
		self.pMenu['text'] = self.oVar.get()
		
class ToolTip( Toplevel ):
    """
    Provides a ToolTip widget for Tkinter.
    To apply a ToolTip to any Tkinter widget, simply pass the widget to the
    ToolTip constructor
    """ 
    def __init__( self, wdgt, msg=None, msgFunc=None, delay=1, follow=True ):
        """
        Initialize the ToolTip
        
        Arguments:
          wdgt: The widget this ToolTip is assigned to
          msg:  A static string message assigned to the ToolTip
          msgFunc: A function that retrieves a string to use as the ToolTip text
          delay:   The delay in seconds before the ToolTip appears(may be float)
          follow:  If True, the ToolTip follows motion, otherwise hides
        """
        self.wdgt = wdgt
        self.parent = self.wdgt.master                                          # The parent of the ToolTip is the parent of the ToolTips widget
        Toplevel.__init__( self, self.parent, bg='black', padx=1, pady=1 )      # Initalise the Toplevel
        self.withdraw()                                                         # Hide initially
        self.overrideredirect( True )                                           # The ToolTip Toplevel should have no frame or title bar
        
        self.msgVar = StringVar()                                               # The msgVar will contain the text displayed by the ToolTip        
        if msg == None:                                                         
            self.msgVar.set( 'No message provided' )
        else:
            self.msgVar.set( msg )
        self.msgFunc = msgFunc
        self.delay = delay
        self.follow = follow
        self.visible = 0
        self.lastMotion = 0
        Message( self, textvariable=self.msgVar, bg='#FFFFDD',
                 aspect=1000 ).grid()                                           # The test of the ToolTip is displayed in a Message widget
        self.wdgt.bind( '<Enter>', self.spawn, '+' )                            # Add bindings to the widget.  This will NOT override bindings that the widget already has
        self.wdgt.bind( '<Leave>', self.hide, '+' )
        self.wdgt.bind( '<Motion>', self.move, '+' )
        
    def spawn( self, event=None ):
        """
        Spawn the ToolTip.  This simply makes the ToolTip eligible for display.
        Usually this is caused by entering the widget
        
        Arguments:
          event: The event that called this funciton
        """
        self.visible = 1
        self.after( int( self.delay * 1000 ), self.show )                       # The after function takes a time argument in miliseconds
        
    def show( self ):
        """
        Displays the ToolTip if the time delay has been long enough
        """
        if self.visible == 1 and time() - self.lastMotion > self.delay:
            self.visible = 2
        if self.visible == 2:
            self.deiconify()
            
    def move( self, event ):
        """
        Processes motion within the widget.
        
        Arguments:
          event: The event that called this function
        """
        self.lastMotion = time()
        if self.follow == False:                                                # If the follow flag is not set, motion within the widget will make the ToolTip dissapear
            self.withdraw()
            self.visible = 1
        self.geometry( '+%i+%i' % ( event.x_root+10, event.y_root+10 ) )        # Offset the ToolTip 10x10 pixes southwest of the pointer
        try:
            self.msgVar.set( self.msgFunc() )                                   # Try to call the message function.  Will not change the message if the message function is None or the message function fails
        except:
            pass
        self.after( int( self.delay * 1000 ), self.show )
            
    def hide( self, event=None ):
        """
        Hides the ToolTip.  Usually this is caused by leaving the widget
        
        Arguments:
          event: The event that called this function
        """
        self.visible = 0
        self.withdraw()

	