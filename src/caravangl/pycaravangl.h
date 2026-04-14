#pragma once
#include <Python.h>
#include "caravangl.h"

typedef struct {
    PyObject_HEAD
    CaravanBuffer buf;
    PyObject *weakreflist; // Support for weak references
} PyCaravanBuffer;