"""String table implementation for memory-efficient string storage in profiling data."""

class StringTable:
    """A string table for interning strings and reducing memory usage."""

    def __init__(self):
        self._strings = []
        self._string_to_index = {}

    def intern(self, string):
        """Intern a string and return its index.

        Args:
            string: The string to intern

        Returns:
            int: The index of the string in the table
        """
        if not isinstance(string, str):
            string = str(string)

        if string in self._string_to_index:
            return self._string_to_index[string]

        index = len(self._strings)
        self._strings.append(string)
        self._string_to_index[string] = index
        return index

    def get_string(self, index):
        """Get a string by its index.

        Args:
            index: The index of the string

        Returns:
            str: The string at the given index, or empty string if invalid
        """
        if 0 <= index < len(self._strings):
            return self._strings[index]
        return ""

    def get_strings(self):
        """Get the list of all strings in the table.

        Returns:
            list: A copy of the strings list
        """
        return self._strings.copy()

    def __len__(self):
        """Return the number of strings in the table."""
        return len(self._strings)
