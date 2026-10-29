from math import copysign, isnan


class ExceptionIsLikeMixin:
    def assertExceptionIsLike(self, exc, template):
        """
        Passes when the provided `exc` matches the structure of `template`.
        Individual exceptions don't have to be the same objects or even pass
        an equality test: they only need to be the same type and contain equal
        `exc_obj.args`.
        """
        if exc is None and template is None:
            return

        if template is None:
            self.fail(f"unexpected exception: {exc}")

        if exc is None:
            self.fail(f"expected an exception like {template!r}, got None")

        if not isinstance(exc, ExceptionGroup):
            self.assertEqual(exc.__class__, template.__class__)
            self.assertEqual(exc.args[0], template.args[0])
        else:
            self.assertEqual(exc.message, template.message)
            self.assertEqual(len(exc.exceptions), len(template.exceptions))
            for e, t in zip(exc.exceptions, template.exceptions):
                self.assertExceptionIsLike(e, t)


class FloatsAreIdenticalMixin:
    def assertFloatsAreIdentical(self, x, y):
        """Fail unless floats x and y are identical, in the sense that:
        (1) both x and y are nans, or
        (2) both x and y are infinities, with the same sign, or
        (3) both x and y are zeros, with the same sign, or
        (4) x and y are both finite and nonzero, and x == y

        """
        msg = 'floats {!r} and {!r} are not identical'

        if isnan(x) or isnan(y):
            if isnan(x) and isnan(y):
                return
        elif x == y:
            if x != 0.0:
                return
            # both zero; check that signs match
            elif copysign(1.0, x) == copysign(1.0, y):
                return
            else:
                msg += ': zeros have different signs'
        self.fail(msg.format(x, y))


class ComplexesAreIdenticalMixin(FloatsAreIdenticalMixin):
    def assertComplexesAreIdentical(self, x, y):
        """Fail unless complex numbers x and y have equal values and signs.

        In particular, if x and y both have real (or imaginary) part
        zero, but the zeros have different signs, this test will fail.

        """
        self.assertFloatsAreIdentical(x.real, y.real)
        self.assertFloatsAreIdentical(x.imag, y.imag)
