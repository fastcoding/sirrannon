/* -- THIS FILE IS GENERATED - DO NOT EDIT *//* -*- Mode: C; c-basic-offset: 4 -*- */

#include <Python.h>



#line 3 "crcanvas.override"
/* vim: syntax=c shiftwidth=4 softtabstop=4
 */
#include <Python.h>
#define NO_IMPORT_PYGOBJECT
#include <pycairo.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>

#include "libcrcanvas.h"

/* Pre Python 2.5 backwards compatibility for PEP 353 */
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

extern Pycairo_CAPI_t *Pycairo_CAPI;
extern PyTypeObject PyGdkCairoContext_Type;

#line 29 "crcanvas.c"


/* ---------- types from other modules ---------- */
static PyTypeObject *_PyGObject_Type;
#define PyGObject_Type (*_PyGObject_Type)
static PyTypeObject *_PyGtkAdjustment_Type;
#define PyGtkAdjustment_Type (*_PyGtkAdjustment_Type)
static PyTypeObject *_PyGtkWidget_Type;
#define PyGtkWidget_Type (*_PyGtkWidget_Type)


/* ---------- forward type declarations ---------- */
PyTypeObject G_GNUC_INTERNAL PyCrBounds_Type;
PyTypeObject G_GNUC_INTERNAL PyCrDeviceBounds_Type;
PyTypeObject G_GNUC_INTERNAL PyCrDash_Type;
PyTypeObject G_GNUC_INTERNAL PyCrCanvas_Type;
PyTypeObject G_GNUC_INTERNAL PyCrItem_Type;
PyTypeObject G_GNUC_INTERNAL PyCrInverse_Type;
PyTypeObject G_GNUC_INTERNAL PyCrPanner_Type;
PyTypeObject G_GNUC_INTERNAL PyCrPath_Type;
PyTypeObject G_GNUC_INTERNAL PyCrLine_Type;
PyTypeObject G_GNUC_INTERNAL PyCrEllipse_Type;
PyTypeObject G_GNUC_INTERNAL PyCrArrow_Type;
PyTypeObject G_GNUC_INTERNAL PyCrPixbuf_Type;
PyTypeObject G_GNUC_INTERNAL PyCrRectangle_Type;
PyTypeObject G_GNUC_INTERNAL PyCrRotator_Type;
PyTypeObject G_GNUC_INTERNAL PyCrText_Type;
PyTypeObject G_GNUC_INTERNAL PyCrVector_Type;
PyTypeObject G_GNUC_INTERNAL PyCrZoomer_Type;
PyTypeObject G_GNUC_INTERNAL PyCrBlit_Type;

#line 61 "crcanvas.c"



/* ----------- CrBounds ----------- */

static int
_wrap_cr_bounds_new(PyGBoxed *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,":None.Bounds.__init__", kwlist))
        return -1;
    self->gtype = CR_TYPE_BOUNDS;
    self->free_on_dealloc = FALSE;
    self->boxed = cr_bounds_new();

    if (!self->boxed) {
        PyErr_SetString(PyExc_RuntimeError, "could not create CrBounds object");
        return -1;
    }
    self->free_on_dealloc = TRUE;
    return 0;
}

static PyObject *
_wrap_cr_bounds_unref(PyObject *self)
{
    
    cr_bounds_unref(pyg_boxed_get(self, CrBounds));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyCrBounds_methods[] = {
    { "unref", (PyCFunction)_wrap_cr_bounds_unref, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

#line 451 "crcanvas.override"
static int
_wrap_cr_bounds__set_x1(PyGBoxed *self, PyObject *value, void *closure)
{
    gdouble val;

    val = PyFloat_AsDouble(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrBounds)->x1 = val;
    return 0;
}
#line 114 "crcanvas.c"


static PyObject *
_wrap_cr_bounds__get_x1(PyObject *self, void *closure)
{
    double ret;

    ret = pyg_boxed_get(self, CrBounds)->x1;
    return PyFloat_FromDouble(ret);
}

#line 464 "crcanvas.override"
static int
_wrap_cr_bounds__set_y1(PyGBoxed *self, PyObject *value, void *closure)
{
    gdouble val;

    val = PyFloat_AsDouble(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrBounds)->y1 = val;
    return 0;
}
#line 138 "crcanvas.c"


static PyObject *
_wrap_cr_bounds__get_y1(PyObject *self, void *closure)
{
    double ret;

    ret = pyg_boxed_get(self, CrBounds)->y1;
    return PyFloat_FromDouble(ret);
}

#line 477 "crcanvas.override"
static int
_wrap_cr_bounds__set_x2(PyGBoxed *self, PyObject *value, void *closure)
{
    gdouble val;

    val = PyFloat_AsDouble(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrBounds)->x2 = val;
    return 0;
}
#line 162 "crcanvas.c"


static PyObject *
_wrap_cr_bounds__get_x2(PyObject *self, void *closure)
{
    double ret;

    ret = pyg_boxed_get(self, CrBounds)->x2;
    return PyFloat_FromDouble(ret);
}

#line 490 "crcanvas.override"
static int
_wrap_cr_bounds__set_y2(PyGBoxed *self, PyObject *value, void *closure)
{
    gdouble val;

    val = PyFloat_AsDouble(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrBounds)->y2 = val;
    return 0;
}

#line 187 "crcanvas.c"


static PyObject *
_wrap_cr_bounds__get_y2(PyObject *self, void *closure)
{
    double ret;

    ret = pyg_boxed_get(self, CrBounds)->y2;
    return PyFloat_FromDouble(ret);
}

static const PyGetSetDef cr_bounds_getsets[] = {
    { "x1", (getter)_wrap_cr_bounds__get_x1, (setter)_wrap_cr_bounds__set_x1 },
    { "y1", (getter)_wrap_cr_bounds__get_y1, (setter)_wrap_cr_bounds__set_y1 },
    { "x2", (getter)_wrap_cr_bounds__get_x2, (setter)_wrap_cr_bounds__set_x2 },
    { "y2", (getter)_wrap_cr_bounds__get_y2, (setter)_wrap_cr_bounds__set_y2 },
    { NULL, (getter)0, (setter)0 },
};

PyTypeObject G_GNUC_INTERNAL PyCrBounds_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Bounds",                   /* tp_name */
    sizeof(PyGBoxed),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    0,             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyCrBounds_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)cr_bounds_getsets,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    0,                 /* tp_dictoffset */
    (initproc)_wrap_cr_bounds_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrDeviceBounds ----------- */

static int
_wrap_cr_device_bounds_new(PyGBoxed *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,":None.DeviceBounds.__init__", kwlist))
        return -1;
    self->gtype = CR_TYPE_DEVICE_BOUNDS;
    self->free_on_dealloc = FALSE;
    self->boxed = cr_device_bounds_new();

    if (!self->boxed) {
        PyErr_SetString(PyExc_RuntimeError, "could not create CrDeviceBounds object");
        return -1;
    }
    self->free_on_dealloc = TRUE;
    return 0;
}

static PyObject *
_wrap_cr_device_bounds_unref(PyObject *self)
{
    
    cr_device_bounds_unref(pyg_boxed_get(self, CrDeviceBounds));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyCrDeviceBounds_methods[] = {
    { "unref", (PyCFunction)_wrap_cr_device_bounds_unref, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

#line 385 "crcanvas.override"
static int
_wrap_cr_device_bounds__set_x1(PyGBoxed *self, PyObject *value, void *closure)
{
    gdouble val;

    val = PyFloat_AsDouble(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrDeviceBounds)->x1 = val;
    return 0;
}
#line 303 "crcanvas.c"


static PyObject *
_wrap_cr_device_bounds__get_x1(PyObject *self, void *closure)
{
    double ret;

    ret = pyg_boxed_get(self, CrDeviceBounds)->x1;
    return PyFloat_FromDouble(ret);
}

#line 398 "crcanvas.override"
static int
_wrap_cr_device_bounds__set_y1(PyGBoxed *self, PyObject *value, void *closure)
{
    gdouble val;

    val = PyFloat_AsDouble(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrDeviceBounds)->y1 = val;
    return 0;
}
#line 327 "crcanvas.c"


static PyObject *
_wrap_cr_device_bounds__get_y1(PyObject *self, void *closure)
{
    double ret;

    ret = pyg_boxed_get(self, CrDeviceBounds)->y1;
    return PyFloat_FromDouble(ret);
}

#line 411 "crcanvas.override"
static int
_wrap_cr_device_bounds__set_x2(PyGBoxed *self, PyObject *value, void *closure)
{
    gdouble val;

    val = PyFloat_AsDouble(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrDeviceBounds)->x2 = val;
    return 0;
}
#line 351 "crcanvas.c"


static PyObject *
_wrap_cr_device_bounds__get_x2(PyObject *self, void *closure)
{
    double ret;

    ret = pyg_boxed_get(self, CrDeviceBounds)->x2;
    return PyFloat_FromDouble(ret);
}

#line 424 "crcanvas.override"
static int
_wrap_cr_device_bounds__set_y2(PyGBoxed *self, PyObject *value, void *closure)
{
    gdouble val;

    val = PyFloat_AsDouble(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrDeviceBounds)->y2 = val;
    return 0;
}
#line 375 "crcanvas.c"


static PyObject *
_wrap_cr_device_bounds__get_y2(PyObject *self, void *closure)
{
    double ret;

    ret = pyg_boxed_get(self, CrDeviceBounds)->y2;
    return PyFloat_FromDouble(ret);
}

#line 437 "crcanvas.override"
static int
_wrap_cr_device_bounds__set_anchor(PyGBoxed *self, PyObject *value, 
        void *closure)
{
    GtkAnchorType val;

    val = (GtkAnchorType) PyInt_AsLong(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrDeviceBounds)->anchor = val;
    return 0;
}
#line 400 "crcanvas.c"


static PyObject *
_wrap_cr_device_bounds__get_anchor(PyObject *self, void *closure)
{
    gint ret;

    ret = pyg_boxed_get(self, CrDeviceBounds)->anchor;
    return pyg_enum_from_gtype(GTK_TYPE_ANCHOR_TYPE, ret);
}

static const PyGetSetDef cr_device_bounds_getsets[] = {
    { "x1", (getter)_wrap_cr_device_bounds__get_x1, (setter)_wrap_cr_device_bounds__set_x1 },
    { "y1", (getter)_wrap_cr_device_bounds__get_y1, (setter)_wrap_cr_device_bounds__set_y1 },
    { "x2", (getter)_wrap_cr_device_bounds__get_x2, (setter)_wrap_cr_device_bounds__set_x2 },
    { "y2", (getter)_wrap_cr_device_bounds__get_y2, (setter)_wrap_cr_device_bounds__set_y2 },
    { "anchor", (getter)_wrap_cr_device_bounds__get_anchor, (setter)_wrap_cr_device_bounds__set_anchor },
    { NULL, (getter)0, (setter)0 },
};

PyTypeObject G_GNUC_INTERNAL PyCrDeviceBounds_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.DeviceBounds",                   /* tp_name */
    sizeof(PyGBoxed),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    0,             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyCrDeviceBounds_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)cr_device_bounds_getsets,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    0,                 /* tp_dictoffset */
    (initproc)_wrap_cr_device_bounds_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrDash ----------- */

#line 314 "crcanvas.override"
static int
_wrap_cr_dash_new(PyGBoxed *self, PyObject *args, PyObject *kwargs)
{
    CrDash *dash = cr_dash_new();

    self->boxed =  g_boxed_copy(CR_TYPE_DASH, dash);
    self->free_on_dealloc = TRUE;
    self->gtype = CR_TYPE_DASH;

    cr_dash_unref(dash);
    return 0;
}
#line 483 "crcanvas.c"


#line 328 "crcanvas.override"
static PyObject *
_wrap_cr_dash__get_dashes(PyGObject *self, void *closure)
{
    CrDash *dash = (CrDash*)(self->obj);
    PyObject         *list, *item;
    int i;

    list = PyList_New(0);
    for (i = 0; i < dash->array->len; i++) {
        item = PyFloat_FromDouble(g_array_index(dash->array, double, i));
        PyList_Append(list, item);
        Py_DECREF(item);
    }
    return list;
}
static int
_wrap_cr_dash__set_dashes(PyGBoxed *self, PyObject *object, void *closure)
{
    CrDash *dash = pyg_boxed_get(self, CrDash);
    gint i, len;
    double v;

    if (!PySequence_Check(object))
	return -1;
    len = PySequence_Length(object);

    g_array_set_size(dash->array, 0);
    for (i = 0; i < len; i++) {
	PyObject *item = PySequence_GetItem(object, i);

        v = PyFloat_AsDouble(item);
	g_array_append_val(dash->array, v);
	if (PyErr_Occurred()) {
	    PyErr_Clear();
	    Py_DECREF(item);
	    return -1;
	}
	Py_DECREF(item);
    }
    
    return 0;
}
#line 529 "crcanvas.c"


#line 372 "crcanvas.override"
static int
_wrap_cr_dash__set_offset(PyGBoxed *self, PyObject *value, void *closure)
{
    gdouble val;

    val = PyFloat_AsDouble(value);
    if (PyErr_Occurred())
        return -1;
    pyg_boxed_get(self, CrDash)->offset = val;
    return 0;
}
#line 544 "crcanvas.c"


static PyObject *
_wrap_cr_dash__get_offset(PyObject *self, void *closure)
{
    double ret;

    ret = pyg_boxed_get(self, CrDash)->offset;
    return PyFloat_FromDouble(ret);
}

static const PyGetSetDef cr_dash_getsets[] = {
    { "dashes", (getter)_wrap_cr_dash__get_dashes, (setter)_wrap_cr_dash__set_dashes },
    { "offset", (getter)_wrap_cr_dash__get_offset, (setter)_wrap_cr_dash__set_offset },
    { NULL, (getter)0, (setter)0 },
};

PyTypeObject G_GNUC_INTERNAL PyCrDash_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Dash",                   /* tp_name */
    sizeof(PyGBoxed),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    0,             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)NULL, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)cr_dash_getsets,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    0,                 /* tp_dictoffset */
    (initproc)_wrap_cr_dash_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrCanvas ----------- */

static int
_wrap_cr_canvas_new(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GType obj_type = pyg_type_from_object((PyObject *) self);
    GParameter params[27];
    PyObject *parsed_args[27] = {NULL, };
    char *arg_names[] = {"root", "repaint_mode", "maintain_center", "maintain_aspect", "auto_scale", "show_less", "hadjustment", "vadjustment", "user_data", "name", "parent", "width_request", "height_request", "visible", "sensitive", "app_paintable", "can_focus", "has_focus", "is_focus", "can_default", "has_default", "receives_default", "composite_child", "style", "events", "extension_events", "no_show_all", NULL };
    char *prop_names[] = {"root", "repaint_mode", "maintain_center", "maintain_aspect", "auto_scale", "show_less", "hadjustment", "vadjustment", "user_data", "name", "parent", "width_request", "height_request", "visible", "sensitive", "app_paintable", "can_focus", "has_focus", "is_focus", "can_default", "has_default", "receives_default", "composite_child", "style", "events", "extension_events", "no_show_all", NULL };
    guint nparams, i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOOOOOOOOOOOOOOOOOOOOOOOOOO:crcanvas.Canvas.__init__" , arg_names , &parsed_args[0] , &parsed_args[1] , &parsed_args[2] , &parsed_args[3] , &parsed_args[4] , &parsed_args[5] , &parsed_args[6] , &parsed_args[7] , &parsed_args[8] , &parsed_args[9] , &parsed_args[10] , &parsed_args[11] , &parsed_args[12] , &parsed_args[13] , &parsed_args[14] , &parsed_args[15] , &parsed_args[16] , &parsed_args[17] , &parsed_args[18] , &parsed_args[19] , &parsed_args[20] , &parsed_args[21] , &parsed_args[22] , &parsed_args[23] , &parsed_args[24] , &parsed_args[25] , &parsed_args[26]))
        return -1;

    memset(params, 0, sizeof(GParameter)*27);
    if (!pyg_parse_constructor_args(obj_type, arg_names,
                                    prop_names, params, 
                                    &nparams, parsed_args))
        return -1;
    pygobject_constructv(self, nparams, params);
    for (i = 0; i < nparams; ++i)
        g_value_unset(&params[i].value);
    if (!self->obj) {
        PyErr_SetString(
            PyExc_RuntimeError, 
            "could not create crcanvas.Canvas object");
        return -1;
    }
    return 0;
}

static PyObject *
_wrap_cr_canvas_set_vadjustment(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "adjustment", NULL };
    PyGObject *adjustment;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!:Cr.Canvas.set_vadjustment", kwlist, &PyGtkAdjustment_Type, &adjustment))
        return NULL;
    
    cr_canvas_set_vadjustment(CR_CANVAS(self->obj), GTK_ADJUSTMENT(adjustment->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_canvas_set_hadjustment(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "adjustment", NULL };
    PyGObject *adjustment;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!:Cr.Canvas.set_hadjustment", kwlist, &PyGtkAdjustment_Type, &adjustment))
        return NULL;
    
    cr_canvas_set_hadjustment(CR_CANVAS(self->obj), GTK_ADJUSTMENT(adjustment->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_canvas_set_scroll_region(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "scroll_x1", "scroll_y1", "scroll_x2", "scroll_y2", NULL };
    double scroll_x1, scroll_y1, scroll_x2, scroll_y2;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dddd:Cr.Canvas.set_scroll_region", kwlist, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2))
        return NULL;
    
    cr_canvas_set_scroll_region(CR_CANVAS(self->obj), scroll_x1, scroll_y1, scroll_x2, scroll_y2);
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 135 "crcanvas.override"
static PyObject *
_wrap_cr_canvas_get_scroll_region(PyGObject *self, PyObject *args)
{
    double x1, y1, x2, y2;
    
    cr_canvas_get_scroll_region(CR_CANVAS(self->obj), &x1, &y1, &x2, &y2);
    
    return Py_BuildValue("(dddd)", x1, y1, x2, y2);
}
#line 696 "crcanvas.c"


static PyObject *
_wrap_cr_canvas_set_scroll_factor(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "scroll_factor_x", "scroll_factor_y", NULL };
    double scroll_factor_x, scroll_factor_y;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dd:Cr.Canvas.set_scroll_factor", kwlist, &scroll_factor_x, &scroll_factor_y))
        return NULL;
    
    cr_canvas_set_scroll_factor(CR_CANVAS(self->obj), scroll_factor_x, scroll_factor_y);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_canvas_scroll_to(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "x", "y", NULL };
    double x, y;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dd:Cr.Canvas.scroll_to", kwlist, &x, &y))
        return NULL;
    
    cr_canvas_scroll_to(CR_CANVAS(self->obj), x, y);
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 146 "crcanvas.override"
static PyObject *
_wrap_cr_canvas_get_scroll_offsets(PyGObject *self, PyObject *args)
{
    double x, y;
    
    cr_canvas_get_scroll_offsets(CR_CANVAS(self->obj), &x, &y);
    
    return Py_BuildValue("(dd)", x, y);
}
#line 739 "crcanvas.c"


static PyObject *
_wrap_cr_canvas_center_on(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "x", "y", NULL };
    double x, y;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dd:Cr.Canvas.center_on", kwlist, &x, &y))
        return NULL;
    
    cr_canvas_center_on(CR_CANVAS(self->obj), x, y);
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 157 "crcanvas.override"
static PyObject *
_wrap_cr_canvas_get_center(PyGObject *self, PyObject *args)
{
    double x, y;
    
    cr_canvas_get_center(CR_CANVAS(self->obj), &x, &y);
    
    return Py_BuildValue("(dd)", x, y);
}
#line 767 "crcanvas.c"


static PyObject *
_wrap_cr_canvas_center_scale(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "x", "y", "w", "h", NULL };
    double x, y, w, h;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dddd:Cr.Canvas.center_scale", kwlist, &x, &y, &w, &h))
        return NULL;
    
    cr_canvas_center_scale(CR_CANVAS(self->obj), x, y, w, h);
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 168 "crcanvas.override"
static PyObject *
_wrap_cr_canvas_get_center_scale(PyGObject *self, PyObject *args)
{
    double x, y, w, h;
    
    cr_canvas_get_center_scale(CR_CANVAS(self->obj), &x, &y, &w, &h);
    
    return Py_BuildValue("(dddd)", x, y, w, h);
}
#line 795 "crcanvas.c"


static PyObject *
_wrap_cr_canvas_set_viewport(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "x1", "y1", "x2", "y2", NULL };
    double x1, y1, x2, y2;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dddd:Cr.Canvas.set_viewport", kwlist, &x1, &y1, &x2, &y2))
        return NULL;
    
    cr_canvas_set_viewport(CR_CANVAS(self->obj), x1, y1, x2, y2);
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 179 "crcanvas.override"
static PyObject *
_wrap_cr_canvas_get_viewport(PyGObject *self, PyObject *args)
{
    double x1, y1, x2, y2;
    
    cr_canvas_get_viewport(CR_CANVAS(self->obj), &x1, &y1, &x2, &y2);
    
    return Py_BuildValue("(dddd)", x1, y1, x2, y2);
}
#line 823 "crcanvas.c"


static PyObject *
_wrap_cr_canvas_zoom(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "x_factor", "y_factor", NULL };
    double x_factor, y_factor;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dd:Cr.Canvas.zoom", kwlist, &x_factor, &y_factor))
        return NULL;
    
    cr_canvas_zoom(CR_CANVAS(self->obj), x_factor, y_factor);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_canvas_zoom_world(PyGObject *self)
{
    
    cr_canvas_zoom_world(CR_CANVAS(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_canvas_set_max_scale_factor(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "x_factor", "y_factor", NULL };
    double x_factor, y_factor;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dd:Cr.Canvas.set_max_scale_factor", kwlist, &x_factor, &y_factor))
        return NULL;
    
    cr_canvas_set_max_scale_factor(CR_CANVAS(self->obj), x_factor, y_factor);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_canvas_set_min_scale_factor(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "x_factor", "y_factor", NULL };
    double x_factor, y_factor;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dd:Cr.Canvas.set_min_scale_factor", kwlist, &x_factor, &y_factor))
        return NULL;
    
    cr_canvas_set_min_scale_factor(CR_CANVAS(self->obj), x_factor, y_factor);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_canvas_set_repaint_mode(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "on", NULL };
    int on;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"i:Cr.Canvas.set_repaint_mode", kwlist, &on))
        return NULL;
    
    cr_canvas_set_repaint_mode(CR_CANVAS(self->obj), on);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_canvas_queue_repaint(PyGObject *self)
{
    
    cr_canvas_queue_repaint(CR_CANVAS(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyCrCanvas_methods[] = {
    { "set_vadjustment", (PyCFunction)_wrap_cr_canvas_set_vadjustment, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "set_hadjustment", (PyCFunction)_wrap_cr_canvas_set_hadjustment, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "set_scroll_region", (PyCFunction)_wrap_cr_canvas_set_scroll_region, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "get_scroll_region", (PyCFunction)_wrap_cr_canvas_get_scroll_region, METH_NOARGS,
      NULL },
    { "set_scroll_factor", (PyCFunction)_wrap_cr_canvas_set_scroll_factor, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "scroll_to", (PyCFunction)_wrap_cr_canvas_scroll_to, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "get_scroll_offsets", (PyCFunction)_wrap_cr_canvas_get_scroll_offsets, METH_NOARGS,
      NULL },
    { "center_on", (PyCFunction)_wrap_cr_canvas_center_on, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "get_center", (PyCFunction)_wrap_cr_canvas_get_center, METH_NOARGS,
      NULL },
    { "center_scale", (PyCFunction)_wrap_cr_canvas_center_scale, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "get_center_scale", (PyCFunction)_wrap_cr_canvas_get_center_scale, METH_NOARGS,
      NULL },
    { "set_viewport", (PyCFunction)_wrap_cr_canvas_set_viewport, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "get_viewport", (PyCFunction)_wrap_cr_canvas_get_viewport, METH_NOARGS,
      NULL },
    { "zoom", (PyCFunction)_wrap_cr_canvas_zoom, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "zoom_world", (PyCFunction)_wrap_cr_canvas_zoom_world, METH_NOARGS,
      NULL },
    { "set_max_scale_factor", (PyCFunction)_wrap_cr_canvas_set_max_scale_factor, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "set_min_scale_factor", (PyCFunction)_wrap_cr_canvas_set_min_scale_factor, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "set_repaint_mode", (PyCFunction)_wrap_cr_canvas_set_repaint_mode, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "queue_repaint", (PyCFunction)_wrap_cr_canvas_queue_repaint, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

static PyObject *
_wrap_cr_canvas__get_root(PyObject *self, void *closure)
{
    CrItem *ret;

    ret = CR_CANVAS(pygobject_get(self))->root;
    /* pygobject_new handles NULL checking */
    return pygobject_new((GObject *)ret);
}

static const PyGetSetDef cr_canvas_getsets[] = {
    { "root", (getter)_wrap_cr_canvas__get_root, (setter)0 },
    { NULL, (getter)0, (setter)0 },
};

PyTypeObject G_GNUC_INTERNAL PyCrCanvas_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Canvas",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyCrCanvas_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)cr_canvas_getsets,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)_wrap_cr_canvas_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrItem ----------- */

static PyObject *
_wrap_cr_item_invoke_paint(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "ct", "all", "x1", "y1", "x2", "y2", NULL };
    int all;
    double x1, y1, x2, y2;
    PycairoContext *ct;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!idddd:Cr.Item.invoke_paint", kwlist, &PycairoContext_Type, &ct, &all, &x1, &y1, &x2, &y2))
        return NULL;
    
    cr_item_invoke_paint(CR_ITEM(self->obj), ct->ctx, all, x1, y1, x2, y2);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_invoke_test(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "c", "x", "y", NULL };
    CrItem *ret;
    double x, y;
    PycairoContext *c;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!dd:Cr.Item.invoke_test", kwlist, &PycairoContext_Type, &c, &x, &y))
        return NULL;
    
    ret = cr_item_invoke_test(CR_ITEM(self->obj), c->ctx, x, y);
    
    /* pygobject_new handles NULL checking */
    return pygobject_new((GObject *)ret);
}

static PyObject *
_wrap_cr_item_report_old_bounds(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "ct", "all", NULL };
    int all;
    PycairoContext *ct;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!i:Cr.Item.report_old_bounds", kwlist, &PycairoContext_Type, &ct, &all))
        return NULL;
    
    cr_item_report_old_bounds(CR_ITEM(self->obj), ct->ctx, all);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_report_new_bounds(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "ct", "all", NULL };
    int all;
    PycairoContext *ct;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!i:Cr.Item.report_new_bounds", kwlist, &PycairoContext_Type, &ct, &all))
        return NULL;
    
    cr_item_report_new_bounds(CR_ITEM(self->obj), ct->ctx, all);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_calculate_bounds(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "bounds", "device_bounds", NULL };
    PyObject *py_bounds, *py_device_bounds;
    CrDeviceBounds *device_bounds = NULL;
    int ret;
    CrBounds *bounds = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"OO:Cr.Item.calculate_bounds", kwlist, &py_bounds, &py_device_bounds))
        return NULL;
    if (pyg_boxed_check(py_bounds, CR_TYPE_BOUNDS))
        bounds = pyg_boxed_get(py_bounds, CrBounds);
    else {
        PyErr_SetString(PyExc_TypeError, "bounds should be a CrBounds");
        return NULL;
    }
    if (pyg_boxed_check(py_device_bounds, CR_TYPE_DEVICE_BOUNDS))
        device_bounds = pyg_boxed_get(py_device_bounds, CrDeviceBounds);
    else {
        PyErr_SetString(PyExc_TypeError, "device_bounds should be a CrDeviceBounds");
        return NULL;
    }
    
    ret = cr_item_calculate_bounds(CR_ITEM(self->obj), bounds, device_bounds);
    
    return PyBool_FromLong(ret);

}

#line 283 "crcanvas.override"
static PyObject *
_wrap_cr_item_invoke_event(PyGObject *self, PyObject *args)
{
    int len;
    PyObject *py_event, *py_matrix;
    PyGObject *py_pick_item;
    CrItem *pick_item;
    cairo_matrix_t *matrix;
    GdkEvent *event;

    len = PyTuple_Size(args);
    if (len < 3) {
        PyErr_SetString(PyExc_TypeError, "invoke_event requires at least 3 "
                "arguments");
        return NULL;
    }
    if (!PyArg_ParseTuple (args, "OOO):Item.invoke_event", &py_event, 
            &py_matrix, &py_pick_item)) {
        return NULL;
    }

    event = pyg_boxed_get(self, GdkEvent);
    matrix = &((PycairoMatrix *)py_matrix)->matrix;
    pick_item = CR_ITEM(py_pick_item->obj);
    cr_item_invoke_event(CR_ITEM(self->obj), event, matrix, pick_item);
    
    Py_INCREF(Py_None);
    return Py_None;
}
#line 1138 "crcanvas.c"


#line 190 "crcanvas.override"
static PyObject *
_wrap_cr_item_get_bounds(PyGObject *self, PyObject *args)
{
    double x1, y1, x2, y2;

    if (!cr_item_get_bounds(CR_ITEM(self->obj), &x1, &y1, &x2, &y2)) {
        PyErr_SetString(PyExc_ValueError, "item has reported no bounds.");
        return NULL;
    }
    
    return Py_BuildValue("(dddd)", x1, y1, x2, y2);
}
#line 1154 "crcanvas.c"


#line 204 "crcanvas.override"
static PyObject *
_wrap_cr_item_get_device_bounds(PyGObject *self, PyObject *args)
{
    CrDeviceBounds bounds;
    PyGBoxed *gboxed;

    if (!cr_item_get_device_bounds(CR_ITEM(self->obj), &bounds)) {
        PyErr_SetString(PyExc_ValueError, "item has reported no bounds.");
        return NULL;
    }
    return pyg_boxed_new(CR_TYPE_DEVICE_BOUNDS, &bounds, TRUE, TRUE);
}
#line 1170 "crcanvas.c"


static PyObject *
_wrap_cr_item_request_update(PyGObject *self)
{
    
    cr_item_request_update(CR_ITEM(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 218 "crcanvas.override"
static PyObject *
_wrap_cr_item_get_inverse_matrix(PyGObject *self, PyObject *args, 
        PyObject *kwargs)
{
    cairo_matrix_t *matrix;

    matrix = cr_item_get_inverse_matrix(CR_ITEM(self->obj));
    return PycairoMatrix_FromMatrix(matrix);
}
#line 1193 "crcanvas.c"


static PyObject *
_wrap_cr_item_cancel_matrix(PyGObject *self)
{
    
    cr_item_cancel_matrix(CR_ITEM(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_add(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "child", NULL };
    PyGObject *child;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!:Cr.Item.add", kwlist, &PyCrItem_Type, &child))
        return NULL;
    
    cr_item_add(CR_ITEM(self->obj), CR_ITEM(child->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_remove(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "child", NULL };
    PyGObject *child;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!:Cr.Item.remove", kwlist, &PyCrItem_Type, &child))
        return NULL;
    
    cr_item_remove(CR_ITEM(self->obj), CR_ITEM(child->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_hide(PyGObject *self)
{
    
    cr_item_hide(CR_ITEM(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_show(PyGObject *self)
{
    
    cr_item_show(CR_ITEM(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_move(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "dx", "dy", NULL };
    double dx, dy;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"dd:Cr.Item.move", kwlist, &dx, &dy))
        return NULL;
    
    cr_item_move(CR_ITEM(self->obj), dx, dy);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_set_z_relative(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "child", "positions", NULL };
    PyGObject *child;
    int positions;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!i:Cr.Item.set_z_relative", kwlist, &PyCrItem_Type, &child, &positions))
        return NULL;
    
    cr_item_set_z_relative(CR_ITEM(self->obj), CR_ITEM(child->obj), positions);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_item_set_z(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "child", "position", NULL };
    PyGObject *child;
    int position;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!i:Cr.Item.set_z", kwlist, &PyCrItem_Type, &child, &position))
        return NULL;
    
    cr_item_set_z(CR_ITEM(self->obj), CR_ITEM(child->obj), position);
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 38 "crcanvas.override"
static PyObject *
_wrap_cr_item_set (PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GType              type;
    CrItem   *item;
    GObjectClass      *class;
    Py_ssize_t        pos = 0;
    PyObject          *value;
    PyObject          *key;

    item = CR_ITEM(self->obj);
    type = G_OBJECT_TYPE(item);
    class = G_OBJECT_GET_CLASS(item);
    
    g_object_freeze_notify (G_OBJECT(item));

    while (kwargs && PyDict_Next (kwargs, &pos, &key, &value)) {
	gchar *key_str = PyString_AsString (key);
	GParamSpec *pspec;
	GValue gvalue ={ 0, };

	pspec = g_object_class_find_property (class, key_str);
	if (!pspec) {
	    gchar buf[512];

	    g_snprintf(buf, sizeof(buf),
		       "canvas item `%s' doesn't support property `%s'",
		       g_type_name(type), key_str);
	    PyErr_SetString(PyExc_TypeError, buf);
	    g_object_thaw_notify (G_OBJECT(item));
	    return NULL;
	}

	g_value_init(&gvalue, G_PARAM_SPEC_VALUE_TYPE(pspec));
	if (pyg_value_from_pyobject(&gvalue, value)) {
	    gchar buf[512];

	    g_snprintf(buf, sizeof(buf),
		       "could not convert value for property `%s'", key_str);
	    PyErr_SetString(PyExc_TypeError, buf);
	    g_object_thaw_notify (G_OBJECT(item));
	    return NULL;
	}
	g_object_set_property(G_OBJECT(item), key_str, &gvalue);
	g_value_unset(&gvalue);
    }
    g_object_thaw_notify (G_OBJECT(item));

    Py_INCREF(Py_None);
    return Py_None;
}
#line 1355 "crcanvas.c"


static PyObject *
_wrap_CrItem__do_paint(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    gpointer klass;
    static char *kwlist[] = { "self", "c", NULL };
    PyGObject *self;
    PycairoContext *c;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!O!:Cr.Item.paint", kwlist, &PyCrItem_Type, &self, &PycairoContext_Type, &c))
        return NULL;
    klass = g_type_class_ref(pyg_type_from_object(cls));
    if (CR_ITEM_CLASS(klass)->paint)
        CR_ITEM_CLASS(klass)->paint(CR_ITEM(self->obj), c->ctx);
    else {
        PyErr_SetString(PyExc_NotImplementedError, "virtual method Cr.Item.paint not implemented");
        g_type_class_unref(klass);
        return NULL;
    }
    g_type_class_unref(klass);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_CrItem__do_calculate_bounds(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    gpointer klass;
    static char *kwlist[] = { "self", "c", "bounds", "device_bounds", NULL };
    CrDeviceBounds *device_bounds = NULL;
    PyObject *py_bounds, *py_device_bounds;
    PycairoContext *c;
    int ret;
    CrBounds *bounds = NULL;
    PyGObject *self;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!O!OO:Cr.Item.calculate_bounds", kwlist, &PyCrItem_Type, &self, &PycairoContext_Type, &c, &py_bounds, &py_device_bounds))
        return NULL;
    if (pyg_boxed_check(py_bounds, CR_TYPE_BOUNDS))
        bounds = pyg_boxed_get(py_bounds, CrBounds);
    else {
        PyErr_SetString(PyExc_TypeError, "bounds should be a CrBounds");
        return NULL;
    }
    if (pyg_boxed_check(py_device_bounds, CR_TYPE_DEVICE_BOUNDS))
        device_bounds = pyg_boxed_get(py_device_bounds, CrDeviceBounds);
    else {
        PyErr_SetString(PyExc_TypeError, "device_bounds should be a CrDeviceBounds");
        return NULL;
    }
    klass = g_type_class_ref(pyg_type_from_object(cls));
    if (CR_ITEM_CLASS(klass)->calculate_bounds)
        ret = CR_ITEM_CLASS(klass)->calculate_bounds(CR_ITEM(self->obj), c->ctx, bounds, device_bounds);
    else {
        PyErr_SetString(PyExc_NotImplementedError, "virtual method Cr.Item.calculate_bounds not implemented");
        g_type_class_unref(klass);
        return NULL;
    }
    g_type_class_unref(klass);
    return PyBool_FromLong(ret);

}

static PyObject *
_wrap_CrItem__do_test(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    gpointer klass;
    static char *kwlist[] = { "self", "c", "x", "y", NULL };
    PyGObject *self;
    double x, y;
    CrItem *ret;
    PycairoContext *c;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!O!dd:Cr.Item.test", kwlist, &PyCrItem_Type, &self, &PycairoContext_Type, &c, &x, &y))
        return NULL;
    klass = g_type_class_ref(pyg_type_from_object(cls));
    if (CR_ITEM_CLASS(klass)->test)
        ret = CR_ITEM_CLASS(klass)->test(CR_ITEM(self->obj), c->ctx, x, y);
    else {
        PyErr_SetString(PyExc_NotImplementedError, "virtual method Cr.Item.test not implemented");
        g_type_class_unref(klass);
        return NULL;
    }
    g_type_class_unref(klass);
    /* pygobject_new handles NULL checking */
    return pygobject_new((GObject *)ret);
}

static const PyMethodDef _PyCrItem_methods[] = {
    { "invoke_paint", (PyCFunction)_wrap_cr_item_invoke_paint, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "invoke_test", (PyCFunction)_wrap_cr_item_invoke_test, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "report_old_bounds", (PyCFunction)_wrap_cr_item_report_old_bounds, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "report_new_bounds", (PyCFunction)_wrap_cr_item_report_new_bounds, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "calculate_bounds", (PyCFunction)_wrap_cr_item_calculate_bounds, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "invoke_event", (PyCFunction)_wrap_cr_item_invoke_event, METH_VARARGS,
      NULL },
    { "get_bounds", (PyCFunction)_wrap_cr_item_get_bounds, METH_NOARGS,
      NULL },
    { "get_device_bounds", (PyCFunction)_wrap_cr_item_get_device_bounds, METH_NOARGS,
      NULL },
    { "request_update", (PyCFunction)_wrap_cr_item_request_update, METH_NOARGS,
      NULL },
    { "get_inverse_matrix", (PyCFunction)_wrap_cr_item_get_inverse_matrix, METH_NOARGS,
      NULL },
    { "cancel_matrix", (PyCFunction)_wrap_cr_item_cancel_matrix, METH_NOARGS,
      NULL },
    { "add", (PyCFunction)_wrap_cr_item_add, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "remove", (PyCFunction)_wrap_cr_item_remove, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "hide", (PyCFunction)_wrap_cr_item_hide, METH_NOARGS,
      NULL },
    { "show", (PyCFunction)_wrap_cr_item_show, METH_NOARGS,
      NULL },
    { "move", (PyCFunction)_wrap_cr_item_move, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "set_z_relative", (PyCFunction)_wrap_cr_item_set_z_relative, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "set_z", (PyCFunction)_wrap_cr_item_set_z, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "set", (PyCFunction)_wrap_cr_item_set, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "do_paint", (PyCFunction)_wrap_CrItem__do_paint, METH_VARARGS|METH_KEYWORDS|METH_CLASS,
      NULL },
    { "do_calculate_bounds", (PyCFunction)_wrap_CrItem__do_calculate_bounds, METH_VARARGS|METH_KEYWORDS|METH_CLASS,
      NULL },
    { "do_test", (PyCFunction)_wrap_CrItem__do_test, METH_VARARGS|METH_KEYWORDS|METH_CLASS,
      NULL },
    { NULL, NULL, 0, NULL }
};

#line 91 "crcanvas.override"
static PyObject *
_wrap_cr_item__get_items(PyGObject *self, void *closure)
{
    CrItem *parent = CR_ITEM(self->obj);
    PyObject         *list, *item;
    GList            *l;

    list = PyList_New(0);
    for (l = parent->items; l != NULL; l = l->next) {
        item = pygobject_new(G_OBJECT(l->data));
        PyList_Append(list, item);
        Py_DECREF(item);
    }
    return list;
}
#line 1509 "crcanvas.c"


#line 108 "crcanvas.override"
static PyObject *
_wrap_cr_item__get_matrix(PyGObject *self, void *closure)
{
    CrItem *item = CR_ITEM(self->obj);
    cairo_matrix_t *matrix;
    PyObject         *pyobject;

    matrix = cr_item_get_matrix(item);

    return PycairoMatrix_FromMatrix(matrix);
}
static int
_wrap_cr_item__set_matrix(PyGObject *self, PyGObject *value, void *closure)
{
    cairo_matrix_t *matrix;

    if (!pygobject_check(value, &PycairoMatrix_Type)) {
        PyErr_SetString(PyExc_TypeError, "must be a cairo_matrix_t object");
        return -1;
    }
    matrix = &((PycairoMatrix *)value)->matrix;
    *cr_item_get_matrix(CR_ITEM(self->obj)) =  *matrix;
    cr_item_request_update(CR_ITEM(self->obj));
    return 0;
}
#line 1538 "crcanvas.c"


static const PyGetSetDef cr_item_getsets[] = {
    { "items", (getter)_wrap_cr_item__get_items, (setter)0 },
    { "matrix", (getter)_wrap_cr_item__get_matrix, (setter)_wrap_cr_item__set_matrix },
    { NULL, (getter)0, (setter)0 },
};

PyTypeObject G_GNUC_INTERNAL PyCrItem_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Item",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyCrItem_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)cr_item_getsets,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};


static int
__CrItem_class_init(gpointer gclass, PyTypeObject *pyclass)
{

    /* overriding do_paint is currently not supported */

    /* overriding do_calculate_bounds is currently not supported */

    /* overriding do_test is currently not supported */
    return 0;
}


/* ----------- CrInverse ----------- */

PyTypeObject G_GNUC_INTERNAL PyCrInverse_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Inverse",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)NULL, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrPanner ----------- */

static PyObject *
_wrap_cr_panner_activate(PyGObject *self)
{
    
    cr_panner_activate(CR_PANNER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_panner_deactivate(PyGObject *self)
{
    
    cr_panner_deactivate(CR_PANNER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyCrPanner_methods[] = {
    { "activate", (PyCFunction)_wrap_cr_panner_activate, METH_NOARGS,
      NULL },
    { "deactivate", (PyCFunction)_wrap_cr_panner_deactivate, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyCrPanner_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Panner",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyCrPanner_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrPath ----------- */

#line 261 "crcanvas.override"
static PyObject *
_wrap_cr_path_setup_line(PyGObject *self, PyObject *args)
{
    cairo_t *c;
    CrPath *path;
    PyObject *py_cairo;

    if (!PyArg_ParseTuple (args, "O!:crcanvas.set_color",
        &PycairoContext_Type, &py_cairo)) {
        return NULL;
    }

    path = CR_PATH(self->obj);
    c = PycairoContext_GET(py_cairo);

    cr_path_setup_line(path, c);

    Py_INCREF(Py_None);
    return Py_None;
}
#line 1755 "crcanvas.c"


static const PyMethodDef _PyCrPath_methods[] = {
    { "setup_line", (PyCFunction)_wrap_cr_path_setup_line, METH_VARARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyCrPath_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Path",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyCrPath_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrLine ----------- */

PyTypeObject G_GNUC_INTERNAL PyCrLine_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Line",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)NULL, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrEllipse ----------- */

PyTypeObject G_GNUC_INTERNAL PyCrEllipse_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Ellipse",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)NULL, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrArrow ----------- */

static PyObject *
_wrap_cr_arrow_connect_parent(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "parent", NULL };
    PyGObject *parent;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!:Cr.Arrow.connect_parent", kwlist, &PyCrItem_Type, &parent))
        return NULL;
    
    cr_arrow_connect_parent(CR_ARROW(self->obj), CR_ITEM(parent->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyCrArrow_methods[] = {
    { "connect_parent", (PyCFunction)_wrap_cr_arrow_connect_parent, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyCrArrow_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Arrow",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyCrArrow_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrPixbuf ----------- */

PyTypeObject G_GNUC_INTERNAL PyCrPixbuf_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Pixbuf",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)NULL, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrRectangle ----------- */

PyTypeObject G_GNUC_INTERNAL PyCrRectangle_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Rectangle",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)NULL, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrRotator ----------- */

static PyObject *
_wrap_cr_rotator_activate(PyGObject *self)
{
    
    cr_rotator_activate(CR_ROTATOR(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_rotator_deactivate(PyGObject *self)
{
    
    cr_rotator_deactivate(CR_ROTATOR(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyCrRotator_methods[] = {
    { "activate", (PyCFunction)_wrap_cr_rotator_activate, METH_NOARGS,
      NULL },
    { "deactivate", (PyCFunction)_wrap_cr_rotator_deactivate, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyCrRotator_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Rotator",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyCrRotator_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrText ----------- */

PyTypeObject G_GNUC_INTERNAL PyCrText_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Text",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)NULL, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrVector ----------- */

PyTypeObject G_GNUC_INTERNAL PyCrVector_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Vector",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)NULL, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrZoomer ----------- */

static PyObject *
_wrap_cr_zoomer_activate(PyGObject *self)
{
    
    cr_zoomer_activate(CR_ZOOMER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_cr_zoomer_deactivate(PyGObject *self)
{
    
    cr_zoomer_deactivate(CR_ZOOMER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyCrZoomer_methods[] = {
    { "activate", (PyCFunction)_wrap_cr_zoomer_activate, METH_NOARGS,
      NULL },
    { "deactivate", (PyCFunction)_wrap_cr_zoomer_deactivate, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

static PyObject *
_wrap_cr_zoomer__get_box(PyObject *self, void *closure)
{
    CrItem *ret;

    ret = CR_ZOOMER(pygobject_get(self))->box;
    /* pygobject_new handles NULL checking */
    return pygobject_new((GObject *)ret);
}

static const PyGetSetDef cr_zoomer_getsets[] = {
    { "box", (getter)_wrap_cr_zoomer__get_box, (setter)0 },
    { NULL, (getter)0, (setter)0 },
};

PyTypeObject G_GNUC_INTERNAL PyCrZoomer_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Zoomer",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyCrZoomer_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)cr_zoomer_getsets,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- CrBlit ----------- */

PyTypeObject G_GNUC_INTERNAL PyCrBlit_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "crcanvas.Blit",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)NULL, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- functions ----------- */

#line 229 "crcanvas.override"
static PyObject *
_wrap_cr_item_make_temp_cairo(PyGObject *self)
{
    cairo_t *c;
    PyObject *obj;

    c = cr_item_make_temp_cairo();

    return PycairoContext_FromContext(c, &PycairoContext_Type, NULL);
}
#line 2406 "crcanvas.c"


#line 241 "crcanvas.override"
static PyObject *
_wrap_cr_path_set_color(PyGObject *self, PyObject *args)
{
    cairo_t *c;
    PyObject *py_cairo;
    guint32 rgba;

    if (!PyArg_ParseTuple (args, "O!k:crcanvas.set_color",
        &PycairoContext_Type, &py_cairo, &rgba)) {
        return NULL;
    }

    c = PycairoContext_GET(py_cairo);
    cr_path_set_color(c, rgba);

    Py_INCREF(Py_None);
    return Py_None;
}
#line 2428 "crcanvas.c"


const PyMethodDef pycrcanvas_functions[] = {
    { "make_temp_cairo", (PyCFunction)_wrap_cr_item_make_temp_cairo, METH_VARARGS,
      NULL },
    { "set_color", (PyCFunction)_wrap_cr_path_set_color, METH_VARARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

/* initialise stuff extension classes */
void
pycrcanvas_register_classes(PyObject *d)
{
    PyObject *module;

    if ((module = PyImport_ImportModule("gobject")) != NULL) {
        _PyGObject_Type = (PyTypeObject *)PyObject_GetAttrString(module, "GObject");
        if (_PyGObject_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name GObject from gobject");
            return ;
        }
    } else {
        PyErr_SetString(PyExc_ImportError,
            "could not import gobject");
        return ;
    }
    if ((module = PyImport_ImportModule("gtk._gtk")) != NULL) {
        _PyGtkAdjustment_Type = (PyTypeObject *)PyObject_GetAttrString(module, "Adjustment");
        if (_PyGtkAdjustment_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name Adjustment from gtk._gtk");
            return ;
        }
        _PyGtkWidget_Type = (PyTypeObject *)PyObject_GetAttrString(module, "Widget");
        if (_PyGtkWidget_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name Widget from gtk._gtk");
            return ;
        }
    } else {
        PyErr_SetString(PyExc_ImportError,
            "could not import gtk._gtk");
        return ;
    }


#line 2477 "crcanvas.c"
    pyg_register_boxed(d, "Bounds", CR_TYPE_BOUNDS, &PyCrBounds_Type);
    pyg_register_boxed(d, "DeviceBounds", CR_TYPE_DEVICE_BOUNDS, &PyCrDeviceBounds_Type);
    pyg_register_boxed(d, "Dash", CR_TYPE_DASH, &PyCrDash_Type);
    pygobject_register_class(d, "CrCanvas", CR_TYPE_CANVAS, &PyCrCanvas_Type, Py_BuildValue("(O)", &PyGtkWidget_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_CANVAS);
    pygobject_register_class(d, "CrItem", CR_TYPE_ITEM, &PyCrItem_Type, Py_BuildValue("(O)", &PyGObject_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_ITEM);
    pyg_register_class_init(CR_TYPE_ITEM, __CrItem_class_init);
    pygobject_register_class(d, "CrInverse", CR_TYPE_INVERSE, &PyCrInverse_Type, Py_BuildValue("(O)", &PyCrItem_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_INVERSE);
    pygobject_register_class(d, "CrPanner", CR_TYPE_PANNER, &PyCrPanner_Type, Py_BuildValue("(O)", &PyGObject_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_PANNER);
    pygobject_register_class(d, "CrPath", CR_TYPE_PATH, &PyCrPath_Type, Py_BuildValue("(O)", &PyCrItem_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_PATH);
    pygobject_register_class(d, "CrLine", CR_TYPE_LINE, &PyCrLine_Type, Py_BuildValue("(O)", &PyCrPath_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_LINE);
    pygobject_register_class(d, "CrEllipse", CR_TYPE_ELLIPSE, &PyCrEllipse_Type, Py_BuildValue("(O)", &PyCrPath_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_ELLIPSE);
    pygobject_register_class(d, "CrArrow", CR_TYPE_ARROW, &PyCrArrow_Type, Py_BuildValue("(O)", &PyCrPath_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_ARROW);
    pygobject_register_class(d, "CrPixbuf", CR_TYPE_PIXBUF, &PyCrPixbuf_Type, Py_BuildValue("(O)", &PyCrItem_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_PIXBUF);
    pygobject_register_class(d, "CrRectangle", CR_TYPE_RECTANGLE, &PyCrRectangle_Type, Py_BuildValue("(O)", &PyCrPath_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_RECTANGLE);
    pygobject_register_class(d, "CrRotator", CR_TYPE_ROTATOR, &PyCrRotator_Type, Py_BuildValue("(O)", &PyGObject_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_ROTATOR);
    pygobject_register_class(d, "CrText", CR_TYPE_TEXT, &PyCrText_Type, Py_BuildValue("(O)", &PyCrItem_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_TEXT);
    pygobject_register_class(d, "CrVector", CR_TYPE_VECTOR, &PyCrVector_Type, Py_BuildValue("(O)", &PyCrPath_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_VECTOR);
    pygobject_register_class(d, "CrZoomer", CR_TYPE_ZOOMER, &PyCrZoomer_Type, Py_BuildValue("(O)", &PyGObject_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_ZOOMER);
    pygobject_register_class(d, "CrBlit", CR_TYPE_BLIT, &PyCrBlit_Type, Py_BuildValue("(O)", &PyCrItem_Type));
    pyg_set_object_has_new_constructor(CR_TYPE_BLIT);
}
