from xml.dom.minidom import parse, parseString
import util
import os.path

TEMPLATE_CONFIG = 'python/newGUI/template.xml'
BASIC_CONFIG = '<?xml version="1.0" encoding="utf-8"?><config quantum="40000" seed="0" simulation="0" log=""></config>'
UNDO_BUFFER_SIZE = 100

class SirannonConfiguration:
    counter = 0
    templates = {}
    def __init__(self):
        self.__xml = None
        self.__config = None        
        self.__index = 0
        self.__save_index = 0
        self.__xmls = []
        self.__name = 'Unsaved {0}'.format(SirannonConfiguration.counter)        
        self.__filename = ''
        self.__dirty = False
        
        # Load the templates once
        SirannonConfiguration.counter += 1
        if not SirannonConfiguration.templates:
            SirannonConfiguration.loadTemplates()
            
    @staticmethod
    def loadTemplates():
        doc = parse(TEMPLATE_CONFIG)
        root = doc.getElementsByTagName('template')[0]
        SirannonConfiguration.templates = dict([(c.getAttribute('name'), c) for c in root.getElementsByTagName('component')])

    def getName(self):
        return self.__name
    
    def getFilename(self):
        return self.__filename
    
    def __updateName(self, filename):
        self.__filename = filename
        self.__name = os.path.split(self.__filename)[1]
    
    def loadFromFile(self, filename):
        self.__updateName(filename)
        self.__xml = parse(filename)
        str = self.toString()
        str = str.replace('\n','')
        str = str.replace('\t','')
        self.loadFromString(str)        
                
    def loadFromString(self, str):
        str = str.replace('\n','')
        str = str.replace('\t','')
        self.__xml = parseString(str)
        self.__reset()
        
    def loadFromDefault(self):
        self.loadFromString(BASIC_CONFIG)        
        
    def updateFromString(self, str):
        str = str.replace('\n','')
        str = str.replace('\t','')
        self.sync(parseString(str))
        self.__update()
        
    def updateFromDefault(self):
        self.updateFromString(BASIC_CONFIG)
        
    def saveToFile(self, filename):
        self.__updateName(filename)
        file = open(filename, 'w')
        str = self.toFormattedString()
        file.write(str)
        file.close()
        self.__dirty = False
        self.__save_index = self.__index
        
    def toString(self):
        return self.__xml.toxml('utf-8')
    
    def toFormattedString(self):
        return self.__xml.toprettyxml(newl='\n', encoding='utf-8')
    
    def isDirty(self):
        return self.__dirty or self.__index != self.__save_index
    
    def forceDirty(self):
        self.__dirty = True
            
    def __reset(self):
        for xml in self.__xmls:            
            xml.unlink()
        self.__xmls = [self.__xml]
        self.__index = 0
        self.__save_index = 0
        self.__dirty = False
        self.__update()
    
    def __update(self):
        self.__xml = self.__xmls[self.__index]
        self.__config = self.__xml.getElementsByTagName('config')[0]
        if not self.__config:
            raise ValueError('No node "config" in XML')
        
    def sync(self, copy=None):        
        # Throw away higher ones 
        for xml in self.__xmls[self.__index+1:]:
            xml.unlink()        
        self.__xmls = self.__xmls[:self.__index+1]
        
        # Too large
        if len(self.__xmls) > UNDO_BUFFER_SIZE:
            self.__xmls[0].unlink()
            self.__xmls.pop(0)
            self.__index -= 1
            self.__save_index -= 1
        
        # New current
        if not copy:
            copy = self.__xml.cloneNode(True) 
        self.__xmls.append(copy)
        print 'Sync: {0}->{1}'.format(self.__index, self.__index+1)
        self.__index += 1
        self.__update()
        
    def undoable(self):
        return self.__index > 0
    
    def redoable(self):
        return self.__index + 1 < len(self.__xmls)
    
    def purge(self): # Erase the last edit fully and undo
        if self.undoable():
            self.undo()
            self.__xmls[-1].unlink()
            self.__xmls.pop(-1)        
        
    def undo(self):
        if self.undoable():
            print 'Undo: {0}->{1}'.format(self.__index, self.__index-1)
            self.__index -= 1
            self.__update()
        return self.undoable(), self.redoable()
    
    def redo(self):
        if self.redoable():
            print 'Redo: {0}->{1}'.format(self.__index, self.__index+1)
            self.__index += 1
            self.__update()
        return self.undoable(), self.redoable()
        
    def getComponentDesc(self, type):
        if type in SirannonConfiguration.templates:
            return SirannonConfiguration.templates[type].getAttribute('desc')
        return None
    
    def getTemplateParamDesc(self, type):
        template = SirannonConfiguration.templates[type]
        return dict([(p.getAttribute('name'), p.getAttribute('desc')) for p in template.getElementsByTagName('param')])
        
    def getTemplates(self):
        return SirannonConfiguration.templates.values()
    
    def getTemplateNames(self):
        return SirannonConfiguration.templates.keys()
    
    def getTemplateParams(self, type):
        list = []
        if type in SirannonConfiguration.templates:
            template = SirannonConfiguration.templates[type]
            parent = template.getAttribute('type')                
            if parent in SirannonConfiguration.templates:
                list += self.getTemplateParams(parent)                
            list += template.getElementsByTagName('param')
        return list

    def getParams(self, component):
        return component.getElementsByTagName('param')
    
    def getParam(self, component, name):
        for param in component.getElementsByTagName('param'):
            if param.getAttribute('name') == name:
                return param
        return None
    
    def createElement(self, name):
        return self.__xml.createElement(name)
    
    def createAttributeNode(self, name):
        return self.__xml.createAttribute(name)
        
    def getChild(self, item, type):
        elems = item.getElementsByTagName(type)
        if not elems:
            elem = self.__xml.createElement(type)
            item.appendChild(elem)
            return elem
        else:
            return elems[0]    
    
    def getComponents(self):
        return self.getElements('component')
    
    def getRoutesFrom(self, name):
        routes = []
        for route in self.__config.getElementsByTagName('route'):
            if route.getAttribute('from') == name:
                routes.append(route)
        return routes
                
    def getElements(self, type):
        return self.__config.getElementsByTagName(type)
    
    def delElement(self, item):       
        self.__config.removeChild(item)
        item.unlink()    
    
    def getComponent(self, name):
        for item in self.__config.getElementsByTagName('component'):
            if item.getAttribute('name') == name:
                return item
        return None
    
    def getRoute(self, _from, to):
        for item in self.__config.getElementsByTagName('route'):
            if item.getAttribute('from') == _from and item.getAttribute('to') == to:
                return item
        return None
    
    def createRoute(self, _from, to, xroute):
        route = self.__xml.createElement('route')
        self.__config.appendChild(route)
        route.setAttribute('from', _from)
        route.setAttribute('to', to)
        route.setAttribute('xroute', str(xroute))
        return route
    
    def createCommand(self, bin, config, flags):
        command = self.__xml.createElement('command')
        self.__config.appendChild(command)
        command.setAttribute('binary', bin)
        command.setAttribute('configuration', config)
        command.setAttribute('flags', flags)
        return command
    
    def getCommand(self, create=False):
        list = self.__config.getElementsByTagName('command')
        if list:
            return list[0]
        elif create:
            command = self.__xml.createElement('command')
            self.__config.appendChild(command)
            return command
        return None
    
    def proofRead(self, correct=False):
        for component in self.getComponents():
            # Make sure the type is in the correct capitalized version
            type = component.getAttribute('type')
            for name in self.getTemplateNames():
                if name.lower() == type:
                    type = name
            component.setAttribute('type', type)
           
            # Get template
            template_params = util.dictify(self.getTemplateParams(type))
            if not template_params:
                continue 
            params = dict([(p.getAttribute('name'), p) for p in component.getElementsByTagName('param')])
           
            # Add lost or new parameters
            for template_param in self.getTemplateParams(type):                
                if template_param.getAttribute('name') not in params:
                    new_param = self.__xml.createElement('param')
                    new_param.setAttribute('name', template_param.getAttribute('name'))
                    new_param.setAttribute('type', template_param.getAttribute('type'))
                    new_param.setAttribute('val', template_param.getAttribute('default'))
                    component.appendChild(new_param)        
            
            # Remove old parameters
            for param in component.getElementsByTagName('param'):
                name = param.getAttribute('name')
                if name not in template_params:
                    if correct:
                        print('Parameter({0}) not in template for component({1})'.format(name, component.getAttribute('name')))
                        component.removeChild(param)
                        param.unlink()
                    else:
                        return -1                   
        return 0            
    
    def createComponent(self, name, type):
        component = self.__xml.createElement('component')
        self.__config.appendChild(component)
        component.setAttribute('name', name)
        component.setAttribute('type', type)
        
        for template_param in self.getTemplateParams(type):
            param = self.__xml.createElement('param')
            component.appendChild(param)
            param.setAttribute('val', template_param.getAttribute('default'))
            param.setAttribute('name', template_param.getAttribute('name'))
            param.setAttribute('type', template_param.getAttribute('type'))
        return component
            
    def uniqueName(self, name):
        name = name.replace(' ', '-')
        for component in self.getElements('component'):
            if component.getAttribute('name') == name:
                if name[-1].isdigit():
                    suffix = str(int(name[-1]) + 1)
                    newname = name[:-1] + suffix
                else:
                    newname = name + '-1'
                return self.uniqueName(newname)                
        return name
    
    def setParam(self, component, param_name, val):
        params = self.getParams(component)
        for param in params:
            if param.getAttribute('name') == param_name:
                param.setAttribute('val', val)
        
