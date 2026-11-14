.. highlight:: c

Expat C API
===========

The :mod:`xml.parsers.expat` module exposes selected Expat functions
through its ``expat_CAPI`` capsule. This allows extension modules to use the
same Expat library as Python. The interface is declared in :file:`pyexpat.h`,
which is not included by :file:`Python.h` and must be included after Expat's
:file:`expat.h` header.

To use the C API, add the following code to the extension module
initialization function and store the pointer to the C API in the module
state:

.. code-block::

   struct PyExpat_CAPI *capi = NULL;
   capi = (struct PyExpat_CAPI *)PyCapsule_Import(PyExpat_CAPSULE_NAME, 0);
   if (capi == NULL) {
       goto error;
   }
   if (!(
        strcmp(capi->magic, PyExpat_CAPI_MAGIC) == 0
        && (size_t)capi->size >= sizeof(struct PyExpat_CAPI)
   )) {
       PyErr_SetString(PyExc_ImportError, "pyexpat version is incompatible");
       goto error;
   }

.. c:macro:: PyExpat_CAPSULE_NAME

   The name of the capsule containing the Expat C API:
   ``"pyexpat.expat_CAPI"``. Pass this to :c:func:`PyCapsule_Import` to obtain
   a pointer to a :c:struct:`PyExpat_CAPI`.


.. c:macro:: PyExpat_CAPI_MAGIC

   The string identifying the layout of :c:struct:`PyExpat_CAPI`.
   Before using the function pointers in the structure, check that
   its ``magic`` member compares equal to this string using ``strcmp()``
   and its ``size`` member is at least :c:expr:`sizeof(struct PyExpat_CAPI)`.


.. c:struct:: PyExpat_CAPI

   Table of function pointers exported by :mod:`xml.parsers.expat`.
   The capsule owns this structure: treat it as read-only and do not free it.
   Keep a strong reference to the capsule while using the table.

   Check the compatibility fields as described under
   :c:macro:`PyExpat_CAPI_MAGIC` before using the function pointers.

   .. c:member:: char *magic

      Interface identifier, set to :c:macro:`PyExpat_CAPI_MAGIC`.

   .. c:member:: int size

      Size of the structure in bytes, set to ``sizeof(struct PyExpat_CAPI)``
      by the :mod:`xml.parsers.expat` module that created the capsule.

   .. c:member:: int MAJOR_VERSION
                 int MINOR_VERSION
                 int MICRO_VERSION

      Major, minor, and micro version numbers of the Expat library used by
      :mod:`xml.parsers.expat`, set to ``XML_MAJOR_VERSION``,
      ``XML_MINOR_VERSION``, and ``XML_MICRO_VERSION`` respectively.

   .. c:member:: void (*SetUnknownEncodingHandler)(XML_Parser parser, XML_UnknownEncodingHandler handler, void *encodingHandlerData)

      Pointer to `XML_SetUnknownEncodingHandler
      <https://libexpat.github.io/doc/api/latest/#XML_SetUnknownEncodingHandler>`_.
      The handler may set a Python exception on failure. The caller must
      check for a Python exception after parsing.

   .. c:member:: int (*DefaultUnknownEncodingHandler)(void *encodingHandlerData, const XML_Char *name, XML_Encoding *info)

      Python's unknown-encoding handler for Expat, used with
      ``XML_SetUnknownEncodingHandler``. It uses Python's codec
      registry to fill *info*.
      The *encodingHandlerData* argument is unused.
      Calling this handler requires an :term:`attached thread state`.

      .. versionadded:: 3.3

   Pointers to the corresponding Expat functions, or ``NULL`` if not
   available in the Expat version used by :mod:`xml.parsers.expat`, are
   listed below.

   Member names omit the ``XML_`` prefix; for example, ``SetHashSalt``
   exposes Expat's ``XML_SetHashSalt`` function. See the `Expat API reference
   <https://libexpat.github.io/doc/api/latest/>`_ for their behavior.

   .. c:member:: const XML_LChar *(*ErrorString)(enum XML_Error code)
                 enum XML_Error (*GetErrorCode)(XML_Parser parser)
                 XML_Size (*GetErrorColumnNumber)(XML_Parser parser)
                 XML_Size (*GetErrorLineNumber)(XML_Parser parser)
                 enum XML_Status (*Parse)(XML_Parser parser, const char *s, int len, int isFinal)
                 XML_Parser (*ParserCreate_MM)(const XML_Char *encoding, const XML_Memory_Handling_Suite *memsuite, const XML_Char *namespaceSeparator)
                 void (*ParserFree)(XML_Parser parser)
                 void (*SetCharacterDataHandler)(XML_Parser parser, XML_CharacterDataHandler handler)
                 void (*SetCommentHandler)(XML_Parser parser, XML_CommentHandler handler)
                 void (*SetDefaultHandlerExpand)(XML_Parser parser, XML_DefaultHandler handler)
                 void (*SetElementHandler)(XML_Parser parser, XML_StartElementHandler start, XML_EndElementHandler end)
                 void (*SetNamespaceDeclHandler)(XML_Parser parser, XML_StartNamespaceDeclHandler start, XML_EndNamespaceDeclHandler end)
                 void (*SetProcessingInstructionHandler)(XML_Parser parser, XML_ProcessingInstructionHandler handler)
                 void (*SetUserData)(XML_Parser parser, void *userData)

   .. c:member:: void (*SetStartDoctypeDeclHandler)(XML_Parser parser, XML_StartDoctypeDeclHandler start)

      .. versionadded:: 3.2

   .. c:member:: enum XML_Status (*SetEncoding)(XML_Parser parser, const XML_Char *encoding)

      .. versionadded:: 3.3

   .. c:member:: int (*SetHashSalt)(XML_Parser parser, unsigned long hash_salt)

      Might be ``NULL`` for Expat versions prior to 2.1.0.

      .. versionadded:: 3.4

   .. c:member:: XML_Bool (*SetReparseDeferralEnabled)(XML_Parser parser, XML_Bool enabled)

      Might be ``NULL`` for Expat versions prior to 2.6.0.

      .. versionadded:: 3.8

   .. c:member:: XML_Bool (*SetAllocTrackerActivationThreshold)(XML_Parser parser, unsigned long long activationThresholdBytes)

      Might be ``NULL`` for Expat versions prior to 2.7.2.

      .. versionadded:: 3.10

   .. c:member:: XML_Bool (*SetAllocTrackerMaximumAmplification)(XML_Parser parser, float maxAmplificationFactor)

      Might be ``NULL`` for Expat versions prior to 2.7.2.

      .. versionadded:: 3.10

   .. c:member:: XML_Bool (*SetBillionLaughsAttackProtectionActivationThreshold)(XML_Parser parser, unsigned long long activationThresholdBytes)

      Might be ``NULL`` for Expat versions prior to 2.4.0.

      .. versionadded:: 3.10

   .. c:member:: XML_Bool (*SetBillionLaughsAttackProtectionMaximumAmplification)(XML_Parser parser, float maxAmplificationFactor)

      Might be ``NULL`` for Expat versions prior to 2.4.0.

      .. versionadded:: 3.10

   .. c:member:: XML_Bool (*SetHashSalt16Bytes)(XML_Parser parser, const uint8_t entropy[16])

      Might be ``NULL`` for Expat versions prior to 2.8.0.

      .. versionadded:: 3.16
