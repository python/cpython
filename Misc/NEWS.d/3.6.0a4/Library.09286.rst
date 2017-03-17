Issue #26750: unittest.mock.create_autospec() now works properly for
subclasses of property() and other data descriptors.  Removes the never
publicly used, never documented unittest.mock.DescriptorTypes tuple.