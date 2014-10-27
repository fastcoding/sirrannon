/* vim: shiftwidth=4 softtabstop=4 tabstop=4 
 */
/* -*- Mode: C; c-basic-offset: 4 -*- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <pygobject.h>
#include <pygtk/pygtk.h>
#include <pycairo.h>
#include <libcrcanvas.h>

/* Pre Python 2.5 backwards compatibility for PEP 353 */
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

Pycairo_CAPI_t *Pycairo_CAPI;

static PyObject *
crpoints_from_value(const GValue *value)
{
    CrPoints *points = (CrPoints *)g_value_get_boxed(value);
    PyObject *list;
    gint i;

    list = PyList_New(0);
    if (points)
	for (i = 0; i < points->array->len; i++) {
	    PyObject *item = PyFloat_FromDouble(g_array_index(points->array,
                                    double, i));

	    PyList_Append(list, item);
	    Py_DECREF(item);
	}
    return list;
}

static int
crpoints_to_value(GValue *value, PyObject *object)
{
    CrPoints *points;
    gint i, len;
    double v;

    if (!PySequence_Check(object))
        return -1;
    len = PySequence_Length(object);
    if (len % 2 != 0)
        return -1;

    points = cr_points_new();
    for (i = 0; i < len; i++) {
        PyObject *item = PySequence_GetItem(object, i);

        v = PyFloat_AsDouble(item);
        g_array_append_val(points->array, v);
        if (PyErr_Occurred()) {
            cr_points_unref(points);
            PyErr_Clear();
            Py_DECREF(item);
            return -1;
        }
        Py_DECREF(item);
    }
    g_value_set_boxed(value, points);
    cr_points_unref(points);
    return 0;
}

static PyObject *
crcontext_from_value(const GValue *value)
{
    cairo_t *ct = g_value_get_boxed(value);

    /* FIXME: why do I need to do this? */
    cairo_reference(ct);

    return PycairoContext_FromContext(ct, &PycairoContext_Type, NULL);
}

static int
crcontext_to_value(GValue *value, PyObject *object)
{
    cairo_t *ct;

    if (!pygobject_check(object, &PycairoContext_Type)) {
            PyErr_SetString(PyExc_TypeError, "must be a cairo_t object");
            return -1;
    }
    ct = ((PycairoContext *) object)->ctx;

    g_value_set_boxed(value, ct);
    return 0;
}

static PyObject *
crmatrix_from_value(const GValue *value)
{
    cairo_matrix_t *matrix = g_value_get_boxed(value);
    cairo_matrix_t m2;

    if (!matrix) {
            /* it is possible for the gvalue to be NULL */
            cairo_matrix_init_identity(&m2);
            matrix = &m2;
    }

    return PycairoMatrix_FromMatrix(matrix);
}

static int
crmatrix_to_value(GValue *value, PyObject *object)
{
    cairo_matrix_t *matrix;

    if (!pygobject_check(object, &PycairoMatrix_Type)) {
            PyErr_SetString(PyExc_TypeError, "must be a cairo_matrix_t object");
            return -1;
    }
    matrix = &((PycairoMatrix *) object)->matrix;

    g_value_set_boxed(value, matrix);
    return 0;
}

static PyObject *
crpattern_from_value(const GValue *value)
{
    cairo_pattern_t *pattern = g_value_get_boxed(value);

    if (!pattern) {
            /* it is possible for the gvalue to be NULL */
            Py_INCREF(Py_None);
            return Py_None;
    }
    cairo_pattern_reference(pattern);

#if CAIRO_VERSION_MAJOR <= 1 && CAIRO_VERSION_MINOR <= 8 && \
    CAIRO_VERSION_MICRO <= 6
    return PycairoPattern_FromPattern(pattern);
#else
    return PycairoPattern_FromPattern(pattern, NULL);
#endif
}

static int
crpattern_to_value(GValue *value, PyObject *object)
{
    cairo_pattern_t *pattern;

    if (object == Py_None) pattern = NULL;
    else if (!pygobject_check(object, &PycairoPattern_Type)) {
            PyErr_SetString(PyExc_TypeError, 
                            "must be a cairo_pattern_t object");
            return -1;
    }
    else pattern = ((PycairoPattern *) object)->pattern;

    g_value_set_boxed(value, pattern);
    return 0;
}

static PyObject *
crsurface_from_value(const GValue *value)
{
    cairo_surface_t *surface = g_value_get_boxed(value);

    if (!surface) {
            /* it is possible for the gvalue to be NULL */
            Py_INCREF(Py_None);
            return Py_None;
    }
    cairo_surface_reference(surface);

    return PycairoSurface_FromSurface(surface, NULL);
}

static int
crsurface_to_value(GValue *value, PyObject *object)
{
    cairo_surface_t *surface;

    if (object == Py_None) surface = NULL;

    else if (!pygobject_check(object, &PycairoSurface_Type)) {
            PyErr_SetString(PyExc_TypeError, 
                            "must be a cairo_surface_t object");
            return -1;
    }
    else surface = ((PycairoSurface *) object)->surface;

    g_value_set_boxed(value, surface);
    return 0;
}

void pycrcanvas_register_classes (PyObject *d);

extern PyMethodDef pycrcanvas_functions[];

DL_EXPORT(void)
initcrcanvas (void)
{
    PyObject *m, *d, *tuple;
	
    init_pygobject ();
    init_pygtk ();

    Pycairo_IMPORT;
    if (Pycairo_CAPI == NULL) return;

    pyg_register_boxed_custom(CR_TYPE_POINTS,
			      crpoints_from_value,
			      crpoints_to_value);
    pyg_register_boxed_custom(CR_TYPE_CONTEXT,
			      crcontext_from_value,
			      crcontext_to_value);
    pyg_register_boxed_custom(CR_TYPE_MATRIX,
			      crmatrix_from_value,
			      crmatrix_to_value);
    pyg_register_boxed_custom(CR_TYPE_PATTERN,
			      crpattern_from_value,
			      crpattern_to_value);
    pyg_register_boxed_custom(CR_TYPE_SURFACE,
			      crsurface_from_value,
			      crsurface_to_value);

    m = Py_InitModule ("crcanvas", pycrcanvas_functions);
    d = PyModule_GetDict (m);

    /* add crcanvas version */
    tuple = Py_BuildValue ("(ii)", CR_MAJOR_VERSION, CR_MINOR_VERSION);
    PyDict_SetItemString(d, "version", tuple);
    Py_DECREF(tuple);

    pycrcanvas_register_classes (d);
    /* pycrcanvas_add_constants (d, "CR_"); */
}

gboolean
pycrcanvas_set_property(GObject *gobject, const gchar *key_str, PyObject *value)
{
        GObjectClass *class;
        GParamSpec *pspec;
        GValue gvalue ={ 0, };

        class = G_OBJECT_GET_CLASS(gobject);
        pspec = g_object_class_find_property (class, key_str);
        if (!pspec) {
            gchar buf[512];

            g_snprintf(buf, sizeof(buf),
                       "object doesn't support property `%s'",
                       key_str);
            PyErr_SetString(PyExc_TypeError, buf);
            return FALSE;
        }

        g_value_init(&gvalue, G_PARAM_SPEC_VALUE_TYPE(pspec));

        if (pyg_value_from_pyobject(&gvalue, value)) {
                gchar buf[512];

                g_snprintf(buf, sizeof(buf),
                "could not convert value for property `%s'", 
                        key_str);

                PyErr_SetString(PyExc_TypeError, buf);
                return FALSE;
        }
        g_object_set_property(gobject, key_str, &gvalue);
        g_value_unset(&gvalue);
        return TRUE;
}

gboolean
pycrcanvas_parse_vargs(GObject *gobject, PyObject *kwargs)
{
        Py_ssize_t pos;
        PyObject *value, *key;

        g_object_freeze_notify (gobject);
        pos = 0;
        /* For each keyword ... */
        while (kwargs && PyDict_Next (kwargs, &pos, &key, &value)) {

                gchar *key_str = PyString_AsString (key);

                if (!pycrcanvas_set_property(gobject, key_str, value)) {
                        g_object_unref(gobject);
                        return FALSE;
                }
        }
        g_object_thaw_notify (gobject);
        return TRUE;
}

