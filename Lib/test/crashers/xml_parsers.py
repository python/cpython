from xml.parsers import expat

# http://python.org/sf/1296433

def test_parse_only_xml_data():
    #
    xml = "<?xml version='1.0' encoding='iso8859'?><s>%s</s>" % ('a' * 1025)
    # this one doesn't crash
    #xml = "<?xml version='1.0'?><s>%s</s>" % ('a' * 10000)

    def handler(text):
        raise Exception

    parser = expat.ParserCreate()
    parser.CharacterDataHandler = handler

    try:
        parser.Parse(xml)
    except:
        pass

if __name__ == '__main__':
    test_parse_only_xml_data()

# Invalid read of size 4
#    at 0x43F936: PyObject_Free (obmalloc.c:735)
#    by 0x45A7C7: unicode_dealloc (unicodeobject.c:246)
#    by 0x1299021D: PyUnknownEncodingHandler (pyexpat.c:1314)
#    by 0x12993A66: processXmlDecl (xmlparse.c:3330)
#    by 0x12999211: doProlog (xmlparse.c:3678)
#    by 0x1299C3F0: prologInitProcessor (xmlparse.c:3550)
#    by 0x12991EA3: XML_ParseBuffer (xmlparse.c:1562)
#    by 0x1298F8EC: xmlparse_Parse (pyexpat.c:895)
#    by 0x47B3A1: PyEval_EvalFrameEx (ceval.c:3565)
#    by 0x47CCAC: PyEval_EvalCodeEx (ceval.c:2739)
#    by 0x47CDE1: PyEval_EvalCode (ceval.c:490)
#    by 0x499820: PyRun_SimpleFileExFlags (pythonrun.c:1198)
#    by 0x4117F1: Py_Main (main.c:492)
#    by 0x12476D1F: __libc_start_main (in /lib/libc-2.3.5.so)
#    by 0x410DC9: (within /home/neal/build/python/svn/clean/python)
#  Address 0x12704020 is 264 bytes inside a block of size 592 free'd
#    at 0x11B1BA8A: free (vg_replace_malloc.c:235)
#    by 0x124B5F18: (within /lib/libc-2.3.5.so)
#    by 0x48DE43: find_module (import.c:1320)
#    by 0x48E997: import_submodule (import.c:2249)
#    by 0x48EC15: load_next (import.c:2083)
#    by 0x48F091: import_module_ex (import.c:1914)
#    by 0x48F385: PyImport_ImportModuleEx (import.c:1955)
#    by 0x46D070: builtin___import__ (bltinmodule.c:44)
#    by 0x4186CF: PyObject_Call (abstract.c:1777)
#    by 0x474E9B: PyEval_CallObjectWithKeywords (ceval.c:3432)
#    by 0x47928E: PyEval_EvalFrameEx (ceval.c:2038)
#    by 0x47CCAC: PyEval_EvalCodeEx (ceval.c:2739)
#    by 0x47CDE1: PyEval_EvalCode (ceval.c:490)
#    by 0x48D0F7: PyImport_ExecCodeModuleEx (import.c:635)
#    by 0x48D4F4: load_source_module (import.c:913)
