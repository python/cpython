.. highlight:: c

Expat C API
===========

The :mod:`pyexpat <xml.parsers.expat>` module exposes selected Expat functions
through its ``expat_CAPI`` capsule. This allows extension modules to use the
same Expat library as Python. The interface is declared in :file:`pyexpat.h`,
which must be included after Expat's :file:`expat.h` header. It is not part of the
:ref:`Limited API <limited-c-api>`.

.. c:macro:: PyExpat_CAPSULE_NAME

   The name of the capsule containing the Expat C API:
   ``"pyexpat.expat_CAPI"``. Pass this to :c:func:`PyCapsule_Import` to obtain
   a pointer to a :c:struct:`PyExpat_CAPI`.


.. c:macro:: PyExpat_CAPI_MAGIC

   The string identifying the layout of :c:struct:`PyExpat_CAPI`.
   Before using the function pointers in the structure, check that
   its ``magic`` member matches this string, its ``size`` member is
   at least :c:expr:`sizeof(struct PyExpat_CAPI)`, and its
   :c:member:`PyExpat_CAPI.MAJOR_VERSION`, :c:member:`PyExpat_CAPI.MINOR_VERSION`,
   and :c:member:`PyExpat_CAPI.MICRO_VERSION` members match the Expat
   version used to build the extension.


.. c:struct:: PyExpat_CAPI

   Table of function pointers exported by :mod:`pyexpat <xml.parsers.expat>`.
   The capsule owns this structure: treat it as read-only and do not free it.
   Keep a strong reference to the capsule while using the table.

   Check the compatibility fields as described under
   :c:macro:`PyExpat_CAPI_MAGIC` before using the function pointers.

   .. c:member:: char *magic

      Interface identifier, set to :c:macro:`PyExpat_CAPI_MAGIC`.

   .. c:member:: int size

      Size of the structure in bytes, set to ``sizeof(struct PyExpat_CAPI)``
      by the :mod:`pyexpat <xml.parsers.expat>` module that created the capsule.

   .. c:member:: int MAJOR_VERSION
                 int MINOR_VERSION
                 int MICRO_VERSION

      Major, minor, and micro version numbers of the Expat library used by
      :mod:`pyexpat <xml.parsers.expat>`.

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
                 void (*SetUnknownEncodingHandler)(XML_Parser parser, XML_UnknownEncodingHandler handler, void *encodingHandlerData)
                 void (*SetUserData)(XML_Parser parser, void *userData)
                 void (*SetStartDoctypeDeclHandler)(XML_Parser parser, XML_StartDoctypeDeclHandler start)
                 enum XML_Status (*SetEncoding)(XML_Parser parser, const XML_Char *encoding)

      Pointers to the corresponding Expat functions. The function names are
      the member names prefixed with ``XML_``; for example, ``Parse`` points
      to ``XML_Parse``. See the `Expat API reference
      <https://libexpat.github.io/doc/api/latest/>`_ for their behavior.

   .. c:member:: int (*DefaultUnknownEncodingHandler)(void *encodingHandlerData, const XML_Char *name, XML_Encoding *info)

      Python's unknown-encoding handler for Expat. It uses Python's codec
      registry to fill *info* for the encoding *name*. The
      *encodingHandlerData* argument is unused and may be ``NULL``.

      Return ``XML_STATUS_OK`` on success or ``XML_STATUS_ERROR`` on failure.
      A Python exception may be set on failure. Calling this callback
      requires an :term:`attached thread state`.

   The remaining members also point to Expat functions with the ``XML_``
   prefix. They may be ``NULL``, depending on the Expat version used by
   :mod:`pyexpat <xml.parsers.expat>`:

   .. c:member:: int (*SetHashSalt)(XML_Parser parser, unsigned long hash_salt)

      ``NULL`` with Expat versions older than 2.1.0.

   .. c:member:: XML_Bool (*SetReparseDeferralEnabled)(XML_Parser parser, XML_Bool enabled)

      ``NULL`` with Expat versions older than 2.6.0.

   .. c:member:: XML_Bool (*SetAllocTrackerActivationThreshold)(XML_Parser parser, unsigned long long activationThresholdBytes)
                 XML_Bool (*SetAllocTrackerMaximumAmplification)(XML_Parser parser, float maxAmplificationFactor)

      ``NULL`` with Expat versions older than 2.7.2.

   .. c:member:: XML_Bool (*SetBillionLaughsAttackProtectionActivationThreshold)(XML_Parser parser, unsigned long long activationThresholdBytes)
                 XML_Bool (*SetBillionLaughsAttackProtectionMaximumAmplification)(XML_Parser parser, float maxAmplificationFactor)

      ``NULL`` with Expat versions older than 2.4.0.

   .. c:member:: XML_Bool (*SetHashSalt16Bytes)(XML_Parser parser, const uint8_t entropy[16])

      ``NULL`` with Expat versions older than 2.8.0.
