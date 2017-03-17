Issue #25791: If __package__ != __spec__.parent or if neither __package__ or
__spec__ are defined then ImportWarning is raised.