"""Display interface abstractions for the live profiling collector."""

import contextlib
import curses
from abc import ABC, abstractmethod


class DisplayInterface(ABC):
    """Abstract interface for display operations to enable testing."""

    @abstractmethod
    def get_dimensions(self):
        """Get terminal dimensions as (height, width)."""
        pass

    @abstractmethod
    def clear(self):
        """Clear the screen."""
        pass

    @abstractmethod
    def refresh(self):
        """Refresh the screen to show changes."""
        pass

    @abstractmethod
    def redraw(self):
        """Redraw the entire window."""
        pass

    @abstractmethod
    def add_str(self, line, col, text, attr=0):
        """Add a string at the specified position."""
        pass

    @abstractmethod
    def get_input(self):
        """Get a character from input (non-blocking). Returns -1 if no input."""
        pass

    @abstractmethod
    def set_nodelay(self, flag):
        """Set non-blocking mode for input."""
        pass

    @abstractmethod
    def has_colors(self):
        """Check if terminal supports colors."""
        pass

    @abstractmethod
    def init_color_pair(self, pair_id, fg, bg):
        """Initialize a color pair."""
        pass

    @abstractmethod
    def get_color_pair(self, pair_id):
        """Get a color pair attribute."""
        pass

    @abstractmethod
    def get_attr(self, name):
        """Get a display attribute by name (e.g., 'A_BOLD', 'A_REVERSE')."""
        pass


class CursesDisplay(DisplayInterface):
    """Real curses display implementation."""

    def __init__(self, stdscr):
        self.stdscr = stdscr

    def get_dimensions(self):
        return self.stdscr.getmaxyx()

    def clear(self):
        self.stdscr.clear()

    def refresh(self):
        self.stdscr.refresh()

    def redraw(self):
        self.stdscr.redrawwin()

    def add_str(self, line, col, text, attr=0):
        try:
            height, width = self.get_dimensions()
            if 0 <= line < height and 0 <= col < width:
                max_len = width - col - 1
                if len(text) > max_len:
                    text = text[:max_len]
                self.stdscr.addstr(line, col, text, attr)
        except curses.error:
            pass

    def get_input(self):
        try:
            return self.stdscr.getch()
        except (KeyError, curses.error):
            return -1

    def set_nodelay(self, flag):
        self.stdscr.nodelay(flag)

    def has_colors(self):
        return curses.has_colors()

    def init_color_pair(self, pair_id, fg, bg):
        try:
            curses.init_pair(pair_id, fg, bg)
        except curses.error:
            pass

    def get_color_pair(self, pair_id):
        return curses.color_pair(pair_id)

    def get_attr(self, name):
        return getattr(curses, name, 0)


class MockDisplay(DisplayInterface):
    """Mock display for testing."""

    def __init__(self, height=40, width=160):
        self.height = height
        self.width = width
        self.buffer = {}
        self.cleared = False
        self.refreshed = False
        self.redrawn = False
        self.input_queue = []
        self.nodelay_flag = True
        self.colors_supported = True
        self.color_pairs = {}

    def get_dimensions(self):
        return (self.height, self.width)

    def clear(self):
        self.buffer.clear()
        self.cleared = True

    def refresh(self):
        self.refreshed = True

    def redraw(self):
        self.redrawn = True

    def add_str(self, line, col, text, attr=0):
        if 0 <= line < self.height and 0 <= col < self.width:
            max_len = self.width - col - 1
            if len(text) > max_len:
                text = text[:max_len]
            self.buffer[(line, col)] = (text, attr)

    def get_input(self):
        if self.input_queue:
            return self.input_queue.pop(0)
        return -1

    def set_nodelay(self, flag):
        self.nodelay_flag = flag

    def has_colors(self):
        return self.colors_supported

    def init_color_pair(self, pair_id, fg, bg):
        self.color_pairs[pair_id] = (fg, bg)

    def get_color_pair(self, pair_id):
        return pair_id << 8

    def get_attr(self, name):
        attrs = {
            "A_NORMAL": 0,
            "A_BOLD": 1 << 16,
            "A_REVERSE": 1 << 17,
            "A_UNDERLINE": 1 << 18,
            "A_DIM": 1 << 19,
        }
        return attrs.get(name, 0)

    def simulate_input(self, char):
        """Helper method for tests to simulate keyboard input."""
        self.input_queue.append(char)

    def get_text_at(self, line, col):
        """Helper method for tests to inspect buffer content."""
        if (line, col) in self.buffer:
            return self.buffer[(line, col)][0]
        return None

    def get_all_lines(self):
        """Get all display content as a list of lines (for testing)."""
        if not self.buffer:
            return []

        max_line = max(pos[0] for pos in self.buffer.keys())
        lines = []
        for line_num in range(max_line + 1):
            line_parts = []
            for col in range(self.width):
                if (line_num, col) in self.buffer:
                    text, _ = self.buffer[(line_num, col)]
                    line_parts.append((col, text))

            # Reconstruct line from parts
            if line_parts:
                line_parts.sort(key=lambda x: x[0])
                line = ""
                last_col = 0
                for col, text in line_parts:
                    if col > last_col:
                        line += " " * (col - last_col)
                    line += text
                    last_col = col + len(text)
                lines.append(line.rstrip())
            else:
                lines.append("")

        # Remove trailing empty lines
        while lines and not lines[-1]:
            lines.pop()

        return lines

    def find_text(self, pattern):
        """Find text matching pattern in buffer (for testing). Returns (line, col) or None."""
        for (line, col), (text, _) in self.buffer.items():
            if pattern in text:
                return (line, col)
        return None

    def contains_text(self, text):
        """Check if display contains the given text anywhere (for testing)."""
        return self.find_text(text) is not None
