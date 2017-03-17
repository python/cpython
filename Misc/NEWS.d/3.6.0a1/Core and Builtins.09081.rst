Issue #22836: Ensure exception reports from PyErr_Display() and
PyErr_WriteUnraisable() are sensible even when formatting them produces
secondary errors.  This affects the reports produced by
sys.__excepthook__() and when __del__() raises an exception.