#! /usr/bin/env python
"""Simple test script for cryptmodule.c
   Roger E. Masse
"""
import crypt
print 'Test encryption: ', crypt.crypt('mypassword', 'ab')
