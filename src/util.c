
static PyObject* PyExc_UVError;


static PyObject *
Util_func_hrtime(PyObject *obj)
{
    UNUSED_ARG(obj);
    return PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)uv_hrtime());
}


static PyObject *
Util_func_get_free_memory(PyObject *obj)
{
    UNUSED_ARG(obj);
    return PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)uv_get_free_memory());
}


static PyObject *
Util_func_get_total_memory(PyObject *obj)
{
    UNUSED_ARG(obj);
    return PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)uv_get_total_memory());
}


static PyObject *
Util_func_loadavg(PyObject *obj)
{
    double avg[3];

    UNUSED_ARG(obj);

    uv_loadavg(avg);
    return Py_BuildValue("(ddd)", avg[0], avg[1], avg[2]);
}


static PyObject *
Util_func_uptime(PyObject *obj)
{
    double uptime;
    uv_err_t err;
    PyObject *exc_data;

    UNUSED_ARG(obj);

    err = uv_uptime(&uptime);
    if (err.code == UV_OK) {
        return PyFloat_FromDouble(uptime);
    } else {
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_resident_set_memory(PyObject *obj)
{
    size_t rss;
    uv_err_t err;
    PyObject *exc_data;

    UNUSED_ARG(obj);

    err = uv_resident_set_memory(&rss);
    if (err.code == UV_OK) {
        return PyInt_FromSsize_t(rss);
    } else {
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_interface_addresses(PyObject *obj)
{
    int i, count;
    char ip[INET6_ADDRSTRLEN];
    uv_interface_address_t* interfaces;
    uv_err_t err;
    PyObject *result, *item, *exc_data;

    UNUSED_ARG(obj);

    err = uv_interface_addresses(&interfaces, &count);
    if (err.code == UV_OK) {
        result = PyList_New(0);
        if (!result) {
            uv_free_interface_addresses(interfaces, count);
            PyErr_NoMemory();
            return NULL;
        }
        for (i = 0; i < count; i++) {
            item = PyDict_New();
            if (!item)
                continue;
            PyDict_SetItemString(item, "name", PyString_FromString(interfaces[i].name));
            PyDict_SetItemString(item, "is_internal", PyBool_FromLong((long)interfaces[i].is_internal));
            if (interfaces[i].address.address4.sin_family == AF_INET) {
                uv_ip4_name(&interfaces[i].address.address4, ip, INET_ADDRSTRLEN);
                PyDict_SetItemString(item, "address", PyString_FromString(ip));
            } else if (interfaces[i].address.address4.sin_family == AF_INET6) {
                uv_ip6_name(&interfaces[i].address.address6, ip, INET6_ADDRSTRLEN);
                PyDict_SetItemString(item, "address", PyString_FromString(ip));
            }
            if (PyList_Append(result, item))
                continue;
            Py_DECREF(item);
        }
        uv_free_interface_addresses(interfaces, count);
        return result;
    } else {
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_cpu_info(PyObject *obj)
{
    int i, count;
    uv_cpu_info_t* cpus;
    uv_err_t err;
    PyObject *result, *item, *times, *exc_data;

    UNUSED_ARG(obj);

    err = uv_cpu_info(&cpus, &count);
    if (err.code == UV_OK) {
        result = PyList_New(0);
        if (!result) {
            uv_free_cpu_info(cpus, count);
            PyErr_NoMemory();
            return NULL;
        }
        for (i = 0; i < count; i++) {
            item = PyDict_New();
            times = PyDict_New();
            if (!item || !times)
                continue;
            PyDict_SetItemString(item, "model", PyString_FromString(cpus[i].model));
            PyDict_SetItemString(item, "speed", PyInt_FromLong((long)cpus[i].speed));
            PyDict_SetItemString(item, "times", times);
            if (PyList_Append(result, item))
                continue;
            Py_DECREF(item);
            PyDict_SetItemString(times, "sys", PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.sys));
            PyDict_SetItemString(times, "user", PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.user));
            PyDict_SetItemString(times, "idle", PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.idle));
            PyDict_SetItemString(times, "irq", PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.irq));
            PyDict_SetItemString(times, "nice", PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.nice));
        }
        uv_free_cpu_info(cpus, count);
        return result;
    } else {
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static Bool setup_args_called = False;

static void
setup_args(void)
{
    int r, argc;
    char **argv;
    void (*Py_GetArgcArgv)(int *argc, char ***argv);
    uv_lib_t dlmain;

    r = uv_dlopen(NULL, &dlmain);
    if (r != 0) {
        return;
    }

    r = uv_dlsym(&dlmain, "Py_GetArgcArgv", (void **)&Py_GetArgcArgv);
    if (r != 0) {
        uv_dlclose(&dlmain);
        return;
    }

    Py_GetArgcArgv(&argc, &argv);
    uv_dlclose(&dlmain);

    uv_setup_args(argc, argv);
    setup_args_called = True;
}


static PyObject *
Util_func_set_process_title(PyObject *self, PyObject *args)
{
    char *title;
    uv_err_t err;

    if (!PyArg_ParseTuple(args, "s:set_process_title", &title)) {
        return NULL;
    }

    if (!setup_args_called) {
        setup_args();
    }

    err = uv_set_process_title(title);
    if (err.code == UV_OK) {
        Py_RETURN_NONE;
    } else {
        PyObject *exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_get_process_title(PyObject *self)
{
    char buffer[512];
    uv_err_t err;

    if (!setup_args_called) {
        setup_args();
    }

    err = uv_get_process_title(buffer, sizeof(buffer));
    if (err.code == UV_OK) {
        return PyString_FromString(buffer);
    } else {
        PyObject *exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyMethodDef
Util_methods[] = {
    { "hrtime", (PyCFunction)Util_func_hrtime, METH_NOARGS, "High resolution time." },
    { "get_free_memory", (PyCFunction)Util_func_get_free_memory, METH_NOARGS, "Get system free memory." },
    { "get_total_memory", (PyCFunction)Util_func_get_total_memory, METH_NOARGS, "Get system total memory." },
    { "loadavg", (PyCFunction)Util_func_loadavg, METH_NOARGS, "Get system load average." },
    { "uptime", (PyCFunction)Util_func_uptime, METH_NOARGS, "Get system uptime." },
    { "resident_set_memory", (PyCFunction)Util_func_resident_set_memory, METH_NOARGS, "Gets resident memory size for the current process." },
    { "interface_addresses", (PyCFunction)Util_func_interface_addresses, METH_NOARGS, "Gets network interface addresses." },
    { "cpu_info", (PyCFunction)Util_func_cpu_info, METH_NOARGS, "Gets system CPU information." },
    { "set_process_title", (PyCFunction)Util_func_set_process_title, METH_VARARGS, "Sets current process title." },
    { "get_process_title", (PyCFunction)Util_func_get_process_title, METH_NOARGS, "Gets current process title." },
    { NULL }
};

#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_util_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv.util",            /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    Util_methods,           /*m_methods*/
};
#endif

PyObject *
init_util(void)
{
    PyObject *module;
#ifdef PYUV_PYTHON3
    module = PyModule_Create(&pyuv_util_module);
#else
    module = Py_InitModule("pyuv.util", Util_methods);
#endif
    if (module == NULL) {
        return NULL;
    }

    return module;
}


