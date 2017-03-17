Issue #26099: The site module now writes an error into stderr if
sitecustomize module can be imported but executing the module raise an
ImportError. Same change for usercustomize.