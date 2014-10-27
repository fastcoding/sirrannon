#!/usr/bin/env python
## 
# This program crawls over the src directory and looks for "@component" and generates
# doc from this 
## 
import re
from distutils import extension

SRC_DIR = 'src'
FILE_EXTENSION = [ '.cpp' ]

OUTPUT_LATEX = 'doc/manual/doc-gen.tex'
OUTPUT_PY = 'python/oldGUI/components_gen.py'
OUTPUT_XML = 'python/newGUI/template.xml'

class param:
    def __init__(self, name, type, default, desc):
        self.type = type
        self.default = default
        self.name = name
        self.desc = desc

class component:
    def __init__(self):
        self.name = ''
        self.type = ''
        self.desc = ''
        self.params = []
        self.abstract = False
        self.private = False
        self.beta = False
        self.availability = []
        
    def addParameter(self, name, type, default, desc):
        self.params.append( param( name, type, default, desc ) )        
        
    def __str__(self):
        return 'name({0}) parent({1}) params({2}) info({3})'.format(
            self.name, self.type, len(self.params), self.desc)
    
    def __lt__(self, other):
        return self.name < other.name

class doc:
    
    def __init__(self):
        self.oComponent = component()
        self.dComponents = {}
        self.dTree = {}
        self.oExprLine = re.compile(r'@(\w+) +(.*)')
        self.oExprParam = re.compile(r'(\S+?), +(\S+?), +(\S*?), +(.+)')
        
    def newComponent(self):
        print('New component')
        self.oComponent = component()
        
    def finishComponent(self):
        self.dComponents[self.oComponent.name] = self.oComponent
        
    def postProcess(self):
        # Construct the tree
        print('Abstract:')
        for pComponent in self.dComponents.values():
            if pComponent.abstract:
                print(pComponent)
                self.dTree[pComponent.name] = []
                
        print('Normal:')
        for pComponent in self.dComponents.values():
            if not pComponent.abstract:
               print(pComponent)
               self.dTree[pComponent.type].append(pComponent)
               
    def outputXmlFile(self, filename=OUTPUT_XML):
        import xml.dom
        xml = xml.dom.getDOMImplementation().createDocument(None, None, None)        
        template = xml.createElement('template')
        xml.appendChild(template)
        
        for component in self.dComponents.values():
            xml_component = xml.createElement('component')
            template.appendChild(xml_component)
            xml_component.setAttribute('name', component.name)
            xml_component.setAttribute('type', component.type)
            xml_component.setAttribute('desc', component.desc)
            xml_component.setAttribute('abstract', str(component.abstract))
            
            for param in component.params:
                xml_param = xml.createElement('param')
                xml_component.appendChild(xml_param)
                xml_param.setAttribute('name', param.name)
                xml_param.setAttribute('type', param.type)
                xml_param.setAttribute('default', param.default)
                xml_param.setAttribute('desc', param.desc)
                        
        file = open(filename, 'w')
        xml.writexml(file)
               
    def outputPyFile(self, sFile=OUTPUT_PY):
        oFile = open(sFile, 'w')
        oFile.write( self.outputPy())
        
    def outputPy(self):
        sFormat = ( "from sirannon import *\n"
                    "g_dComponents     = {}\n"
                    "g_dBaseComponents = {}\n" )
                
        for sAbstract in self.dTree.keys():
            sFormat += self.outputComponentPy(self.dComponents[sAbstract], True)
            lComponents = self.dTree[sAbstract]
            lComponents.sort()
            for pComponent in lComponents:
                 sFormat += self.outputComponentPy(pComponent, False)
                 
        lAbstracts = list(self.dTree.keys())
        lAbstracts.sort()
        
        sFormat += (  "g_lComponents = g_dComponents.keys()\n"
                      "g_lComponents.sort()\n"        
                      "g_lBaseComponents = {0}\n" ).format(lAbstracts)
        return sFormat
               
    def outputComponentPy(self, pComponent, bAbstract):    
        pyname = pComponent.name.replace('-','_')
        if pyname == 'in':
            pyname = '_in'
        pytype = pComponent.type.replace('-','_')
        if pyname == 'core':
            pytype = 'Component'
        if bAbstract:
             sFormat = ( "class {0}({1}):\n"
                         "\tdef __init__(self,sType,sName):\n"
                         "\t\t{1}.__init__(self,sType,sName)\n" ).format(pyname, pytype)
        else:
            sFormat = ( "class {0}({1}):\n"
                        "\tdef __init__(self,sName):\n"
                        "\t\t{1}.__init__(self,\"{2}\",sName)\n" ).format(pyname, pytype, pComponent.name)
        
        sFormat += '\t\tself.SetTip("{0}")\n'.format(pComponent.desc)
                      
        for pParam in pComponent.params:
            sFormat += '\t\tself.AddParamater("{0.name}", "{0.type}", "{0.default}", "{0.desc}")\n'.format(pParam)
              
        if bAbstract: 
            sFormat += 'g_dBaseComponents["{0.name}"] = {1}\n\n'.format(pComponent, pyname)
        else:
            sFormat += 'g_dComponents["{0.name}"] = {1}\n\n'.format(pComponent, pyname)
        return sFormat
                
    def outputLatexFile(self, sFile=OUTPUT_LATEX):
        oFile = open(sFile, 'w')
        oFile.write( self.outputLatex())
        
    def outputLatex(self):
        sFormat = ''
        ## Collect all abstract classes
        oKeys = list(self.dTree.keys())
        oKeys.sort()
        for sAbstract in oKeys:
            if self.dComponents[sAbstract].private:
                continue
            sFormat += self.outputComponentLatex(self.dComponents[sAbstract], 'section')
            self.dTree[sAbstract].sort()
            for pComponent in self.dTree[sAbstract]:
                if pComponent.private:
                    continue
                sFormat += self.outputComponentLatex(pComponent, 'subsection')
            
            sFormat += '\\newpage\n'
        
        ## Fix forbidding Latex chars
        sFormat = sFormat.replace('&', r'\&')
        
        return sFormat
                   
    def outputComponentLatex(self, pComponent, sLevel='subsection'):
        sFormat = ( "\\{1}{{{0.name}}}\n"
                    "{0.desc}\n" ).format(pComponent, sLevel)
                    
        if pComponent.beta:
            sFormat += r"\newline WARNING: this component is in a beta version and might not function properly."
            
        if pComponent.availability:
            sFormat += reduce( lambda x, y: x+" "+y, pComponent.availability, r"\newline Availability:" )                
        
        if len(pComponent.params):
            sFormat += ( "\\begin{itemize}\n"
                         "\\item Parameters:\n"
                         "\\begin{itemize}\n" )
                         
            for pParam in pComponent.params:
                sDefault = pParam.default
                if not sDefault:
                    sDefault = '""'
                sFormat += "\item {0.type} {0.name}: {0.desc}, default: {1}\n".format(pParam, sDefault)
                   
            sFormat += ( "\\end{itemize}\n"
                         "\\end{itemize}\n" )        
        return sFormat
        
    def outputComponents(self):
        print('Components ({0}):'.format(len(self.dComponents.keys())))
        for name in self.dComponents.keys():
            print( self.outputComponentLatex( self.dComponents[name] ) )         
     
    def parse(self, sDir=None):
        if not sDir:
            sDir = SRC_DIR
        print('Parsing dir({0})'.format(sDir))
        
        import os        
        for sItem in os.listdir(sDir):
            if sItem[0] == '.':
                continue
            sFull = os.path.join(sDir, sItem)
            
            if os.path.isdir(sFull):
                self.parse(sFull)
            elif os.path.isfile(sFull):
                if( os.path.splitext(sItem)[1] in FILE_EXTENSION ):
                    self.parseFile(sFull)
    
    def parseFile(self, sFile):
        print('Parsing file({0})'.format(sFile))
        oFile = open(sFile, "r")
        bComment = False
        bComponent = False
        bInfo = False
        for sLine in oFile:           
            sLine = sLine.strip()
            if len(sLine) < 2:
                continue
            
            ## Detect comment start
            bStart = sLine.find("/*") != -1
            bEnd = sLine.find("*/") != -1
            
            ##Ignore one line comments
            if bStart and bEnd:
                continue
            
            ## Single start
            if bStart:
                if bComment:
                    raise SyntaxError('corrupted start :' + sLine )
                bComment = True
                print( 'Comment start: ' + sLine)
                
            ## Detect comment end           
            if bEnd:
                if not bComment:
                    raise SyntaxError('corrupted end: ' + sLine)
                bComment = False
                print( 'Comment end: ' + sLine)
                if bComponent:
                    self.finishComponent()
                    bComponent = False
           
            ## Toggle when seeing @component
            sLine = sLine.strip('/* \t\n\r')
            if not bComponent and sLine.find('@component') == 0:
               bComponent = True
               self.newComponent()
            if not bComponent:
                continue
            
            ## Forbidden characheters
            sLine = sLine.replace('"',"'")
            sLine = sLine.replace('_','-')
            
            ## Parse the command                
            if sLine[0] == '@':
                ## Component info            
                oMatch = self.oExprLine.match(sLine)
                if not oMatch:
                    print('@ match failed')
                    continue            
                sCommand = oMatch.group(1)
                sData = oMatch.group(2)
                print('Match {0} {1}'.format(sCommand, sData))
                
                # Handle command
                bInfo = False        
                if sCommand == 'component':                    
                    self.oComponent.name = sData
                    
                elif sCommand == 'type':
                    self.oComponent.type = sData
                    
                elif sCommand == 'properties':
                    if sLine.find('abstract') != -1:
                        self.oComponent.abstract = True
                    if sLine.find('private') != -1:
                        self.oComponent.private = True
                    if sLine.find('beta') != -1:
                        self.oComponent.beta = True
                    if sLine.find('unix') != -1:
                        self.oComponent.availability.append( "Unix" )
                    if sLine.find('windows') != -1:
                        self.oComponent.availability.append( "Windows" )                   
                    
                elif sCommand == 'param':
                    oMatch = self.oExprParam.match(sData)
                    if not oMatch:
                        print('param match failed')
                        continue
                    self.oComponent.addParameter(*oMatch.groups())
                    
                elif sCommand == 'info':
                    bInfo = True
                    self.oComponent.desc = sData                
                
            elif bInfo:                
                self.oComponent.desc = self.oComponent.desc + ' ' + sLine

if __name__ == "__main__":
    oDoc = doc()
    oDoc.parse()
    oDoc.postProcess()
    oDoc.outputLatexFile()
    oDoc.outputPyFile()
    oDoc.outputXmlFile()
