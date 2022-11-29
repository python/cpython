#! /opt/python-master/bin/python3
import ssl

# Only install a ctype version if there isn't alreay a native implementation
if not hasattr(ssl.SSLSocket, 'export_keying_material'):
    import ctypes

    ssl_lib = ctypes.CDLL(ssl._ssl.SSL_DLL_PATH)

    ssl_lib.SSL_export_keying_material.argtypes = (
        ctypes.c_void_p,                  # SSL pointer
        ctypes.c_void_p, ctypes.c_size_t, # out pointer, out length
        ctypes.c_void_p, ctypes.c_size_t, # label buffer, label length
        ctypes.c_void_p, ctypes.c_size_t, # context, context length
        ctypes.c_int)                     # use context flag
    ssl_lib.SSL_export_keying_material.restype = ctypes.c_int

    def SSL_export_keying_material(self, label, key_len, context = None):
        sslobj = self._sslobj

        c_ssl_addr = sslobj.get_internal_addr()

        c_key_len = key_len
        c_key = ctypes.create_string_buffer(c_key_len)

        c_label_len = len(label)
        c_label = ctypes.create_string_buffer(label, c_label_len)

        if context is None:
            c_context_len = 0
            c_context = 0
            c_use_context = 0
        else:
            c_context_len = len(context)
            c_context = ctypes.create_string_buffer(context, c_context_len)
            c_use_context = 1

        if ssl_lib.SSL_export_keying_material(
                c_ssl_addr,
                c_key, c_key_len,
                c_label, c_label_len,
                c_context, c_context_len, c_use_context):
            return bytes(c_key)
        else:
            return None

    ssl.SSLSocket.export_keying_material = SSL_export_keying_material
