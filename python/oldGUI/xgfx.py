from Tkinter import *
from xlib import*
#from xcomponents import *
from components_gen import *
from Dialog import Dialog
from tkMessageBox import askquestion, Message
from tkFileDialog import askopenfilename, asksaveasfilename
import os

g_initialDir = os.path.join( 'dat', 'xml' )
box_font_b = ('Helvetica', '8', 'bold')
box_font_i = ('Helvetica', '8', 'italic')
button_font_big = ( 'Tahoma', '10', 'bold' )

light_orange = '#fbc544'
lime		 = '#9ACD32'
sky_blue	 = '#6495ED'
dark_grey    = '#9B9B9B'

sBackground = '#EEEEEE'
sComponent = '#C3D9FF'
sComponentFocus = '#356AA0'
sConnection = '#3F4C6B'

iDefaultWidth = 150
iDefaultHeight = 120

## Generalise directions
dxy = { NW:(-1,-1), N:(0,-1), NE:(1,-1), E:(1,0), SE:(1,1), S:(0,1), SW:(-1,1), W:(-1,0) }
lxy = [(-1,-1), (0,-1), (1,-1), (1,0), (1,1), (0,1), (-1,1), (-1,0)]
minimax = lambda x,mi,ma: min(max(mi,x),ma)

class XConfiguration(Toplevel):
	
	def __init__(self, cnfg):
		## Our root)
		self.root = Tk()
		self.root.withdraw()
		
		## What dimensions?
		h = self.root.winfo_screenheight()
		w = self.root.winfo_screenwidth()

		## Baseclass
		Toplevel.__init__(self, width=w, height=0.9*h)		
		self.protocol("WM_DELETE_WINDOW", self.Quit)
		
		## Linked object
		self.cnfg = cnfg
		self.mode = IntVar()
		
		## Create a canvas
		self.canv = XCanvas (self, 0.95*w, 0.85*h, bg=sBackground)
		self.canv.grid(row=0, column=0)
		
		## Settings
		self.oSettings = XSettings( self)
		self.oSettings.add( 'Quantum', 'the minimum time per step in us (0: active loop)', 1000 )
		self.oSettings.add( 'Simulation', 'the simulation time per step in us (0: realtime)', 0 )
		self.oSettings.add( 'Seed', 'the seed of random numbers (0: initialize with present time)', 0 )
		self.oSettings.update( self.dirty )

		## Create a menu
		self.pMenuBar = Menu(self)
		pMenu = Menu(self.pMenuBar, tearoff=0)
		self.pMenuBar.add_cascade(label = 'File', menu = pMenu)
		pMenu.add_command(label='Load Ctrl+L', command=self.LoadXML)
		pMenu.add_command(label='Save Ctrl+S', command=self.SaveXML)
		pMenu.add_command(label='Save as', command=self.GenerateXML)
		pMenu.add_separator()
		pMenu.add_command(label='Quit',	command=self.Quit)
		
		## Create edit menu
		pMenu = Menu(self.pMenuBar, tearoff=0)
		self.pMenuBar.add_cascade(label = 'Edit', menu = pMenu)
		pMenu.add_radiobutton(label='Drag',    variable=self.mode, value=0, command=self.SetMode)
		pMenu.add_radiobutton(label='Connect', variable=self.mode, value=1, command=self.SetMode)
		pMenu.add_separator()
		pMenu.add_command(label='Resize components', command=self.SqrComp)
		pMenu.add_command(label='Spread components', command=self.Rasterize)
		pMenu.add_command(label='Clear components', command=self.ClearCmpnts)
		pMenu.add_command(label='Clear connections', command=self.ClearLines)
		
		## Create settings menu
		pMenu = Menu(self.pMenuBar, tearoff=0)
		self.pMenuBar.add_cascade(label = 'Settings', menu = pMenu)
		pMenu.add_command(label='Settings', command=self.oSettings.show)
				
		## Create mode menu
		pMenu = Menu(self.pMenuBar, tearoff=0)
		self.pMenuBar.add_cascade(label = 'Run', menu = pMenu)
		pMenu.add_command(label='Launch', command=self.Launch)
		pMenu.add_command(label='Batch', command=self.Batch)
		
		## Set it as the window menu
		self.config(menu = self.pMenuBar)
		
		## Register CRTL+S
		self.bind( '<Control-Key-s>', self.SaveXML )
		self.bind( '<Control-Key-l>', self.LoadXML )
		
		## Launch menu
		self.pLaunch = XLaunch( self )
		self.pLaunch.withdraw() # start hidden
		self.pBatch = XBatch( self )
		self.pBatch.withdraw() # start hidden
		
		## Filename
		self.sFile  = ''
		self.bDirty = False
		self.SetFile('')	
							
	def destroy(self, e=None):
		## Catch the end
		import sys
		sys.exit(0)			
		
	def Go(self):		
		## Mainloop
		self.root.mainloop()
		
	def SetFile(self,sFile):
		self.sFile = sFile
		self.dirty(bState=False)
		self.RefreshTitle()
		
	def dirty(self,bState=True,bArgs=True):
		if self.bDirty != bState:
			self.bDirty = bState
			self.RefreshTitle()
		if bArgs:
			self.RefreshLaunch()
		
	def RefreshTitle(self):
		s = 'xstreamer interface '
		if self.sFile:
			s += '- ' + os.path.split( self.sFile )[-1]
		if self.bDirty:
			s += '*'
		self.title( s )
		
	def RefreshLaunch(self):
		self.pLaunch.Refresh()
		self.pBatch.Refresh()
		
	def Launch(self,oEvent=None):
		## Show the launch window
		self.pLaunch.deiconify()
		
	def Batch(self,oEvent=None):
		## Show the launch window
		self.pBatch.deiconify()

	def GenerateXML(self,sFile=''):
		## Ask for file
		if not sFile:
			sFile = asksaveasfilename(initialdir=g_initialDir,filetypes = [('XML-file','*.xml')], parent = self.master, title = 'Save XML-file')
		if not sFile:			
			return None	## The user canceled	
		if sFile.rfind('.xml') == -1:
			sFile += '.xml'	
			
		## Remember the last name
		self.SetFile( sFile )
		
		## Log our gfx positions
		self.canv.SaveGfx()
		
		## Execute
		return self.cnfg.GenerateXML(sFile)
		
	def SaveXML(self,iEvent=None): 
		## We save using the current filename
		return self.GenerateXML(self.sFile)	
		
	def LoadXML(self,iEvent=None):
		## Log our gfx positions
		self.canv.SaveGfx()
		
		## Save previous
		if self.bDirty:	
			s = YesNoCancel('Confirm', 'Do you want to save the current session?')
			if s == 'yes':
				self.GenerateXML()
				return None
					
		## Ask for file to load
		sFile = askopenfilename(initialdir=g_initialDir,filetypes = [('XML-file','*.xml')], parent = self.master, title = 'Load XML-file')
		if not sFile: 
			return None ## The user canceled
		if sFile.rfind('.xml') == -1:
			sFile += '.xml'
			
		## File
		self.SetFile( sFile )
			
		## Execute
		oRet = self.cnfg.LoadXML(self.sFile)
		
		## Refresh laumcher
		self.RefreshLaunch()
		
		return oRet
		
	def SqrComp(self):
		if askquestion('Confirm', 'Are you sure?') == 'yes':
			self.canv.SqrComp()
	
	def ClearCmpnts(self):
		if askquestion('Confirm', 'Are you sure?') == 'yes':
			self.canv.delAllCmpnts()
		
	def ClearLines(self):
		if askquestion('Confirm', 'Are you sure?') == 'yes':
			self.canv.delAllLines()
			
	def Rasterize(self):
		if askquestion('Confirm', 'Are you sure?') == 'yes':
			self.canv.Rasterize()
		
	def Quit(self):
		## Log our gfx positions
		self.canv.SaveGfx()
		
		## Save previous
		if self.bDirty:
			s = YesNoCancel('Confirm', 'Do you want to save the current session?')
			if s == 'yes':
				if not self.SaveXML():
					return
			elif s == 'cancel':
				return
				
		## Done
		self.destroy()
				
	def GetQuantum(self):
		return int(self.oSettings.get('Quantum'))
	
	def SetQuantum(self,iVal):
		if self.oSettings.get('Quantum') != iVal:
			self.dirty()
			self.oSettings.set( 'Quantum', iVal )
			
	def GetSimulation(self):
		return int(self.oSettings.get('Simulation'))
	
	def SetSimulation(self,iVal):
		if self.oSettings.get('Simulation') != iVal:
			self.dirty()
			self.oSettings.set('Simulation', iVal)
			
	def GetSeed(self):
		return int(self.oSettings.get('Seed'))
	
	def SetSeed(self,iVal):
		if self.oSettings.get('Seed') != iVal:
			self.dirty()
			self.oSettings.set('Seed', iVal)
		
	def Settings(self):
		## Activate settings
		self.cnfg.oSettings.Show()
		
	def SetMode(self):
		i = self.mode.get()		
		if i==0:
			self.canv.mode = 'drag'
		elif i==1:
			self.canv.mode = 'connect'
			
class XLaunch(Toplevel):
	def __init__(self, master, **kw):
		Toplevel.__init__(self, **kw)
		self.dArgs = {}
		
		## Title
		self.master = master
		self.title('Run dialog')
		self.configure( bd=12, width=250, height=250)
		
		## Program name
		self.pTopFrame = Frame( self )
		self.pTopFrame.grid(row=0)
		x = XEntry( self.pTopFrame, 'program' )
		x.set( 'xstreamer.exe' )
		y,z = x.widgets()
		y.grid(row=0,column=0,sticky=NW,padx=5,pady=5)
		z.grid(row=0,column=1,sticky=NW,padx=5,pady=5)
		self.pProgEntry = x
			
		## Get all $ arguments
		self.pArgFrame = Frame(self)
		self.pArgFrame.grid(row=1,column=0,columnspan=2)
			
		## Run button
		x = Frame( self )
		x.grid(row=2,column=0,columnspan=2,pady=10)
		x.columnconfigure(0,minsize=75)
		x.columnconfigure(1,minsize=75)		
		self.pLaunch = Button( x, text='Launch', command=self.Launch, bd=4, padx=5, pady=5, font=button_font_big  )
		self.pLaunch.grid(row=0,column=0)
		self.pStop = Button( x, text='Stop', command=self.Stop, bd=4, padx=5, pady=5, font=button_font_big )
		self.pStop.grid(row=0,column=1)
		self.pStop.config(state=DISABLED)
		self.iXstreamerID = 0
		
		## Handle close & open
		self.protocol("WM_DELETE_WINDOW", self.Quit)
		
	def Refresh(self, pEvent=None):
		## Old and new
		dArgs = self.dArgs
		self.dArgs = {}		
		pArgFrame = self.pArgFrame
		self.pArgFrame = Frame( self )
				
		## Get all $ arguments
		lArgs = self.master.cnfg.GetCommandLineArgs()
		lArgs.sort()		
		for i,sArg in enumerate(lArgs):
			x = XEntry( self.pArgFrame, sArg )
			y,z = x.widgets()
			y.grid(row=i,column=0,sticky=NW,padx=5,pady=5)
			z.grid(row=i,column=1,sticky=NW,padx=5,pady=5)
			
			## already defined?
			if sArg in dArgs:
				x.set( dArgs[sArg].get() )
			self.dArgs[sArg] = x
			
		## Destroy the old one
		pArgFrame.destroy()	
		self.pArgFrame.grid(row=1,column=0,columnspan=2)
		
	def Stop(self):
		## Kill the program!
		if self.iXstreamerID:
			try:
				## Unix solution				
				os.kill( self.iXstreamerID )
			except AttributeError:
				## Windows solution
				print 'gui\kill.exe' + ' ' + '-f %d' % self.iXstreamerID
				os.spawnv( os.P_WAIT, 'gui\kill.exe', [  ] )
		self.iXstreamerID = 0
		
	def Launch(self):
		## Save the xml
		if not self.master.SaveXML():
			return		
			
		## Activate it in a seperate thread
		import thread
		thread.start_new_thread( self.LaunchX, (self.master.sFile,) )
		
	def Quit(self,pEvent=None):
		## Handle close
		self.withdraw()
			
	def LaunchX(self, sFile):
		## unlock button
		self.pLaunch.config(state=DISABLED)
		self.pStop.config(state=NORMAL)

		## Launch
		sProg = os.path.join('.',self.pProgEntry.get())	
		lArgs = [ os.path.split(sProg)[1], sFile ]
		lKeys = self.dArgs.keys()
		lKeys.sort()
		lArgs += [ self.dArgs[sKey].get() for sKey in lKeys ]
		print ( 'running: ' + '%s ' * len(lArgs) ) %  tuple(lArgs) 
		self.iXstreamerID = os.spawnv( os.P_NOWAIT, sProg, lArgs )
		
		## Wait
		os.waitpid(self.iXstreamerID,0)

		## unlock button
		self.pLaunch.config(state=NORMAL)
		self.pStop.config(state=DISABLED)
		
class XBatch(XLaunch):
	def __init__(self, master, **kw):
		XLaunch.__init__(self, master, **kw)
		
		self.title('Batch dialog')
		
		## Add another entry
		x = XEntry( self.pTopFrame, 'cycles' )
		x.set( '1' )
		y,z = x.widgets()
		y.grid(row=1,column=0,sticky=NW,padx=5,pady=5)
		z.grid(row=1,column=1,sticky=NW,padx=5,pady=5)
		self.pCyclesEntry = x
		
	def LaunchX( self, sFile):
		## Launch
		sProg = os.path.join( '.', self.pProgEntry.get() )		
		lKeys = self.dArgs.keys()
		lKeys.sort()
		
		## How many times to run the batch
		iCycles = int( self.pCyclesEntry.get() )
				
		## Run this iCycles amount of times
		for i in range( iCycles ):
			## Construct the arguments each time increasing
			lArgs = [ os.path.split( sProg )[1], sFile ]
			for sKey in lKeys:
				sVal   = self.dArgs[sKey].get()
				tSplit = sVal.split(':')
				if len(tSplit) > 1:
					fStart = float( tSplit[0] )
					fStep  = float( tSplit[1] )
					fVal = fStart + i * fStep
					lArgs.append( str( fVal ) )
				else:
					lArgs.append( sVal )
								
			## Launch the xstreamer
			#print ( 'running: ' + '%s ' * len(lArgs) ) %  tuple(lArgs) 
			os.spawnv( os.P_WAIT, sProg, lArgs )
			
class XConnector(Frame):
	
	def __init__(self, master, cnfg, cmpnt1, cmpnt2, **kw):
		Frame.__init__(self, master, width = 100, height=25, relief=RAISED, **kw)
		
		## Attrs
		self.cnfg  = cnfg
		self.tag1 = cmpnt1.name
		self.tag2 = cmpnt2.name
		
		## Entry field
		self.x = XEntry(self, 'xroute', 0)
		y,z = self.x.widgets()
		y.grid(row=0,column=0,sticky=NW,padx=5,pady=5)		
		z.grid(row=0,column=1,sticky=NW,padx=5,pady=5)
		
		## Reference class		
		self.route = cnfg.MakeRoute(cmpnt1, cmpnt2)
				
		## Save data when we are unmapped
		self.bind('<Unmap>', self.__Unmap)
		self.bind('<Map>', self.__Map)
		
		## Pass
		self.SetGfx = self.route.SetGfx
		
	def __Map(self, e):
		self.x.set(self.route.Get())
		
	def __Unmap(self, e):
		## Log the values
		self.route.Set(self.x.get())
		
	def set(self, val):
		self.x.set(val)
		
	def get(self):
		return self.x.val()
		
	def destroy(self):
		self.cnfg.RemoveRoute(self.route)
		Frame.destroy(self)

		
class XComponent(Frame):
	
	def __init__(self, master, cnfg, name, screenname, otype, **kw):
		Frame.__init__(self, master, class_='XComponent', width = 300, height=300, relief=GROOVE, bd=5, **kw)
				
		## Linked component
		self.cnfg  = cnfg
		self.name  = name
		if type(otype) != type(self):
			self.cmpnt = cnfg.MakeComponent(otype, screenname)
		else:
			sname = self.cnfg.GenerateValidName(otype.cmpnt.GetName(), other=otype)
			self.cmpnt = otype.cmpnt.copy(sname)			
			self.cnfg.AddComponent(self.cmpnt)			
			
		## Attributes
		self.dAttrs = {}
		i = 0
		for sName in self.cmpnt.lAttrs:
			sValue = self.cmpnt.dAttrs[sName]
			
			## Input widget
			if sName in ('type', 'name'):
				x = XEntry(self,  sName, sValue, self.cmpnt.GetTip())
			else:
				x = XEntry(self,  sName, sValue)
			self.dAttrs[sName] = x
			
			## Rasterize
			y,z = x.widgets()
			y.grid(row=i%10,column=i/10*2+0,sticky=NW,padx=5,pady=5)		
			z.grid(row=i%10,column=i/10*2+1,sticky=NW,padx=5,pady=5)
			i += 1
			
			## Special case
			if sName == 'type':
				x.disable()
						
		## Parameters
		self.dParams = {}
		for sName, sType in self.cmpnt.lParams:
			sValue = self.cmpnt.dParams[sName, sType]
			sTip = self.cmpnt.dInfo[sName, sType]
			
			if sType == 'bool':
				x = XOption(self, '%s (%s)' % (sName,sType), sValue=sValue, sTip=sTip)
			else:
				x = XEntry(self, '%s (%s)' % (sName,sType), sValue, sTip)
			self.dParams[sName] = x
			
			## Rasterize
			y,z = x.widgets()
			y.grid(row=i%10,column=i/10*2+0,sticky=NW,padx=5,pady=5)		
			z.grid(row=i%10,column=i/10*2+1,sticky=NW,padx=5,pady=5)
			i += 1
			
		## Save data when we are unmapped
		self.bind('<Map>',   self.__Map)
		self.bind('<Unmap>', self.__Unmap)
		
		## Pass
		self.SetGfx = self.cmpnt.SetGfx
		
	def __Map(self, e):
		## Load values
		for key in self.cmpnt.dAttrs.keys():
			self.dAttrs[key].set(self.cmpnt.dAttrs[key])
			
		for key in self.cmpnt.dParams.keys():
			self.dParams[key[0]].set(self.cmpnt.dParams[key])	
		
	def __Unmap(self, e):
		## Check name for uniqueness
		name = self.cnfg.GenerateValidName(self.dAttrs['name'].get(), own=self)
		self.UpdateName(name)
		
		## Log the values
		bDirty = False
		for key in self.dAttrs.keys():
			sVal = self.dAttrs[key].get()
			if self.cmpnt.GetAttribute(key) != sVal:
				bDirty = True
				self.cmpnt.SetAttribute(key, sVal)
		for key in self.dParams.keys():
			sVal = self.dParams[key].get()
			if self.cmpnt.GetParameter(key) != sVal:
				bDirty = True
				self.cmpnt.SetParameter(key, sVal)
				
		if bDirty:
			self.master.dirty()

	def GetType(self):
		return self.cmpnt.dAttrs['type']
	
	def GetName(self):
		return self.cmpnt.dAttrs['name']
	
	def GetComponent(self):
		return self.cmpnt
	
	def UpdateName(self, name):
		## Change name
		self.cmpnt.SetName(name)
		self.dAttrs['name'].set(name)
		self.master.dTag2Box[self.name].settext(name)
	
	def destroy(self):
		self.cnfg.RemoveComponent(self.cmpnt)
		Frame.destroy(self)
		
		
class XLine:	
	def __init__(self, master, name, x=0, y=0):
		## Values
		self.master = master # the canvas we reside in
		self.name  = name 	 # the tag we use in our canvas
		self.x = x
		self.y = y
		self.tag1 = None
		self.tag2 = None
		
		## Create the line we are composed of
		id = self.master.create_line(0, 0, 0, 0, 0, 0, 0, 0, arrow=LAST, smooth=False, width=2, fill=sConnection)
		self.master.addtag_withtag(self.name, id)
		
		## Start point
		self.__reshape(0, 0, 0, 0)
		self.set_back(x,y)
		self.set_front(x,y)
				
	def __reshape(self, x1=0, y1=0, x2=0, y2=0):
		'''Internal function: reshapes the line'''
		## Linked to boxes		
		if self.tag1 and self.tag2:
			x11, y11, x12, y12 = self.master.GetBox(self.tag1).box()
			x21, y21, x22, y22 = self.master.GetBox(self.tag2).box()
			w1,h1 = x12 - x11, y12 - y11
			w2,h2 = x22 - x21, y22 - y21
			cx1 = (x11 + x12) / 2
			cy1 = (y11 + y12) / 2
			cx2 = (x21 + x22) / 2
			cy2 = (y21 + y22) / 2
			dcx = cx2 - cx1
			dcy = cy2 - cy1
			sx,sy = cmp(cx1,cx2), cmp(cy1,cy2)
			if not sx: sx = 1
			if not sy: sy = 1
			f = 1.0
			#if dcy > 0.0: 	f = 0.5
			#else:			f = 2.0				
			rx,ry = -(( dcx>f*dcy) + (dcx>-f*dcy) - 1 ), -( (dcx<f*dcy) + (dcx>-f*dcy) - 1 )
			nx1,ny1 = cx1+(rx-sx)*w1/2,cy1+(ry-sy)*h1/2			
			nx3,ny3 = cx2+rx*w2/2,  cy2+ry*h2/2
			if (abs(dcy) < h1/2) or (abs(dcx) < w1/2):
				nx0,ny0 = cx1-rx*w1/2, 				cy1-ry*h1/2
				nx1,ny1 = nx0-rx*abs(nx3-nx0)/2,	ny0-ry*abs(ny3-ny0)/2
				nx2,ny2 = nx1+(rx-sx)*abs(nx3-nx0),	ny1+(ry-sy)*abs(ny3-ny0)	
				self.master.coords(self.name, nx0, ny0, nx1, ny1, nx2, ny2, nx3, ny3)
			else:
				nx2,ny2 = cx1+(rx-sx)*abs(dcx), cy1+(ry-sy)*abs(dcy)
				self.master.coords(self.name, nx1, ny1, nx2, ny2, nx3, ny3)			
		## Free	
		else:		
			## Difference
			dx = x2 - x1
			dy = y2 - y1
			## Steps
			if abs(dx) >= abs(dy):
				nx1,ny1 = x1, y1
				nx2,ny2 = (x1+x2)/2, ny1
				nx3,ny3 = (x1+x2)/2, y2
				nx4,ny4 = x2, y2
			else:
				nx1,ny1 = x1, y1 
				nx2,ny2 = nx1,(y1+y2)/2
				nx3,ny3 = x2, (y1+y2)/2
				nx4,ny4 = x2, y2
		
			self.master.coords(self.name, nx1, ny1, nx2, ny2, nx3, ny3, nx4, ny4)
		
		## Save
		self.x1 = x1
		self.x2 = x2
		self.y1 = y1
		self.y2 = y2
		
	def set_tags(self, tag1, tag2):
		'''Causes the line to start in the box with tag1 and end in the box with tag2.
		The lines takes a fitting path'''
		self.tag1 = tag1
		self.tag2 = tag2
		self.__reshape()
		
	def set_front(self, x, y):
		'''Set the top of the line to (x,y)'''
		self.__reshape(self.x1, self.y1, x, y)
		
	def set_back(self, x, y):
		'''Set the back of the line to (x,y)'''
		self.__reshape(x, y, self.x2, self.y2)
		
	def move_front(self, dx, dy):
		'''Move the top of the line with (dx,dy)'''
		self.__reshape(self.x1, self.y1, self.x2+dx, self.y2+dy)
		
	def move_back(self, dx, dy):
		'''Move the back of the line with (dx,dy)'''
		self.__reshape(self.x1+dx, self.y1+dy, self.x2, self.y2)
	
	def scale_back(self, cx, cy, fx, fy):
		'''Scale the line so that the back is now at (1-f)*c+f*back'''
		dx = self.x1 - cx
		dy = self.y1 - cy
		self.__reshape(cx+fx*dx, cy+fy*dy, self.x2, self.y2)
		
	def scale_front(self, cx, cy, fx, fy):
		'''Scale the line so that the front is now at (1-f)*c+f*front'''
		dx = self.x2 - cx
		dy = self.y2 - cy		
		self.__reshape(self.x1, self.y1, cx+fx*dx, cy+fy*dy)
		
	def coords(self):
		'''Returns the quadruple backx, backy, frontx, fronty'''
		return self.x1, self.y1, self.x2, self.y2
	
	def get_center(self):
		'''Return the center of line as (x,y)'''
		return (self.x1+self.x2)/2, (self.y1+self.y2)/2
		
		
class XBox:
	
	def __init__(self, master, name, x=0.0, y=0.0, width=120.0, height=80.0, text='', type='',
		border=10.0, idot=2.0, fill=sComponent, highlight=sComponentFocus, outline='black', 
		minwidth=20.0, minheight=20.0, maxwidth=750.0, maxheight=750.0, shadow=True):		
		## Values
		self.master = master		
		self.name = name # The tag that we have in our master
		self.idot = idot # pixelsize of the dots to resize our window
		self.minW = minwidth
		self.minH = minheight
		self.maxW = maxwidth
		self.maxH = maxheight
		self.width  = width
		self.height = height
		self.border = border # how far we stay away from the canvas border
		self.fill = fill
		self.highlight = highlight
		self.select = False
		self.shadow = shadow
		
		## Shadow
		if self.shadow:
			id = self.master.create_rectangle(0.0, 0.0, 0.0, 0.0, fill=dark_grey, width=0)
			self.master.addtag_withtag('%s-shadow'%(self.name), id)
			self.master.addtag_withtag(self.name, id)
				
		## Fill the canvas with the box		
		id = self.master.create_rectangle(0.0, 0.0, 0.0, 0.0, fill=self.fill, outline=outline)
		self.master.addtag_withtag('%s-box'%self.name, id)
		self.master.addtag_withtag(self.name, id)
		
		## Create the nine dots
		if self.idot:
			for s in dxy.keys():
				id = self.master.create_rectangle(0.0, 0.0, 0.0, 0.0, fill='black')
				self.master.addtag_withtag('%s-dot-%s'%(self.name,s), id)
				self.master.addtag_withtag('%s-dot'%(self.name), id)
				self.master.addtag_withtag(self.name, id)		
			
		## Text & Type
		id = self.master.create_text(0.0, 0.0, text=' ', width=0.0, justify=CENTER, font=box_font_b, anchor=S)
		self.master.addtag_withtag('%s-text'%self.name, id)
		self.master.addtag_withtag(self.name, id)
		if text: self.settext(text)
		id = self.master.create_text(0.0, 0.0, text=' ', width=0.0, justify=CENTER, font=box_font_i, anchor=N)
		self.master.addtag_withtag('%s-type'%self.name, id)
		self.master.addtag_withtag(self.name, id)
		if type: self.settype(type)
		
		## Resize
		self.resize(width, height)	
		self.set(x,y)
		
		## Add mouse handlers
		self.tag = None
		self.master.tag_bind(self.name, "<Button-1>", self.__lMousePressed, '+')
		self.master.bind("<ButtonRelease-1>", self.__lMouseReleased, '+')
		self.master.bind("<B1-Motion>",	self.__lMouseMotion, '+')
		
	def set(self, x, y):
		'''Put the top-left corner of the box at (x,y)'''
		x0,y0 = self.coords()
		self.move(x-x0,y-y0)
		
	def settext(self, s):
		'''Change the name of the box to s'''
		s = s.replace(' ','\n')
		self.master.itemconfigure('%s-text'%self.name, text=s)
		
	def settype(self, s):
		'''Change the type of the box to s'''
		s = '(%s)' % s
		self.master.itemconfigure('%s-type'%self.name, text=s)
				
	def move(self, dx, dy):
		'''Move the box relatively with (dx,dy)'''
		## Offset
		x0, y0 = self.coords()
		x1, y1 = x0+dx, y0+dy
		## Limits
		x1 = min(max(x1, self.border), self.master.width() - self.width - self.border)
		y1 = min(max(y1, self.border), self.master.height()- self.height- self.border)
		## reposition
		self.master.move(self.name, x1-x0, y1-y0)
		
	def resize(self, width, height):
		'''Resize the box to (width,height) with the top-left corner remaining fixed'''
		## Constrain values
		width  = min(max(width, self.minW),self.maxW)
		height = min(max(height,self.minH),self.maxH)
		
		## New dimensions
		fx = 1.0
		if self.width:
			fx = width/self.width
		fy = 1.0
		if self.height:
			fy = height/self.height
		self.width  = width
		self.height = height
		x0,y0 = self.coords()
		r = self.idot/2.0
		h = self.height/2.0 - r
		w = self.width /2.0 - r
		x = self.width /2.0 + x0
		y = self.height/2.0 + y0
					
		## Fill the canvas with the box
		self.master.coords('%s-box'%self.name,x-w, y-h, x+w, y+h)
		
		## Create the nine dots
		if self.idot:
			for s in dxy.keys():
				dx,dy = dxy[s]
				X = x + dx*w
				Y = y + dy*h
				self.master.coords('%s-dot-%s'%(self.name,s), X-r, Y-r, X+r, Y+r)
		dx,dy = 4,4
		if self.shadow:			
			self.master.coords('%s-shadow'%self.name, x-w+dx, y-h+dy, x+w+dx, y+h+dy)		
			
		## Text
		self.master.itemconfigure('%s-text'%self.name, width=0.95*self.width)
		self.master.coords('%s-text'%self.name, x, y)
		self.master.itemconfigure('%s-type'%self.name, width=0.95*self.width)
		self.master.coords('%s-type'%self.name, x, y)
		
		## We should't be moving the canvas really but some reason there is an odd drift
		## I have no clue why this formula works, I found it emperically
		self.move(-self.idot/2.0-dx,-self.idot/2.0-dy)
		
		## Notify		
		self.master.resized(self.name, fx, fy)	
	
	def __lMousePressed(self, e):
		'''Internal function: handles pressing of the left mouse'''
		ids   = self.master.find_overlapping(e.x, e.y, e.x, e.y)
		valid = self.master.find_withtag('%s-dot'%self.name)
		ids   = set(ids) & set(valid)
	
		if ids:
			self.tag = self.master.gettags(ids.pop())[0]
		elif not self.tag:
			self.startDrag(e.x, e.y)

	def __lMouseMotion(self, e):
		'''Internal function: handles motion of the left mouse'''
		## Ignore random dragging
		if not self.tag: return

		## Only move in drag mode
		if self.master.mode != 'drag': return
		
		## State changed
		self.master.dirty(bArgs=False)
		
		if self.tag == 'box':
			## We are dragging the window
			dx,dy = self.drag
			self.set(e.x-dx, e.y-dy)	
		else:			
			## We are resizing the window
			#x0,y0 = self.coords()
			dx,dy,z,z = self.master.coords(self.tag)
			sx,sy = dxy[ self.tag[self.tag.rfind('-')+1:] ]
			dx = e.x - dx
			dy = e.y - dy
			W,H = self.width,self.height
			self.resize(self.width+sx*dx, self.height+sy*dy)
			
			## Actual size difference (is different from W&H when reaching limits)
			dx,dy = sx*(self.width - W), sy*(self.height - H)
			
			## We control our translation with the master
			tx,ty = 0.0,0.0
			if sy == -1.0:
				ty = dy
			if sx == -1.0:
				tx = dx
			## I have no clue why this formula works, I found it emperically
			self.move(tx,ty)
				
	def __lMouseReleased(self, e):
		'''Internal function: handles release of the left mouse'''
		self.tag = None		
			
	def startDrag(self, x1, y1):
		x0, y0 = self.coords()
		self.tag  = 'box'			
		self.drag = x1-x0, y1-y0	
		
	def coords(self):
		'''Returns the top-left corner of the box as a tuple (x,y)'''
		x, y, z, z = self.master.coords(self.name)
		return x, y
	
	def front(self):
		'''Return the front(middle-right) of the box as a tuple (x,y)'''
		x,y = self.coords()
		return x+self.width-1.0,y+self.height/2.0-1.0
	
	def back(self):
		'''Return the back(middle-left) of the box as a tuple (x,y)'''
		x,y = self.coords()
		return x,y+self.height/2.0
	
	def shape(self, xa, ya, xb, yb):
		'''	Shape the box so that the top-left corncer matches (xa,ya)
			and the bottom-right corner matches (xb,yb)'''
		x1 = min(xa,xb)
		y1 = min(ya,yb)
		x2 = max(xa,xb)
		y2 = max(ya,yb)
		self.resize(x2-x1,y2-y1)
		self.set(x1,y1)
		
	def selected(self, bool):
		'''Set wether or not the box is selected with the boolean bool'''
		self.select = bool
		if not bool:
			self.master.itemconfigure('%s-box'%self.name, fill=self.fill)
		else:
			self.master.itemconfigure('%s-box'%self.name, fill=self.highlight)
			
	def isSelected(self):
		'''Returns a boolean, indicating if the box is selected'''
		return self.select
	
	def box(self):
		'''Returns a tuple (x1,y1,x2,y2) defining the top-left and bottom-right
		corners of the inner rectangle'''
		return self.master.coords('%s-box'%self.name)


class XCanvas(Canvas):
		
	def __init__(self, ms, width, height, **kw):
		Canvas.__init__(self, ms, **kw)
		
		## Dimensions
		self.w = width
		self.h = height
		self['width']  = self.w
		self['height'] = self.h
		
		## Mouse button manager
		self.bind("<Button-3>",			self.__rMouse, '+')		
		self.bind("<Button-1>", 		self.__lMousePressed, '+')
		self.bind("<ButtonRelease-1>",  self.__lMouseReleased, '+')
		self.bind("<B1-Motion>", 		self.__lMouseMotion, '+')
		self.bind('<Double-Button-1>',  self.__l2Mouse,'+')
				
		## Our right click menu
		self.__mkMenu()
		
		## Various attributes
		self.mode = 'drag'
		self.icmpnt = 0
		self.iconn  = 0
		self.line = None
		self.rect = None
		self.tag = ''
		self.iMenuCreateX = 0
		self.iMenuCreateY = 0
		
		## Link canvas objects to other classes
		self.dTag2Cmpnt = {}
		self.dTag2Box   = {}
		self.dTag2Line  = {}
		self.dTag2Conn	= {}
		
		## Redirect
		self.dirty = self.master.dirty
		
	def SaveGfx(self):
		'''Calls the SetGfx for component and connector abouts its box and line respectively'''
		## Components
		for tag in self.dTag2Cmpnt.keys():
			box = self.dTag2Box[tag]
			x, y = box.coords()
			self.dTag2Cmpnt[tag].SetGfx(x, y, box.width, box.height)
			
		## Lines
		for key in self.dTag2Line:
			oLine = self.dTag2Line[key]
			oConn = self.dTag2Conn[oLine.name]
			x1, y1, x2, y2 = oLine.coords()
			oConn.SetGfx(x1, y1, x2-x1+1, y2-y1+1)
			
	def GetBox(self, sName):
		'''Return the box linked with a tag or named component sName'''
		## We need to convert these screennames to tags (if nec
		if sName in self.dTag2Box:
			return self.dTag2Box[sName]
		for tag in self.dTag2Cmpnt.keys():
			if self.dTag2Cmpnt[tag].GetName() == sName:
				return self.dTag2Box[tag]
			
	def __getLine(self, sName):
		'''Internal function: get a line'''
		tag1 = self.dTag2Conn[sName].tag1
		tag2 = self.dTag2Conn[sName].tag2
		return self.dTag2Line[tag1,tag2]
			
	def Rasterize(self):
		'''Spread all boxes over the canvas'''
		n = len(self.dTag2Box)
		if not n: return
		dx = self.width()/((n-1)/5+2)
		dy = 100		
		i = 0
		for tag in self.dTag2Box:
			self.dTag2Box[tag].set(i/5*dx+25, i%5*dy+25)
			i += 1
									
	def __mouse_tag(self, e, s=''):
		'''Internal function: find the most front box or line at the mouse event'''
		ids  = list(self.find_overlapping(e.x, e.y, e.x, e.y))
		ids.sort()
		tags = reduce(lambda x,y: x + list(self.gettags(y)), ids, [])
		if tags.count('current'):
			tags.remove('current')		
		if ids:
			tags = filter(lambda x: x.find(s) != -1, tags)
			if tags:
				if tags[-1].find('-') == -1:
					return tags[-1]
				else:
					return tags[-1][:tags[-1].find('-')]
		return ''
	
	def __mux_tag(self, tagorid, s=''):
		'''Internal function: find the boxes or lines inside the item tagorid'''
		x1, y1, x2, y2 = self.bbox(tagorid)
		ids = list(self.find_enclosed(x1, y1, x2, y2))
		tags = reduce(lambda x,y: x + self.gettags(y), ids, ())
		ret = []
		if ids:
			tags = filter(lambda x: x.find(s) != -1, tags)
			for tag in tags:
				if tag.find('-') == -1:
					ret.append(tag)
				else:
					ret.append(tag[:tag.find('-')])
		return ret
	
	def __rMouse(self, e):
		'''Internal function: handles clicking of the right mouse'''
		## Where did we click?
		tag = self.__mouse_tag(e)
		if tag != self.tag:
			self.__hd()
		self.tag = tag
		
		## Check highlighting
		self.__Highlight()
		
		if not self.tag:
			self.__shwMenuCreate(e)			
		elif self.__mouse_tag(e, 'cmpnt'):						
			self.__shwMenuBox(e)
		elif self.__mouse_tag(e, 'line'):
			self.__shwMenuLine(e)
		
	def __lMousePressed(self, e):
		'''Internal function: handles clicking of the left mouse'''
		## Where did we click?
		tag = self.__mouse_tag(e)
		if tag != self.tag:
			self.__hd()
		self.tag = tag
		
		## Check highlighting
		self.__Highlight()
			
		## Which mode?
		if self.mode == 'connect' and self.__mouse_tag(e, 'cmpnt'):
			## Create a connector
			self.tag1 = self.tag
			self.iconn += 1
			self.line = XLine(self,'line%s'%self.iconn,x=e.x,y=e.y)
		elif self.mode == 'drag' and not self.__mouse_tag(e, 'cmpnt'):
			## Ok we are drawing a rectangle			
			self.rect = XBox(self, 'rect', x=e.x, y=e.y, width=0, height=0, 
			  				 idot=0, fill='', highlight='', outline='grey',
			  				 minwidth=0, minheight=0, maxwidth=1024, maxheight=1024,
			  				 shadow=False)
			self.rect_x = e.x
			self.rect_y = e.y
		elif self.mode == 'drag' and self.__mouse_tag(e, 'cmpnt'):
			x = self.__mouse_tag(e, 'dot')
			if not x:
				## Dont select when the user is reshaping a box
				for tag in self.dTag2Box.keys():
					box = self.dTag2Box[tag]
					if box.isSelected():
						box.startDrag(e.x, e.y)
					
	def __lMouseMotion(self, e):
		'''Internal function: handles motion of the left mouse'''
		## Reshape
		if self.line:
			self.line.set_front(e.x, e.y)
		if self.rect:
			self.rect.shape(self.rect_x, self.rect_y, e.x, e.y)
			
			## Highlight selected
			tags = self.__mux_tag('rect','cmpnt')
			for tag in self.dTag2Box.keys():
				self.dTag2Box[tag].selected(False)
			for tag in tags:
				self.dTag2Box[tag].selected(True)
			
	def __lMouseReleased(self, e):
		'''Internal function: handles release of the left mouse'''
		## Only keep the connector if he was dropped in a box
		if self.line:
			self.tag2 = self.__mouse_tag(e, 'cmpnt')
			if not self.tag2 or self.tag2 == self.tag1 or (self.tag1,self.tag2) in self.dTag2Line:
				self.delete('line%s'%self.iconn)
				self.iconn -= 1
			else:
				self.__mkConn()
		self.line = None
		
		if self.rect:
			self.delete('rect')
		self.rect = None
			
	def __l2Mouse(self, e):
		'''Internal function: handles double click of the left mouse'''
		## Where did we click?
		self.tag = self.__mouse_tag(e)
		
		## Hide & Show
		self.__hd()
		
		if self.__mouse_tag(e,'cmpnt'):
			self.__shwInnerBox()
		elif self.__mouse_tag(e,'line'):
			self.__shwInnerLine()
			
	def __Highlight(self):
		'''Internal function: resets the highlightning when you clicked outside
		the current selection'''
		if (not self.tag in self.dTag2Box) or (not self.dTag2Box[self.tag].isSelected()):
			for tag in self.dTag2Box.keys():
				self.dTag2Box[tag].selected(False)
			if self.tag in self.dTag2Box:
				self.dTag2Box[self.tag].selected(True)
			
	def resized(self, tag, fx, fy):
		'''Canvas item with tag resized itself with factor (fx,fy), stick lines to their box'''
		## On of our cmpnts resized
		for tag1,tag2 in self.dTag2Line.keys():
			oLine = self.dTag2Line[tag1,tag2]
			if tag == tag1:
				x, y = self.dTag2Box[tag].coords()
				oLine.scale_back(x, y, fx, fy)	
				oLine.move_back(+1,+1)			
			if tag == tag2:
				x, y = self.dTag2Box[tag].coords()
				oLine.scale_front(x, y, fx, fy)
				oLine.move_front(+1,+1)
							
	def move(self, tag, dx, dy):
		'''Canvas item with tag moved itself with offset (dx,dy), stick lines to their box'''
		## Base function
		Canvas.move(self, tag, dx, dy)
		
		## On of our cmpnts repositioned
		for tag1,tag2 in self.dTag2Line.keys():
			oLine = self.dTag2Line[tag1,tag2]
			if tag == tag1:
				oLine.move_back(dx,dy)
			if tag == tag2:
				oLine.move_front(dx,dy)
			
	## Make fixed parts
	def __mkMenu(self):
		## Create a right menu to select a new component
		self.fMenu1 = Frame(self.master)
		self.mMenu1 = Menu(self.fMenu1, tearoff=0, takefocus=0)
		self.vMenu1 = IntVar()
		
		## Make a menu for each type
		dMenus = {}
		for sClass in g_lBaseComponents:
			## Make the submenu			
			oMenu = Menu(self.mMenu1, tearoff=0, takefocus=0)
			
			## Attach and save it
			dMenus[sClass] = oMenu			
			self.mMenu1.add_cascade( label=sClass, menu=oMenu )
				
		## Add all classes
		for iClass,sClass in enumerate(g_lComponents):
			## Find out the name of the parent class
			sParent = g_dComponents[sClass].__bases__[0].__name__
			sParent = sParent.replace( '_', '-')
			sSuper = g_dBaseComponents[sParent].__name__
			sSuper = sSuper.replace( '_', '-')
			if sSuper in dMenus:
				dMenus[sSuper].add_radiobutton(label=sClass, variable=self.vMenu1, value=iClass, command=self.__mkComp)
			else:
				print 'Could not find menu %s for %s' % (sSuper,sClass)
					
		## Create a right menu to select box options
		self.fMenu2 = Frame(self.master)
		self.mMenu2 = Menu(self.fMenu2, tearoff=0, takefocus=0)
		self.vMenu2 = IntVar()
		j = 0
		for s in ('open', 'delete', 'copy'):
			self.mMenu2.add_radiobutton(label=s, variable=self.vMenu2, value=j, command=self.__cnfComp)		
			j += 1
		self.mMenu2.add_separator()
		self.mMenu2.add_radiobutton(label='drag',    variable=self.master.mode, value=0, command=self.master.SetMode)
		self.mMenu2.add_radiobutton(label='connect', variable=self.master.mode, value=1, command=self.master.SetMode)
		
		## Create a right menu to select box options
		self.fMenu3 = Frame(self.master)
		self.mMenu3 = Menu(self.fMenu3, tearoff=0, takefocus=0)
		self.vMenu3 = IntVar()
		j = 0
		for s in ('open', 'delete'):
			self.mMenu3.add_radiobutton(label=s, variable=self.vMenu3, value=j, command=self.__cnfConn)		
			j += 1
		self.mMenu3.add_separator()
		self.mMenu3.add_radiobutton(label='drag',    variable=self.master.mode, value=0, command=self.master.SetMode)
		self.mMenu3.add_radiobutton(label='connect', variable=self.master.mode, value=1, command=self.master.SetMode)
			
	def __cnfComp(self):
		'''Internal function: handles the component menu'''
		## What options?
		cnf = ('open', 'delete', 'copy')[self.vMenu2.get()]
		
		## Hide all the rest
		self.__hd()
				
		## Action
		if cnf == 'open':
			self.__shwInnerBox()
		elif cnf == 'delete':
			self.__delComp()
		elif cnf == 'copy':
			self.__cpyComp()
			
	def __cnfConn(self):
		'''Internal function: handles the connector menu'''
		## What options?
		cnf = ('open', 'delete')[self.vMenu3.get()]
		
		## Hide all the rest
		self.__hd()
				
		## Action
		if cnf == 'open':
			self.__shwInnerLine()
		elif cnf == 'delete':
			self.__delConn()
			
	def SqrComp(self, w=130.0, h=75.0):
		'''Set all boxes to w,h dimesnions'''
		for box in self.dTag2Box.values():
			box.resize(w,h)			
			
	def __delComp(self):
		'''Internal function: deletes a component/box'''
		self.master.dirty()
		self.delete(self.tag)
		self.dTag2Cmpnt[self.tag].destroy()
		del self.dTag2Cmpnt[self.tag]
		del self.dTag2Box[self.tag]
		
		## Delete lines
		for key in self.dTag2Line.keys():
			if self.tag in key:
				self.delete(self.dTag2Line[key].name)
				self.dTag2Conn[self.dTag2Line[key].name].destroy()
				del self.dTag2Conn[self.dTag2Line[key].name]
				del self.dTag2Line[key]
				
	def __delConn(self):
		'''Internal function: deletes a connector/line'''
		self.master.dirty(bArgs=False)
		self.delete(self.tag)
		oConn = self.dTag2Conn[self.tag]
		del self.dTag2Conn[self.tag]
		del self.dTag2Line[oConn.tag1, oConn.tag2]
		oConn.destroy()		
				
	def delAllCmpnts(self):
		'''Delete all components and hence all connectors from the xcanvas'''
		for tag in self.dTag2Box.keys():
			self.delete(tag)
			self.dTag2Cmpnt[tag].destroy()
		self.dTag2Cmpnt = {}
		self.dTag2Box   = {}
		
		## Killing all components, kills all lines aswell
		self.delAllLines()
			
	def delAllLines(self):
		'''Delete all connectors/lines from the xcanvas'''
		for key in self.dTag2Line.keys():
			self.delete(self.dTag2Line[key].name)
			self.dTag2Conn[self.dTag2Line[key].name].destroy()
		self.dTag2Line = {}
		self.dTag2Conn = {}
		
	def ldComp(self, type, name, x, y, w, h):
		'''Create component & box with name, type, top-left corner at (x,y),
		width w and height h. Returns the new component'''
		## Make a new component
		s = 'cmpnt%d'%self.icmpnt
		self.icmpnt += 1
		xcmpnt = XComponent(self, self.master.cnfg, s, name, type)		
	
		## Create a box	
		box = XBox(self, s, x=x, y=y, width=w, height=h, text=xcmpnt.GetName(), type=type)
				
		## Link em
		self.dTag2Cmpnt[s] = xcmpnt
		self.dTag2Box[s]   = box
		return xcmpnt
				
	def ldConn(self, name1, name2, x0, y0, x1, y1):
		'''Create a connector & line between components with name name1 & name2, back of the line at (x0,y0)
		and top of the line at (x1,y1). Returns the new connector.'''
		## We need to convert these screennames to tags
		for tag in self.dTag2Cmpnt.keys():
			if self.dTag2Cmpnt[tag].GetName() == name1:
				tag1 = tag
			if self.dTag2Cmpnt[tag].GetName() == name2:
				tag2 = tag
		
		line = XLine(self,'line%s'%self.iconn, x0, y0)
		self.iconn += 1
		line.set_front(x1, y1)		
		x = XConnector(self, self.master.cnfg, self.dTag2Cmpnt[tag1], self.dTag2Cmpnt[tag2])
		self.dTag2Conn[line.name] = x
		self.dTag2Line[tag1,tag2] = line
		line.set_tags(tag1, tag2)
		return x
			
			
	def __cpyComp(self):
		'''Internal function: copy a component/box'''		
		## What type we selected?
		oXOtherComponent = self.dTag2Cmpnt[self.tag]
			
		## Make a new component
		s = 'cmpnt%d'%self.icmpnt
		self.icmpnt += 1
		oXComponent = XComponent(self, self.master.cnfg, s, '', oXOtherComponent)
	
		## Coordinates of the new component
		pOtherBox = self.dTag2Box[self.tag]
		w,h = pOtherBox.width, pOtherBox.height
		x0,y0 = pOtherBox.coords()
		if (x0 + 2*w) < int(self['width']): 
			x = x0 + w + 10
		else:
			x = x0 - w - 10
		y = y0
		
		## Create a box				
		pBox = XBox(self, s, x=x, y=y, width=w, height=h, text=oXComponent.GetName(), type=oXComponent.GetType())
				
		## Link em
		self.dTag2Cmpnt[s] = oXComponent
		self.dTag2Box[s]   = pBox	
		self.__hd()
	
	def __mkComp(self):
		'''Internal function: make a new component/box'''
		## What type we selected?
		sType = g_lComponents[self.vMenu1.get()]
		
		## Dirty flag
		self.master.dirty(bArgs=False)
				
		## Make a new component
		s = 'cmpnt%d'%self.icmpnt
		self.icmpnt += 1
		oXComp = XComponent(self, self.master.cnfg, s, sType, sType)
		
		## Coordinates of the menu
		x = self.iMenuCreateX - self.winfo_rootx()
		y = self.iMenuCreateY - self.winfo_rooty()	
		
		## Create a box	
		pBox = XBox(self, s, x=x, y=y, width=iDefaultWidth, height=iDefaultHeight, text=oXComp.GetName(), type=sType)
		
		## Link em
		self.dTag2Cmpnt[s] = oXComp
		self.dTag2Box[s] = pBox		
		
	def __mkConn(self):
		'''Internal function: make a connector'''
		self.master.dirty(bArgs=False)
		x = XConnector(self, self.master.cnfg, self.dTag2Cmpnt[self.tag1], self.dTag2Cmpnt[self.tag2])
		self.dTag2Conn[self.line.name] = x
		self.dTag2Line[self.tag1,self.tag2] = self.line
		self.line.set_tags(self.tag1, self.tag2)
				
	## Show & hide parts
	def __shwInnerBox(self):
		'''Internal function: map the component linked to the box''' 
		self.delete('inner')
		x, y, u ,v = self.bbox(self.tag)
		id = self.create_window(0, 0, window=self.dTag2Cmpnt[self.tag], anchor=NW)
		self.addtag_withtag('inner', id)
		x,y = self.__checkCoords('inner', (x+u)*0.50, 0.25*y+0.75*v)
		self.coords('inner', x, y)	
				
	def __shwInnerLine(self):
		'''Internal function: map the connector linked to a line'''
		self.delete('inner')
		x, y, u ,v = self.bbox(self.tag)
		id = self.create_window(0, 0, window=self.dTag2Conn[self.tag], anchor=NW)
		self.addtag_withtag('inner', id) 
		x,y = self.__checkCoords('inner', (x+u)*0.50, (y+v)*0.50)
		self.coords('inner', x, y)	
		
	def __shwMenuLine(self, e):
		'''Internal function: map the menu to modify the selected box'''
		## The menu is nice, but the placement is quite bogus
		h = 75
		H = self.winfo_screenheight()
		x = e.x_root+32
		y = e.y_root+10
		if y > (H - h - 50):
			y = H - h - 50
		self.mMenu3.tk_popup(x, y, entry=0)
		
#		self.delete('menuright3')
#		id = self.create_window(0, 0, window=self.fMenu3, anchor=NW)
#		self.addtag_withtag('menuright3', id)
#		x, y = self.__checkCoords( 'menuright3', x, y)
#		self.coords('menuright3', x, y)
		
	def __shwMenuCreate(self, e):
		'''Internal function : map the menu to create new boxes'''
		## The menu is nice, but the placement is quite bogus
		h = 350
		H = self.winfo_screenheight()
		x = e.x_root
		y = e.y_root
		if y > (H - h - 50):
			y = H - h - 50
		self.mMenu1.tk_popup(x, y, entry=0)
		
		## Store the mouse pointer
		self.iMenuCreateX, self.iMenuCreateY = x,y
		
	def __shwMenuBox(self, e):
		'''Internal function: map the menu to modify the selected box'''
		## The menu is nice, but the placement is quite bogus
		h = 75
		H = self.winfo_screenheight()
		x = e.x_root+32
		y = e.y_root+32
		if y > (H - h - 50):
			y = H - h - 50
		self.mMenu2.tk_popup(x, y, entry=0)
		
#		self.delete('menuright2')
#		id = self.create_window(0, 0, window=self.fMenu2, anchor=NW)
#		self.addtag_withtag('menuright2', id)
#		x, y = self.__checkCoords( 'menuright2', e.x, e.y)
#		self.coords('menuright2', x, y)
		
	def __checkCoords(self, tag, x0, y0):
		'''Internal function: checks if an item can be placed at x0,y0 without going out of bound'''
		x1, y1, x2, y2 = self.bbox(tag)
		w = x2-x1
		h = y2-y1
		if x0+w > self.w:
			x0 -= w
		if y0+h > self.h:
			y0 -= h
		return x0,y0
			
	def __hd(self):
		'''Internal function: hide all menus'''
		self.delete('menu')
		self.delete('inner')
		
	def width(self, tag=None):
		if not tag:
			return int(self['width'])
		else:
			return self.dTag2Box[tag].width
		
	def height(self, tag=None):
		if not tag:
			return int(self['height'])
		else:
			return self.dTag2Box[tag].height
		
	def __mouseCoords(self):
		'''Internal function which gives the mouse pointer, relative to the canvas'''
		x, y = self.winfo_pointerxy()
		x -= self.winfo_rootx()
		y -= self.winfo_rooty()
		return x,y
					
def YesNoCancel(title, message, **options):
	options["icon"] = 'question'
	options["type"] = 'yesnocancel'
	options["title"] = title
	options["message"] = message
	return Message(**options).show()
