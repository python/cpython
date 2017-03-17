Issue #25195: Fix a regression in mock.MagicMock. _Call is a subclass of
tuple (changeset 3603bae63c13 only works for classes) so we need to
implement __ne__ ourselves.  Patch by Andrew Plummer.