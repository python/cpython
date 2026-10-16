# Test relative from imports with lazy keyword
lazy from .basic2 import x, f

def get_x():
    return x

def get_f():
    return f
