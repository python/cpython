.. highlight:: c

Expat C API
-----------

:mod:`!pyexpat` exposes a C interface for extension modules.
Consumers must include the header file :file:`pyexpat.h` (which is not
included by default by :file:`Python.h`) and ``expat.h`` for Expat.

To use the C API, consider adding the following code in your extension
module initilisation function and store the pointer to the C API in
the module state:

.. code-block::

   struct PyExpat_CAPI *capi = NULL;
   capi = (struct PyExpat_CAPI *)PyCapsule_Import(PyExpat_CAPSULE_NAME, 0);
   if (capi == NULL) {
       goto error;
   }
   if (!(
        strcmp(capi->magic, PyExpat_CAPI_MAGIC) == 0
        && (size_t)capi->size >= sizeof(struct PyExpat_CAPI)
        && capi->MAJOR_VERSION == XML_MAJOR_VERSION
        && capi->MINOR_VERSION == XML_MINOR_VERSION
        && capi->MICRO_VERSION == XML_MICRO_VERSION
   )) {
       PyErr_SetString(PyExc_ImportError, "pyexpat version is incompatible");
       goto error;
   }


.. c:macro:: PyExpat_CAPI_MAGIC

   The Expat C API magic number.


.. c:macro:: PyExpat_CAPSULE_NAME

   The Expat C API capsule name to import via :c:func:`PyCapsule_Import`.


.. c:struct:: PyExpat_CAPI

   The Expat C API structure.

   .. below are members that were added in Python 2.x

   .. c:member:: char *magic

      Set to :c:macro:`PyExpat_CAPI_MAGIC`.

   .. c:member:: int size

      Set to ``sizeof(struct PyExpat_CAPI)``.

   .. c:member:: int MAJOR_VERSION
                 int MINOR_VERSION
                 int MICRO_VERSION

      Set to ``XML_MAJOR_VERSION``, ``XML_MINOR_VERSION``,
      and ``XML_MICRO_VERSION`` respectively of the linked
      Expat library.

   The following members expose the C API as provided by Expat.
   See the `official documentation <https://libexpat.github.io/doc/api/latest>`_
   for details (some names may slightly differ but the signature and intent
   will not).

   .. c:member:: const XML_LChar *(*ErrorString)(enum XML_Error errno)
   .. c:member:: enum XML_Error (*GetErrorCode)(XML_Parser parser)

   .. c:member:: XML_Size (*GetErrorColumnNumber)(XML_Parser parser)
   .. c:member:: XML_Size (*GetErrorLineNumber)(XML_Parser parser)

   .. c:member:: enum XML_Status (*Parse)(\
                   XML_Parser parser,\
                   const char *s, int len, int isFinal)

   .. c:member:: XML_Parser (*ParserCreate_MM)(\
                   const XML_Char *encoding,\
                   const XML_Memory_Handling_Suite *memsuite,\
                   const XML_Char *sep)

   .. c:member:: void (*ParserFree)(XML_Parser parser)

   .. c:member:: void (*SetCharacterDataHandler)(\
                   XML_Parser parser,\
                   XML_CharacterDataHandler handler)

   .. c:member:: void (*SetCommentHandler)(\
                   XML_Parser parser,\
                   XML_CommentHandler handler)

   .. c:member:: void (*SetDefaultHandlerExpand)(\
                   XML_Parser parser,\
                   XML_DefaultHandler handler)

   .. c:member:: void (*SetElementHandler)(\
                   XML_Parser parser,\
                   XML_StartElementHandler start,\
                   XML_EndElementHandler end)

   .. c:member:: void (*SetNamespaceDeclHandler)(\
                   XML_Parser parser,\
                   XML_StartNamespaceDeclHandler start,\
                   XML_EndNamespaceDeclHandler end)

   .. c:member:: void (*SetProcessingInstructionHandler)(\
                   XML_Parser parser,\
                   XML_ProcessingInstructionHandler handler)

   .. c:member:: void (*SetUnknownEncodingHandler)(\
                   XML_Parser parser,\
                   XML_UnknownEncodingHandler handler,\
                   void *encodingHandlerData)

   .. c:member:: void (*SetUserData)(\
                   XML_Parser parser,\
                   void *userData)

   .. below are members that were added after Python 3.0

   .. c:member:: void (*SetStartDoctypeDeclHandler)(\
                   XML_Parser parser,\
                   XML_StartDoctypeDeclHandler start)

      .. versionadded:: 3.2

   .. c:member:: enum XML_Status (*SetEncoding)(\
                   XML_Parser parser,\
                   const XML_Char *encoding)

      .. versionadded:: 3.3

   .. c:member:: int (*DefaultUnknownEncodingHandler)(\
                   void *encodingHandlerData,\
                   const XML_Char *name,\
                   XML_Encoding *info)

      .. versionadded:: 3.3

   .. c:member:: int (*SetHashSalt)(\
                   XML_Parser parser,\
                   unsigned long hash_salt)

      Might be ``NULL`` for Expat versions prior to 2.1.0.

      .. versionadded:: 3.4

   .. c:member:: XML_Bool (*SetReparseDeferralEnabled)(\
                   XML_Parser parser,\
                   XML_Bool enabled)

      Might be ``NULL`` for Expat versions prior to 2.6.0.

      .. versionadded:: 3.8

   .. c:member:: XML_Bool (*SetAllocTrackerActivationThreshold)(\
                   XML_Parser parser,\
                   unsigned long long activationThresholdBytes)

      Might be ``NULL`` for Expat versions prior to 2.7.2.

      .. uncomment this when the backport is done
      .. versionadded:: 3.10

   .. c:member:: XML_Bool (*SetAllocTrackerMaximumAmplification)(\
                   XML_Parser parser,\
                   float maxAmplificationFactor)

      Might be ``NULL`` for Expat versions prior to 2.7.2.

      .. uncomment this when the backport is done
      .. versionadded:: 3.10

   .. c:member:: XML_Bool (*SetBillionLaughsAttackProtectionActivationThreshold)(\
                   XML_Parser parser,\
                   unsigned long long activationThresholdBytes)

      Might be ``NULL`` for Expat versions prior to 2.4.0.

      .. uncomment this when the backport is done
      .. versionadded:: 3.10

   .. c:member:: XML_Bool (*SetBillionLaughsAttackProtectionMaximumAmplification)(\
                   XML_Parser parser,\
                   float maxAmplificationFactor)

      Might be ``NULL`` for Expat versions prior to 2.4.0.

      .. uncomment this when the backport is done
      .. versionadded:: 3.10
