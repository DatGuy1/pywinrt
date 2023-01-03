#include "pybase.h"
#include <winrt/base.h>

namespace py::cpp::_winrt
{
    /**
     * Python wrapper around System.Array type.
     */
    struct Array
    {
        PyObject_HEAD;
        std::unique_ptr<py::Array> array;
    };

    static constexpr const char* const Array_type_name = "Array";

    PyDoc_STRVAR(Array_doc, "class for wrapped COM array instances");

    /**
     * Allocates memory for a new System.Array object.
     * @param [in]  subtype The type to use for allocation.
     */
    static Array* Array_Alloc(PyTypeObject* subtype)
    {
        auto self = reinterpret_cast<Array*>(subtype->tp_alloc(subtype, 0));

        if (!self)
        {
            return nullptr;
        }

        // call C++ constructors on memory allocated from CPython heap
        new (&self->array) std::unique_ptr<py::Array>{};

        return self;
    }

    /**
     * Creates a new System.Array object from @p array.
     * @param [in]  array   A WinRT array such as a COM array.
     * @returns The new Python wrapper object or @c nullptr on error.
     */
    PyObject* Array_New(std::unique_ptr<py::Array> array) noexcept
    {
        auto type = get_python_type<py::Array>();

        if (!type)
        {
            return nullptr;
        }

        auto self = Array_Alloc(type);

        if (!self)
        {
            return nullptr;
        }

        self->array = std::move(array);

        return reinterpret_cast<PyObject*>(self);
    }

    /**
     * Assigns @p array to @p obj if @p obj is a System.Array.
     * @param [in]  obj     The Python System.Array wrapper object.
     * @param [in]  array   The WinRT array wrapper.
     * @returns @c true on success, otherwise returns @c false and sets Python
     * exception.
     */
    bool Array_Assign(PyObject* obj, std::unique_ptr<py::Array> array) noexcept
    {
        if (Py_TYPE(obj) != get_python_type<py::Array>())
        {
            {
                PyErr_SetString(PyExc_TypeError, "argument must be System.Array");
                return false;
            }
        }

        reinterpret_cast<Array*>(obj)->array = std::move(array);

        return true;
    }

    static PyObject* Array_tp_new(
        PyTypeObject* subtype, PyObject* args, PyObject* kwds) noexcept
    {
        auto self = Array_Alloc(subtype);

        if (!self)
        {
            return nullptr;
        }

        pyobj_handle self_handle{reinterpret_cast<PyObject*>(self)};

        PyObject* arg0;
        PyObject* arg1 = nullptr;

        if (PyArg_UnpackTuple(args, "Array", 1, 2, &arg0, &arg1) < 0)
        {
            return nullptr;
        }

        std::unique_ptr<py::Array> array;

        if (PyUnicode_Check(arg0))
        {
            if (PyUnicode_CompareWithASCIIString(arg0, "?") == 0)
            {
                self->array = std::make_unique<py::ComArray<bool>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "b") == 0)
            {
                self->array = std::make_unique<py::ComArray<int8_t>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "B") == 0)
            {
                self->array = std::make_unique<py::ComArray<uint8_t>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "h") == 0)
            {
                self->array = std::make_unique<py::ComArray<int16_t>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "H") == 0)
            {
                self->array = std::make_unique<py::ComArray<uint16_t>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "i") == 0)
            {
                self->array = std::make_unique<py::ComArray<int32_t>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "I") == 0)
            {
                self->array = std::make_unique<py::ComArray<uint32_t>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "q") == 0)
            {
                self->array = std::make_unique<py::ComArray<int64_t>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "Q") == 0)
            {
                self->array = std::make_unique<py::ComArray<uint64_t>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "f") == 0)
            {
                self->array = std::make_unique<py::ComArray<float>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "d") == 0)
            {
                self->array = std::make_unique<py::ComArray<double>>();
            }
            else if (PyUnicode_CompareWithASCIIString(arg0, "u") == 0)
            {
                self->array = std::make_unique<py::ComArray<char16_t>>();
            }
            else
            {
                PyErr_SetString(PyExc_ValueError, "unsupported format string");
                return nullptr;
            }
        }
        else if (PyType_Check(arg0))
        {
            // FIXME: where can we put this so it only imports once?
            PyDateTime_IMPORT;

            auto type = reinterpret_cast<PyTypeObject*>(arg0);

            // if a type has an _assign_array_ special method, use that to create
            // the py::Array and assign it to self->array.
            pyobj_handle assign_array{PyObject_GetAttrString(arg0, "_assign_array_")};

            if (!assign_array)
            {
                if (PyErr_ExceptionMatches(PyExc_AttributeError))
                {
                    PyErr_Clear();
                }
                else
                {
                    return nullptr;
                }
            }

            if (assign_array)
            {
                pyobj_handle result{
                    PyObject_CallFunction(assign_array.get(), "O", self)};

                if (!result)
                {
                    return nullptr;
                }
            }
            else if (type == &PyUnicode_Type)
            {
                self->array = std::make_unique<py::ComArray<winrt::hstring>>();
            }
            else if (std::strcmp(type->tp_name, "UUID") == 0) // TODO: stricter
                                                              // check
            {
                self->array = std::make_unique<py::ComArray<winrt::guid>>();
            }
            else if (type == PyDateTimeAPI->DateTimeType)
            {
                self->array = std::make_unique<
                    py::ComArray<winrt::Windows::Foundation::DateTime>>();
            }
            else if (type == PyDateTimeAPI->DeltaType)
            {
                self->array = std::make_unique<
                    py::ComArray<winrt::Windows::Foundation::TimeSpan>>();
            }
            else if (type == py::get_python_type<winrt::Windows::Foundation::Point>())
            {
                self->array = std::make_unique<
                    py::ComArray<winrt::Windows::Foundation::Point>>();
            }
            else if (type == py::get_python_type<winrt::Windows::Foundation::Size>())
            {
                self->array = std::make_unique<
                    py::ComArray<winrt::Windows::Foundation::Size>>();
            }
            else if (type == py::get_python_type<winrt::Windows::Foundation::Rect>())
            {
                self->array = std::make_unique<
                    py::ComArray<winrt::Windows::Foundation::Rect>>();
            }
            else if (type == py::winrt_type<Object>::get_python_type())
            {
                self->array = std::make_unique<
                    py::ComArray<winrt::Windows::Foundation::IInspectable>>();
            }
            else
            {
                PyErr_Format(
                    PyExc_TypeError,
                    "cannot use %.100s as type for WinRT array",
                    type->tp_name);
                return nullptr;
            }
        }
        else
        {
            PyErr_SetString(PyExc_TypeError, "first argument must be str or type");
            return nullptr;
        }

        if (!arg1)
        {
            // no args means empty array
            return self_handle.detach();
        }

        if (PyIndex_Check(arg1))
        {
            Py_ssize_t count = PyNumber_AsSsize_t(arg1, PyExc_OverflowError);

            if (count == -1 && PyErr_Occurred())
            {
                if (!PyErr_ExceptionMatches(PyExc_TypeError))
                {
                    return nullptr;
                }

                PyErr_Clear();
            }
            else
            {
                if (count < 0)
                {
                    PyErr_SetString(PyExc_ValueError, "negative count");
                    return nullptr;
                }

                uint32_t size = count;

                if (size != count)
                {
                    PyErr_SetString(PyExc_OverflowError, "count exceeds max size");
                    return nullptr;
                }

                if (!self->array->Alloc(size))
                {
                    return nullptr;
                }

                return self_handle.detach();
            }
        }

        if (PyObject_CheckBuffer(arg1))
        {
            Py_buffer view;

            if (PyObject_GetBuffer(arg1, &view, PyBUF_FULL_RO) < 0)
            {
                return nullptr;
            }

            using py_buffer_ptr
                = std::unique_ptr<Py_buffer, decltype(&PyBuffer_Release)>;
            py_buffer_ptr{&view, &PyBuffer_Release};

            if (view.itemsize != self->array->ValueSize())
            {
                PyErr_SetString(PyExc_TypeError, "itemsize is incorrect");
                return nullptr;
            }

            if (view.format == nullptr
                || std::strcmp(view.format, self->array->Format()) != 0)
            {
                PyErr_SetString(PyExc_TypeError, "format is incorrect");
                return nullptr;
            }

            uint32_t const size = view.len / view.itemsize;

            if (size != view.len / view.itemsize)
            {
                PyErr_SetString(PyExc_OverflowError, "count exceeds max size");
                return nullptr;
            }

            if (!self->array->Alloc(size))
            {
                return nullptr;
            }

            if (PyBuffer_ToContiguous(
                    self->array->Data(),
                    &view,
                    self->array->Size() * self->array->ValueSize(),
                    'C')
                < 0)
            {
                return nullptr;
            }

            return self_handle.detach();
        }

        if (PyList_CheckExact(arg1) || PyTuple_CheckExact(arg1))
        {
            Py_ssize_t count = PySequence_Fast_GET_SIZE(arg1);

            uint32_t size = count;

            if (size != count)
            {
                PyErr_SetString(PyExc_OverflowError, "count exceeds max size");
                return nullptr;
            }

            if (!self->array->Alloc(size))
            {
                return nullptr;
            }

            PyObject** items = PySequence_Fast_ITEMS(arg1);

            for (uint32_t i = 0; i < self->array->Size(); i++)
            {
                if (!self->array->Set(i, items[i]))
                {
                    return nullptr;
                }
            }

            return self_handle.detach();
        }

        PyErr_Format(
            PyExc_TypeError,
            "cannot convert '%.200s' object to Array",
            Py_TYPE(arg1)->tp_name);

        return nullptr;
    }

    static void Array_tp_dealloc(Array* self) noexcept
    {
        PyTypeObject* tp = Py_TYPE(self);
        std::destroy_at(&self->array);
        tp->tp_free(self);
        Py_DECREF(tp);
    }

    static PyMethodDef Array_tp_methods[] = {
#if PY_VERSION_HEX >= 0x03090000
        {"__class_getitem__",
         Py_GenericAlias,
         METH_O | METH_CLASS,
         PyDoc_STR("See PEP 585")},
#endif
        {}};

    static Py_ssize_t Array_sq_length(Array* self) noexcept
    {
        return self->array->Size();
    }

    PyObject* Array_sq_item(Array* self, Py_ssize_t index) noexcept
    {
        return self->array->At(index);
    }

    static int Array_bf_getbuffer(Array* self, Py_buffer* view, int flags) noexcept
    {
        view->obj = nullptr;

        if (((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE))
        {
            PyErr_SetString(PyExc_BufferError, "Array is not writable.");
            return -1;
        }

        view->readonly = 1;

        // required fields
        Py_INCREF(self);
        view->obj = reinterpret_cast<PyObject*>(self);
        view->buf = reinterpret_cast<void*>(self->array->Data());
        view->len
            = static_cast<Py_ssize_t>(self->array->Size()) * self->array->ValueSize();
        view->itemsize = self->array->ValueSize();
        view->ndim = 1;

        if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
        {
            view->format = const_cast<char*>(self->array->Format());
        }
        else
        {
            view->format = nullptr;
        }

        if ((flags & PyBUF_ND) == PyBUF_ND)
        {
            // HACK: using internal as a Py_ssize_t value so that we don't have
            // to allocate/free the extra info
            view->internal
                = reinterpret_cast<void*>(static_cast<uintptr_t>(self->array->Size()));
            view->shape = reinterpret_cast<Py_ssize_t*>(&view->internal);
        }
        else
        {
            view->internal = nullptr;
            view->shape = nullptr;
        }

        if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES)
        {
            view->strides = &(view->itemsize);
        }
        else
        {
            view->strides = nullptr;
        }

        view->suboffsets = nullptr;

        return 0;
    }

#if PY_VERSION_HEX < 0x03090000
    PyBufferProcs Array_buffer_procs
        = {reinterpret_cast<getbufferproc>(Array_bf_getbuffer), nullptr};
#endif

    static PyType_Slot Array_type_slots[] = {
        {Py_tp_doc, const_cast<char*>(Array_doc)},
        {Py_tp_new, Array_tp_new},
        {Py_tp_dealloc, Array_tp_dealloc},
        {Py_tp_methods, Array_tp_methods},
        {Py_sq_length, Array_sq_length},
        {Py_sq_item, Array_sq_item},
#if PY_VERSION_HEX >= 0x03090000
        {Py_bf_getbuffer, Array_bf_getbuffer},
#endif
        {},
    };

    PyType_Spec Array_type_spec
        = {"_winrt.Array", sizeof(Array), 0, Py_TPFLAGS_DEFAULT, Array_type_slots};
} // namespace py::cpp::_winrt
