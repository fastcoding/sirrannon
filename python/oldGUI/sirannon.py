#!/usr/bin/env python
from components_gen import *

class Configuration:
	
	def __init__(self, sFile=None):
		## Lists
		self.lComponents = []
		self.lRoutes	 = []
		self.oGfx = None
		self.sFile = sFile
		
	def MakeComponent(self, sType, sName):
		name = self.GenerateValidName(sName)
		
		sType = sType.lower()
		oCmpnt = None
		for sKey in g_dComponents.keys():
			if sKey.lower() == sType:
				oCmpnt = g_dComponents[sKey](name)
		if not oCmpnt:
			raise ValueError, "Component %s not found" % sType
		self.lComponents.append(oCmpnt)
		return oCmpnt
	
	def AddComponent(self, oCmpnt):
		self.lComponents.append(oCmpnt)
			
	def RemoveComponent(self, oCmpnt):
		self.lComponents.remove(oCmpnt)
		
	def MakeRoute(self, oCmpnt1, oCmpnt2):
		oRoute = Route(oCmpnt1, oCmpnt2)
		self.lRoutes.append(oRoute)
		return oRoute
		
	def RemoveRoute(self, oRoute):
		self.lRoutes.remove(oRoute)
		
	def IsEmpty(self):
		return not (self.lComponents or self.lRoutes)
	
	def SetQuantum(self, iVal):
		self.oGfx.SetQuantum(iVal)
		
	def GetQuantum(self):
		return self.oGfx.GetQuantum()
	
	def SetSimulation(self, iVal):
		self.oGfx.SetSimulation(iVal)
		
	def GetSimulation(self):
		return self.oGfx.GetSimulation()
	
	def SetSeed(self, iVal):
		self.oGfx.SetSeed(iVal)
		
	def GetSeed(self):
		return self.oGfx.GetSeed()
	
	def GetCommandLineArgs(self):
		dArgs = {}
		for oCmpnt in self.lComponents:
			for sVal in oCmpnt.dParams.values():
				if sVal and sVal[0] == '$':
					dArgs[sVal] = 0
		return dArgs.keys()
		
	def GenerateXML(self, sFile):
		return self.__GenerateXML(sFile)
	
	def __GenerateXML(self, sFile=''):
		## Generate
		sHeader  = '<config quantum="%d" simulation="%d" seed="%d">\n'
		sBody1	 = '%s\n' * len(self.lComponents)
		sBody2	 = '%s\n' * len(self.lRoutes)
		sTrailer = '</config>'		
		s  = sHeader % (self.GetQuantum(),self.GetSimulation(),self.GetSeed())
		s += sBody1 % tuple( [ x.GenerateXML() for x in self.lComponents ] )
		s += '\n'
		s += sBody2 % tuple( [ x.GenerateXML() for x in self.lRoutes ] )
		s += sTrailer		
		
		## Save & close
		if sFile:
			f = file(sFile,'w+')
			f.write(s)
			f.close()
		return s
	
	def GenerateValidName(self, name, own=None, other=None):
		names = [ x.GetName() for x in self.lComponents]
		if own:
			names.remove(own.GetName())			
		if name not in names:
			return name
		else:
			base = name
			if base.rfind(' ') != -1:
				base = base[:base.rfind(' ')]
			i = 1
			if base in names:
				if other:
					name = '%s %d' % (base, i)			
					while(name in names):
						i += 1
						name = '%s %d' % (base ,i)
					other.UpdateName(name)
			i += 1
			name = '%s %d' % (base ,i)
			while(name in names):
				i += 1
				name = '%s %d' % (base ,i)				
			return name
			
	def Draw(self):
		import xgfx
		self.oGfx = xgfx.XConfiguration(self)
		self.oGfx.Go()
		
		## Open file defined in command line
		if self.sFile:
			self.LoadXML(self.sFile)
		
	def LoadXML(self, sFile):
		## Clear everything
		self.oGfx.canv.delAllCmpnts()
	
		## Merge with blank
		return self.MergeXML(sFile)
				
	def MergeXML(self, sFile):
		## Open the file
		from xml.parsers import expat		
		self.xml = expat.ParserCreate()		
		oFile = file(sFile,'r+')
							
		## Handlers and values
		self.mode = ''
		self.lattrs = []
		self.xattrs = {}
		self.xml.StartElementHandler = self.start_element		
		self.xml.EndElementHandler   = self.end_element
		
		## Parse
		try:
			self.xml.ParseFile(oFile)
		finally:
			oFile.close()
			
		## Save its values
		return self.__GenerateXML()
	
	def clear_element(self):
		self.mode = ''
		self.lattrs = []
		self.xattrs = {}
		self.attrs  = {}

	def start_element(self, name, attrs):
		if name == 'config':
			## Quantum value
			if 'quantum' in attrs:
				self.SetQuantum(int(float(attrs['quantum'])))
			if 'simulation' in attrs:
				self.SetSimulation(int(float(attrs['simulation'])))
			if 'seed' in attrs:
				self.SetSeed(int(attrs['seed']))
		
		if name == 'component':
			## Check if the element param is nested inside a component element
			if self.mode:
				raise SyntaxError, 'Error validating xml-file: illegal nesting of component on line %s' % self.xml.CurrentLineNumber
			
			## Creat the cmpnt
			self.mode = 'cmpnt'
			self.attrs = attrs.copy()				

		elif name == 'param':
			## Check if the element param is nested inside a component element
			if not self.mode == 'cmpnt':
				raise SyntaxError, 'Error validating xml-file: param not nested inside a component on line %s' % self.xml.CurrentLineNumber
				
			## Set parameter
			self.lattrs.append(attrs.copy())
					
		elif name == 'gfx':
			## Check if the element param is nested inside a component element
			if (not self.mode == 'cmpnt') and (not self.mode == 'rt'):
				raise ValueError, 'Error validating xml-file: gfx not nested inside a component on line %s' % self.xml.CurrentLineNumber
			
			## Save data
			self.xattrs = attrs.copy()
			for x in self.xattrs.keys():
				try:
					self.xattrs[x] = float(self.xattrs[x])
				except:
					raise TypeError, 'Error validating xml-file: non-float value %s' % self.attrs[x]
						
		elif name == 'route':
			## Check if the element param is nested inside a component element
			if self.mode:
				raise SyntaxError, 'Error validating xml-file: illegal nesting of route on line %s' % self.xml.CurrentLineNumber
			
			## Save data
			self.mode = 'rt'
			self.attrs = attrs.copy()
		
	def end_element(self, name):
		if name == 'component':
			## Create the component
			xattrs = self.xattrs
			if not xattrs:
				## This file was created manually, select default locations
				xattrs['x'] = (len(self.lComponents) / 5)*100 + 25
				xattrs['y'] = (len(self.lComponents) % 5)*100 + 25
				xattrs['w'] = 75
				xattrs['h'] = 75
			attrs  = self.attrs		
			xcmpnt = self.oGfx.canv.ldComp(attrs['type'], attrs['name'], xattrs['x'], xattrs['y'], xattrs['w'], xattrs['h'])
								
			## Set attributes
			for attr in attrs.keys():
				xcmpnt.cmpnt.AddAttribute(attr, attrs[attr])
							
			## Set parameters
			for pattrs in self.lattrs:
				## Check if attribute exists
				if not xcmpnt.cmpnt.HasParameter(pattrs['name'], pattrs['type']):
					print 'Warning, parameter %s not in syntax for component %s' % (pattrs['name'], xcmpnt.cmpnt.GetName())
				else:
					xcmpnt.cmpnt.AddParamater(pattrs['name'], pattrs['type'], pattrs['val'])
			self.clear_element()
			#print xcmpnt.cmpnt.dParams
				
		elif name == 'route':
			## Creat the route
			xattrs = self.xattrs
			attrs  = self.attrs
			if not xattrs:
				## This file was created manually, select default locations
				x1,y1 = self.oGfx.canv.GetBox(attrs['from']).front()
				x2,y2 = self.oGfx.canv.GetBox(attrs['to']).back()
				xattrs['x'] = x1
				xattrs['y'] = y1
				xattrs['w'] = x2-x1
				xattrs['h'] = y2-y1
			xroute = self.oGfx.canv.ldConn(attrs['from'], attrs['to'], xattrs['x'], xattrs['y'], xattrs['x']+xattrs['w'], xattrs['y']+xattrs['h'])
			
			## Set attributes
			if 'xroute' in attrs:
				xroute.route.Set(attrs['xroute'])			
			self.clear_element()
					
class Route:	
	def __init__(self, oCmpnt1, oCmpnt2, xroute=0):
		self.oCmpnt1 = oCmpnt1
		self.oCmpnt2 = oCmpnt2
		self.xroute	 = xroute
		self.oGfx	= None
		
	def GetName(self):
		return '%s2%s' % (self.oCmpnt1.GetName(), self.oCmpnt2.GetName())
		
	def Set(self, xroute):
		self.xroute = xroute
		
	def Get(self):
		return self.xroute
		
	def SetGfx(self, x, y, w, h):
		self.oGfx = x, y, w, h
		
	def GenerateXML(self):
		a = '\t<route from="%s" to="%s" xroute="%s"/>\n'		
		s  = a % (self.oCmpnt1.GetName(), self.oCmpnt2.GetName(), self.xroute)	
		return s

class Component:
	def __init__(self, sType, sName):
		## Dicts
		self.dAttrs  = {}
		self.lAttrs  = [] # need to use list because a dict forgets the insertion order
		self.dParams = {}
		self.lParams = [] # need to use list because a dict forgets the insertion order
		self.dInfo = {}
		
		## Forced Attributes
		self.AddAttribute('name', sName)
		self.AddAttribute('type', sType)
		
		## Forced parameters
		#self.AddParamater('debug', 'bool', 'false', )
		#self.AddParamater('thread', 'bool', 'false',)
		
		## Gfx
		self.sTip = ""
		self.oGfx = None
		
	def copy(self, newname):
		other = Component(self.GetType(), '')
		other.dAttrs = self.dAttrs.copy()
		other.dAttrs['name'] = newname
		other.lAttrs = self.lAttrs[:]
		other.dParams = self.dParams.copy()
		other.lParams = self.lParams[:]
		other.dInfo = self.dInfo.copy()
		return other
	
	def SetName(self, s):
		self.dAttrs['name'] = s
		
	def GetName(self):
		return self.dAttrs['name']
	
	def GetType(self):
		return self.dAttrs['type']
		
	def SetGfx(self, x, y, w, h):
		self.oGfx = x, y, w, h
	
	def AddAttribute(self, sName, sDefault=None):
		self.dAttrs[sName] = sDefault
		if not self.lAttrs.count(sName):
			self.lAttrs.append(sName)
		
	def SetAttribute(self, sName, sValue):
		if not sName in self.dAttrs:
			raise ValueError, '%s is an unknown attribute' % sType
		self.dAttrs[sName] = sValue
		
	def GetAttribute(self, sName ):
		if not sName in self.dAttrs:
			raise ValueError, '%s is an unknown attribute' % sType
		return self.dAttrs[sName]
		
	def HasParameter(self, sName, sType):
		if not (sName, sType) in self.dParams:
			return False
		return True
		
	def AddParamater(self, sName, sType, sDefault=None, sTip=''):
		self.dParams[sName, sType] = sDefault
		self.dInfo[sName, sType] = sTip
		if not self.lParams.count((sName, sType)):
			self.lParams.append((sName, sType))
			
	def RemoveParameter(self, sName, sType):
		sType = ''
		for key in self.dParams.keys():
			if key[0] == sName:
				sType = key[1]
				break
		if not sType:
			raise ValueError, '%s is an unknown parameter' % sName
		del self.dParams[sName, sType]
		self.lParams.remove((sName, sType))
		
	def SetParameter(self, sName, sValue):
		sType = ''
		for key in self.dParams.keys():
			if key[0] == sName:
				sType = key[1]
				break
		if not sType:
			raise ValueError, '%s is an unknown parameter' % sName
		self.dParams[sName, sType] = sValue
		
	def GetParameter(self, sName):
		sType = ''
		for key in self.dParams.keys():
			if key[0] == sName:
				sType = key[1]
				break
		if not sType:
			raise ValueError, '%s is an unknown parameter' % sName
		return self.dParams[sName, sType]
	
	def SetTip(self, sTip):
		self.sTip = sTip
		
	def GetTip(self):
		return self.sTip
		
	def GenerateXML(self):
		sHeader	    = '\t<component ' + '%s ' * len(self.lAttrs) + '>\n'
		sAttribute	= '%s="%s"'
		sBody		= '%s\n' * len(self.lParams)
		sParam		= '\t\t<param name="%s" type="%s" val="%s" />'
		sGfx		= '\t\t<gfx x="%s" y="%s" w="%s" h="%s" />\n' 
		sTrailer	= '\t</component>\n'
		
		s  = sHeader % tuple( [ sAttribute % (sName, self.dAttrs[sName])				 for sName		 in self.lAttrs  ] )
		s += sBody   % tuple( [ sParam	 % (sName, sType, self.dParams[sName,sType])	for sName,sType in self.lParams ] )
		if self.oGfx: s += sGfx % self.oGfx
		s += sTrailer		
		return s
	
if __name__ == '__main__':
	## Check version
	import sys
	major, minor, micro, level, serial = sys.version_info
	if major != 2 or minor < 4:
		print 'xstreamer requires python 2.4 or higher, please download the latest version from www.python.org'
		sys.exit(-1)
		
	## Default file
	sFile = None
	if( len(sys.argv) > 1 ):
		sFile = argv[1]
			
	## Main configuration
	Configuration(sFile).Draw()