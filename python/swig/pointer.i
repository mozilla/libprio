%module pointer

// Typemaps for dealing with the pointer to implementation idiom.
%define OPAQUE_POINTER(T)
    %{
        void T ## _PyCapsule_clear(PyObject *capsule) {
            T ptr = PyCapsule_GetPointer(capsule, "T");
            T ## _clear(ptr);
        }
    %}

    // We've assigned a destructor to the capsule instance, so prevent users from
    // manually destroying the pointer for safety.
    %ignore T ## _clear;

    // Get the pointer from the capsule
    %typemap(in) T, const_ ## T {
        $1 = PyCapsule_GetPointer($input, "T");
    }

    // Create a new capsule for the new pointer
    %typemap(out) T {
        $result = PyCapsule_New($1, "T", T ## _PyCapsule_clear);
    }

    // Create a temporary stack variable for allocating a new opaque pointer
    %typemap(in,numinputs=0) T* (T tmp = NULL) {
        $1 = &tmp;
    }

    // Return the pointer to the newly allocated memory
    %typemap(argout) T* {
        $result = SWIG_Python_AppendOutput(
            $result,PyCapsule_New(*$1, "T", T ## _PyCapsule_clear)
        );
    }
%enddef