Issue #27366: Implemented PEP 487 (Simpler customization of class creation).
Upon subclassing, the __init_subclass__ classmethod is called on the base
class. Descriptors are initialized with __set_name__ after class creation.