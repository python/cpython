
/* Memory object interface */

#ifndef Py_MEMORYOBJECT_H
#define Py_MEMORYOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	PyObject_HEAD
	PyObject *base;
	Py_buffer view;
} PyMemoryViewObject;


PyAPI_DATA(PyTypeObject) PyMemoryView_Type;

#define PyMemory_Check(op) (Py_Type(op) == &PyMemoryView_Type)
#define PyMemoryView(op) (((PyMemoryViewObject *)(op))->view)

#define Py_END_OF_MEMORY	(-1)

PyAPI_FUNC(PyObject *) PyMemoryView_GetContiguous(PyObject *base, 
						  int buffertype, 
						  char fort);

	/* Return a contiguous chunk of memory representing the buffer
	   from an object in a memory view object.  If a copy is made then the
	   base object for the memory view will be a *new* bytes object. 
	   
	   Otherwise, the base-object will be the object itself and no 
	   data-copying will be done. 

	   The buffertype argument can be PyBUF_READ, PyBUF_WRITE,
	   PyBUF_SHADOW to determine whether the returned buffer
	   should be READONLY, WRITABLE, or set to update the
	   original buffer if a copy must be made.  If buffertype is
	   PyBUF_WRITE and the buffer is not contiguous an error will
	   be raised.  In this circumstance, the user can use
	   PyBUF_SHADOW to ensure that a a writable temporary
	   contiguous buffer is returned.  The contents of this
	   contiguous buffer will be copied back into the original
	   object after the memoryview object is deleted as long as
	   the original object is writable and allows setting an
	   exclusive write lock. If this is not allowed by the
	   original object, then a BufferError is raised.
	   
	   If the object is multi-dimensional and if fortran is 'F',
	   the first dimension of the underlying array will vary the
	   fastest in the buffer.  If fortran is 'C', then the last
	   dimension will vary the fastest (C-style contiguous).  If
	   fortran is 'A', then it does not matter and you will get
	   whatever the object decides is more efficient.  

	   A new reference is returned that must be DECREF'd when finished.
	*/

PyAPI_FUNC(PyObject *) PyMemoryView_FromObject(PyObject *base);

PyAPI_FUNC(PyObject *) PyMemoryView_FromMemory(Py_buffer *info);
	/* create new if bufptr is NULL 
	    will be a new bytesobject in base */

#ifdef __cplusplus
}
#endif
#endif /* !Py_MEMORYOBJECT_H */
