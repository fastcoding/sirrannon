import math
import cairo
import canvas
from math import fabs

P = canvas.ARROW_LENGTH * 1.5

class Vector:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        
    def __eq__(self, p):
        return self.x == p.x and self.y == p.y
    
    def __neg__(self):
        return Vector(-self.x, -self.y)
    
    def unitize(self):
        s = math.sqrt(self.norm())
        if s:
            return Vector(self.x / s, self.y / s)
        return Vector(0., 0.)
    
    def norm(self):
        return self.x * self.x + self.y * self.y
        
    def __mul__(self, f):
        if isinstance(f, Vector):
            return Vector(self.x * f.x, self.y * f.y)
        else:
            return Vector(self.x * f, self.y * f)
        
    def __add__(self, p):
       return Vector(self.x + p.x, self.y + p.y)
        
    def __sub__(self, p):
        return Vector(self.x - p.x, self.y - p.y)
    
    def abs(self):
        return Vector(math.fabs(self.x), math.fabs(self.y))
    
    def __repr__(self):
        return '({0},{1})'.format(self.x, self.y)
    
def rot(a, r):
    return (a + r) % 4

def flip(a):
    return (a + 2) % 4
    
SOUTH, WEST, NORTH, EAST  = 0, 1, 2, 3
dir = { SOUTH: Vector(0,1), WEST: Vector(-1,0), NORTH: Vector(0,-1), EAST: Vector(1,0), }

def sign(x):
    if x < 0:
        return -1
    elif x > 0:
        return 1
    else:
        return 0

def dot(p1, p2):
    return p1.x * p2.x + p1.y * p2.y

def getPoints(x1, y1, d1, w1, h1, x2, y2, d2, w2, h2):    
    # Different orientations
    r0, r1, r2, r3 = [dir[(d1 + x) % 4] for x in range(4)]
            
    # Convert into a very natural axis system 
    # Y-axis pointing away from the controller
    # X-axis parellel with the component edge
    # Left handiness
    matrix = cairo.Matrix.init_rotate(-d1*math.pi/2)
    x, y = matrix.transform_point(x2-x1, y2-y1)
    w1, h1 = matrix.transform_distance(w1, h1)
    w1, h1 = math.fabs(w1/2), math.fabs(h1/2)
    w2, h2 = matrix.transform_distance(w2, h2)
    w2, h2 = math.fabs(w2/2), math.fabs(h2/2)
    rx = r3 * sign(x)
    ry = r0 * sign(y)  
    
    if dir[d2] == r0:        
        if y > 0:
            if fabs(x) < w1 + P:
                return (0, P, r0), (-1, w2 + P, -rx), (1, 0, ry), (0, w2 + P, rx)
            else:
                return (1, P, r0), (1, 0, rx)
        else:
            if fabs(x) < w1 + P:             
                return (0, P, r0), (0, w1 + P, rx), (1, 0, ry), (-1, w1 + P, -rx)
            else:            
                return (0, P, r0), (1, 0, rx)
            
    elif dir[d2] == r2:
        if y > 0:
            return (0.5, 0, r0), (1, 0, rx)
        elif fabs(x) - w2 - P < w1 + P:
            return (0, max(0, y + 2*h2) + P, r0), (1, max(w1, w2) + P, rx), (1, P + max(0, y + 2*h2) + P, ry), (0, max(w1, w2) + P, -rx)
        else:
            return (0, P, r0), (0, (w1 + fabs(x) - w2) / 2, rx), (1, P + P, ry), (1, -(w1 + fabs(x) - w2) / 2, rx)
        
    else:
        if dir[d2] == r3:
            x = -x
            r1, r3 = r3, r1
        if y > 0 and x > P:
            return (1, 0, r0),            
        elif y - h2 > P:
            if x > P:
                return (1, 0, r0),
            else:
                return (0, P, r0), (sign(-x), P, r1), (1, -P, r0)            
        else:
            if x - P > w1 + P or (2*h1 > -y and x > w1):
                return (0, P, r0), (0, (w1 + fabs(x)) / 2, rx), (sign(-y), P, r2)
            else:
                return (0, max(0, y + h2) + P, r0), (0, max(-x, w1) + P, r1), (sign(-y), max(0, y + h2) + P, r2)

    return ()