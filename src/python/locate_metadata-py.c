/* createrepo_c - Library of routines for manipulation with repodata
 * Copyright (C) 2013  Tomas Mlcoch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <Python.h>
#include <assert.h>
#include <stddef.h>

#include "locate_metadata-py.h"
#include "exception-py.h"
#include "typeconversion.h"

typedef struct {
    PyObject_HEAD
    struct cr_MetadataLocation *ml;
} _MetadataLocationObject;

struct cr_MetadataLocation *
MetadataLocation_FromPyObject(PyObject *o)
{
    if (!MetadataLocationObject_Check(o)) {
        PyErr_SetString(PyExc_TypeError, "Expected a createrepo_c.MetadataLocation object.");
        return NULL;
    }
    return ((_MetadataLocationObject *) o)->ml;
}

static int
check_MetadataLocationStatus(const _MetadataLocationObject *self)
{
    assert(self != NULL);
    assert(MetadataLocationObject_Check(self));
    if (self->ml == NULL) {
        PyErr_SetString(CrErr_Exception, "Improper createrepo_c MetadataLocation object.");
        return -1;
    }
    return 0;
}

/* Function on the type */

static PyObject *
metadatalocation_new(PyTypeObject *type,
                     G_GNUC_UNUSED PyObject *args,
                     G_GNUC_UNUSED PyObject *kwds)
{
    _MetadataLocationObject *self = (_MetadataLocationObject *)type->tp_alloc(type, 0);
    if (self)
        self->ml = NULL;
    return (PyObject *)self;
}

PyDoc_STRVAR(metadatalocation_init__doc__,
"Class representing location of metadata\n\n"
".. method:: __init__(path, ignore_db)\n\n"
"    :arg path: String with url/path to the repository\n"
"    :arg ignore_db: Boolean. If False then in case of remote repository\n"
"                    databases will not be downloaded)\n");

static int
metadatalocation_init(_MetadataLocationObject *self,
                      PyObject *args,
                      G_GNUC_UNUSED PyObject *kwds)
{
    char *repopath;
    PyObject *py_ignore_db = NULL;
    GError *tmp_err = NULL;

    if (!PyArg_ParseTuple(args, "sO|:metadatalocation_init", &repopath, &py_ignore_db))
        return -1;

    /* Free all previous resources when reinitialization */
    if (self->ml) {
        cr_metadatalocation_free(self->ml);
    }

    /* Init */
    self->ml = cr_locate_metadata(repopath, PyObject_IsTrue(py_ignore_db), &tmp_err);
    if (tmp_err) {
        nice_exception(&tmp_err, NULL);
        return -1;
    }
    return 0;
}

static void
metadatalocation_dealloc(_MetadataLocationObject *self)
{
    if (self->ml)
        cr_metadatalocation_free(self->ml);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
metadatalocation_repr(G_GNUC_UNUSED _MetadataLocationObject *self)
{
    return PyString_FromFormat("<createrepo_c.MetadataLocation object>");
}

/* MetadataLocation methods */

static struct PyMethodDef metadatalocation_methods[] = {
    {NULL} /* sentinel */
};

/* Mapping interface */

Py_ssize_t
length(_MetadataLocationObject *self)
{
    if (self->ml)
        return 9;
    return 0;
}

PyObject *
getitem(_MetadataLocationObject *self, PyObject *pykey)
{
    char *key, *value;

    if (check_MetadataLocationStatus(self))
        return NULL;

    if (!PyString_Check(pykey)) {
        PyErr_SetString(PyExc_TypeError, "String expected!");
        return NULL;
    }

    key = PyString_AsString(pykey);
    value = NULL;

    if (!strcmp(key, "primary")) {
        value = self->ml->pri_xml_href;
    } else if (!strcmp(key, "filelists")) {
        value = self->ml->fil_xml_href;
    } else if (!strcmp(key, "other")) {
        value = self->ml->oth_xml_href;
    } else if (!strcmp(key, "primary_db")) {
        value = self->ml->pri_sqlite_href;
    } else if (!strcmp(key, "filelists_db")) {
        value = self->ml->fil_sqlite_href;
    } else if (!strcmp(key, "other_db")) {
        value = self->ml->oth_sqlite_href;
    } else if (!strcmp(key, "group")) {
        value = self->ml->groupfile_href;
    } else if (!strcmp(key, "group_gz")) {
        value = self->ml->cgroupfile_href;
    } else if (!strcmp(key, "updateinfo")) {
        value = self->ml->updateinfo_href;
    }

    if (value)
        return PyString_FromString(value);
    else
        Py_RETURN_NONE;
}

static PyMappingMethods mapping_methods = {
    .mp_length = (lenfunc) length,
    .mp_subscript = (binaryfunc) getitem,
    .mp_ass_subscript = NULL,
};

/* Object */

PyTypeObject MetadataLocation_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                              /* ob_size */
    "createrepo_c.MetadataLocation",/* tp_name */
    sizeof(_MetadataLocationObject),/* tp_basicsize */
    0,                              /* tp_itemsize */
    (destructor)metadatalocation_dealloc,/* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_compare */
    (reprfunc)metadatalocation_repr,/* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    &mapping_methods,               /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    metadatalocation_init__doc__,   /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    PyObject_SelfIter,              /* tp_iter */
    0,                              /* tp_iternext */
    metadatalocation_methods,       /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc)metadatalocation_init,/* tp_init */
    0,                              /* tp_alloc */
    metadatalocation_new,           /* tp_new */
    0,                              /* tp_free */
    0,                              /* tp_is_gc */
};
