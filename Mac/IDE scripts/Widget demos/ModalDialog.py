import W
from Carbon import Windows


w = W.ModalDialog((100, 100))

w.ed = W.EditText((10, 10, 80, 50))
w.ok = W.Button((10, 70, 80, 16), "Ok", w.close)
w.setdefaultbutton(w.ok)
w.open()
