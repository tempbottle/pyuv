
static Loop *default_loop = NULL;


static PyTypeObject LoopCountersResultType;

static PyStructSequence_Field loop_counters_result_fields[] = {
    {"eio_init", ""},
    {"req_init", ""},
    {"handle_init", ""},
    {"stream_init", ""},
    {"tcp_init", ""},
    {"udp_init", ""},
    {"pipe_init", ""},
    {"tty_init", ""},
    {"poll_init", ""},
    {"prepare_init", ""},
    {"check_init", ""},
    {"idle_init", ""},
    {"async_init", ""},
    {"timer_init", ""},
    {"process_init", ""},
    {"fs_event_init", ""},
    {NULL}
};

static PyStructSequence_Desc loop_counters_result_desc = {
    "loop_counters_result",
    NULL,
    loop_counters_result_fields,
    16
};


static void
_loop_cleanup(void)
{
    Py_XDECREF(default_loop);
}


static PyObject *
new_loop(PyTypeObject *type, PyObject *args, PyObject *kwargs, int is_default)
{
    if ((args && PyTuple_GET_SIZE(args)) || (kwargs && PyDict_Check(kwargs) && PyDict_Size(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "Loop initialization takes no parameters");
        return NULL;
    }

    if (is_default) {
        if (!default_loop) {
            default_loop = (Loop *)PyType_GenericNew(type, args, kwargs);
            if (!default_loop) {
                return NULL;
            }
            default_loop->uv_loop = uv_default_loop();
            default_loop->is_default = 1;
            default_loop->weakreflist = NULL;
            Py_AtExit(_loop_cleanup);
        }
        Py_INCREF(default_loop);
        return (PyObject *)default_loop;
    } else {
        Loop *self = (Loop *)PyType_GenericNew(type, args, kwargs);
        if (!self) {
            return NULL;
        }
        self->uv_loop = uv_loop_new();
        self->is_default = 0;
        self->weakreflist = NULL;
        return (PyObject *)self;
    }
}


static PyObject *
Loop_func_run(Loop *self)
{
    Py_BEGIN_ALLOW_THREADS
    uv_run(self->uv_loop);
    Py_END_ALLOW_THREADS
    if (PyErr_Occurred()) {
        PyErr_WriteUnraisable(Py_None);
    }
    Py_RETURN_NONE;
}


static PyObject *
Loop_func_run_once(Loop *self)
{
    int r;

    Py_BEGIN_ALLOW_THREADS
    r = uv_run_once(self->uv_loop);
    Py_END_ALLOW_THREADS
    if (PyErr_Occurred()) {
        PyErr_WriteUnraisable(Py_None);
    }
    return PyBool_FromLong((long)r);
}


static PyObject *
Loop_func_now(Loop *self)
{
    return PyLong_FromDouble(uv_now(self->uv_loop));
}


static PyObject *
Loop_func_update_time(Loop *self)
{
    uv_update_time(self->uv_loop);
    Py_RETURN_NONE;
}


static PyObject *
Loop_func_default_loop(PyObject *cls)
{
    UNUSED_ARG(cls);
    return new_loop(&LoopType, NULL, NULL, 1);
}


static PyObject *
Loop_active_handles_get(Loop *self, void *closure)
{
    ngx_queue_t *q;
    uv_handle_t *handle;
    PyObject *list, *item;

    UNUSED_ARG(closure);

    list = PyList_New(0);
    if (!list) {
        PyErr_NoMemory();
        return NULL;
    }
#ifdef UV_LEAN_AND_MEAN
    return list;
#else
    ngx_queue_foreach(q, &self->uv_loop->active_handles) {
        handle = ngx_queue_data(q, uv_handle_t, active_queue);
        if (handle->data) {
            item = (PyObject *)handle->data;
            ASSERT(item);
            if (PyList_Append(list, item))
                continue;
            /* No need to Py_DECREF here, we need the item to be referenced only once, and PyList_Append does that */
        }
    }
    return list;
#endif
}


static PyObject *
Loop_default_get(Loop *self, void *closure)
{
    UNUSED_ARG(closure);
    if (self->is_default) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}


static PyObject *
get_counter_value(uint64_t value)
{
    PyObject *val;
#if SIZEOF_TIME_T > SIZEOF_LONG
    val = PyLong_FromLongLong((PY_LONG_LONG)value);
#else
    val = PyInt_FromLong((long)value);
#endif
    return val;
}

static PyObject *
Loop_counters_get(Loop *self, void *closure)
{
    int pos;
    uv_counters_t* uv_counters;
    PyObject *counters;

    UNUSED_ARG(closure);
    uv_counters = &self->uv_loop->counters;

    counters =  PyStructSequence_New(&LoopCountersResultType);
    if (!counters) {
        PyErr_NoMemory();
        return NULL;
    }

    pos = 0;

    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->eio_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->req_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->handle_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->stream_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->tcp_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->udp_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->pipe_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->tty_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->poll_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->prepare_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->check_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->idle_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->async_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->timer_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->process_init));
    PyStructSequence_SET_ITEM(counters, pos++, get_counter_value(uv_counters->fs_event_init));

    return counters;

}


static PyObject *
Loop_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return new_loop(type, args, kwargs, 0);
}


static int
Loop_tp_traverse(Loop *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    return 0;
}


static int
Loop_tp_clear(Loop *self)
{
    Py_CLEAR(self->dict);
    return 0;
}


static void
Loop_tp_dealloc(Loop *self)
{
    if (self->uv_loop) {
        uv_loop_delete(self->uv_loop);
    }
    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)self);
    }
    Loop_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject*
Loop_dict_get(Loop *self, void* c)
{
    UNUSED_ARG(c);

    if (self->dict == NULL) {
        self->dict = PyDict_New();
        if (self->dict == NULL) {
            return NULL;
        }
    }
    Py_INCREF(self->dict);
    return self->dict;
}


static int
Loop_dict_set(Loop *self, PyObject* val, void* c)
{
    PyObject* tmp;

    UNUSED_ARG(c);

    if (val == NULL) {
        PyErr_SetString(PyExc_TypeError, "__dict__ may not be deleted");
        return -1;
    }
    if (!PyDict_Check(val)) {
        PyErr_SetString(PyExc_TypeError, "__dict__ must be a dictionary");
        return -1;
    }
    tmp = self->dict;
    Py_INCREF(val);
    self->dict = val;
    Py_XDECREF(tmp);
    return 0;
}


static PyMethodDef
Loop_tp_methods[] = {
    { "run", (PyCFunction)Loop_func_run, METH_NOARGS, "Run the event loop." },
    { "run_once", (PyCFunction)Loop_func_run_once, METH_NOARGS, "Run a single event loop iteration, waiting for events if necessary." },
    { "now", (PyCFunction)Loop_func_now, METH_NOARGS, "Return event loop time, expressed in nanoseconds." },
    { "update_time", (PyCFunction)Loop_func_update_time, METH_NOARGS, "Update event loop's notion of time by querying the kernel." },
    { "default_loop", (PyCFunction)Loop_func_default_loop, METH_CLASS|METH_NOARGS, "Instantiate the default loop." },
    { NULL }
};


static PyGetSetDef Loop_tp_getsets[] = {
    {"__dict__", (getter)Loop_dict_get, (setter)Loop_dict_set, NULL},
    {"active_handles", (getter)Loop_active_handles_get, NULL, "List of active handles in this loop", NULL},
    {"default", (getter)Loop_default_get, NULL, "Is this the default loop?", NULL},
    {"counters", (getter)Loop_counters_get, NULL, "Loop counters", NULL},
    {NULL}
};


static PyTypeObject LoopType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Loop",                                                    /*tp_name*/
    sizeof(Loop),                                                   /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Loop_tp_dealloc,                                    /*tp_dealloc*/
    0,                                                              /*tp_print*/
    0,                                                              /*tp_getattr*/
    0,                                                              /*tp_setattr*/
    0,                                                              /*tp_compare*/
    0,                                                              /*tp_repr*/
    0,                                                              /*tp_as_number*/
    0,                                                              /*tp_as_sequence*/
    0,                                                              /*tp_as_mapping*/
    0,                                                              /*tp_hash */
    0,                                                              /*tp_call*/
    0,                                                              /*tp_str*/
    0,                                                              /*tp_getattro*/
    0,                                                              /*tp_setattro*/
    0,                                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)Loop_tp_traverse,                                 /*tp_traverse*/
    (inquiry)Loop_tp_clear,                                         /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    offsetof(Loop, weakreflist),                                    /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Loop_tp_methods,                                                /*tp_methods*/
    0,                                                              /*tp_members*/
    Loop_tp_getsets,                                                /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    offsetof(Loop, dict),                                           /*tp_dictoffset*/
    0,                                                              /*tp_init*/
    0,                                                              /*tp_alloc*/
    Loop_tp_new,                                                    /*tp_new*/
};


