#!/usr/bin/env python
# requires:
#	Python 2.5
# 	numpy
#	matplotlib

from Tkinter import *
from pylab import *
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2TkAgg
import math

class XRoot(Tk):
	def __init__(self, sFile, sStep):
		Tk.__init__(self)		
		self.oAnim = XStats(self, sFile, sStep)
		self.oAnim.plot()
		self.oAnim.grid()		

class XStats(Frame):	
	def __init__(self, master, sFile, sStep, **kwargs):
		Frame.__init__(self, master, **kwargs)
		
		## Matplot lib structures
		self.oFigure = Figure((12,9), 75)
		self.oGraph = FigureCanvasTkAgg(self.oFigure, self)		
		self.oCanvas = self.oGraph.get_tk_widget()
		oFrame = Frame(self)
		self.oToolbar = NavigationToolbar2TkAgg(self.oGraph,oFrame)
		oFrame.grid(row=2, column=0, sticky=W )		
		self.oCanvas.grid(row=1, column=0)
		self.oAxes = self.oFigure.add_subplot(111)
		
		## Data
		self.iStep = int(sStep)
		if self.iStep <= 0:
			raise ValueError, "step must be strictly positive: %d" % self.iStep
		self.dData = {}
		
	def plot(self):
		## Open the file
		oFile = file(sFile, 'r')
		
		## Parse file
		for sLine in oFile:
			## Parse line
			lItems = sLine.split(',')
			if len(lItems) != 5:
				break
			
			## Information
			sName = lItems[0]
			iSend = int(lItems[1])
			iSize = int(lItems[3])
			
			## Store
			self.dData.setdefault(sName,[]).append((iSend,iSize))
				
		## Loop over the different curves
		lColors = ['red', 'green', 'blue']
		lColors = ['0.3', '0.0', '0.5']
		lLines = [ '-^', '-s', '-d' ]
		i = -1
		for sName in self.dData.keys():
			lData = self.dData[sName]			
			iTime0 = lData[0][0]
			iTime = 0
			iKey = 0
			iMax = len(lData)
			lX = []
			lY = []
			i += 1
			while(iKey < iMax):
				iSize = 0
				while(iKey < iMax and lData[iKey][0] < iTime):
					iSize += lData[iKey][1]
					iKey += 1
					
				## Log data for this interval
				lX.append((iTime - iTime0) / 1000.)
				lY.append(iSize * 8. / self.iStep)
				
				## New time
				iTime += self.iStep

			## Plot
			if not sName:
				sName = 'stream' 
			self.oAxes.plot(lX, lY, lLines[i], label=sName, color=lColors[i])
								
		## Axes & grid
		#self.oAxes.set_title('bandwidth', fontsize='xx-large')
		self.oAxes.set_xlabel('time (s)', fontsize='xx-large')
		self.oAxes.set_ylabel('bandwidth (kbps)', fontsize='xx-large')
		self.oAxes.grid(True)	
		self.oAxes.legend()	
		self.oGraph.draw()	
		#self.oAxes.xaxis.set_minor_locator(MultipleLocator(0.640))		
		#self.oAxes.yaxis.set_minor_locator(MultipleLocator(20))
		#self.oAxes.xaxis.set_major_locator(MultipleLocator(0.640))
		#self.oAxes.set_yticklabels( [str(i) for i in range(0,8001,1000)], fontsize='x-large')
		#self.oAxes.set_xticklabels([x.get_text() for x in self.oAxes.get_xticklabels()], fontsize='x-large')
		#self.oAxes.set_ylim((0,8000))
		#self.oAxes.set_xlim((0,30))
		
	
if __name__ == '__main__':
	import sys
	sFile = sys.argv[1]
	sStep = sys.argv[2] if len(sys.argv) > 2 else '40'
	oRoot = XRoot(sFile,sStep)
	oRoot.mainloop()
