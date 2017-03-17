Issue #25958: Support "anti-registration" of special methods from
various ABCs, like __hash__, __iter__ or __len__.  All these (and
several more) can be set to None in an implementation class and the
behavior will be as if the method is not defined at all.
(Previously, this mechanism existed only for __hash__, to make
mutable classes unhashable.)  Code contributed by Andrew Barnert and
Ivan Levkivskyi.