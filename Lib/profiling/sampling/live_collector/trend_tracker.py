"""TrendTracker - Encapsulated trend tracking for live profiling metrics.

This module provides trend tracking functionality for profiling metrics,
calculating direction indicators (up/down/stable) and managing associated
visual attributes like colors.
"""

import curses
from typing import Dict, Literal, Any

TrendDirection = Literal["up", "down", "stable"]


class TrendTracker:
    """
    Tracks metric trends over time and provides visual indicators.

    This class encapsulates all logic for:
    - Tracking previous values of metrics
    - Calculating trend directions (up/down/stable)
    - Determining visual attributes (colors) for trends
    - Managing enable/disable state

    Example:
        tracker = TrendTracker(colors_dict)
        tracker.update("func1", "nsamples", 10)
        trend = tracker.get_trend("func1", "nsamples")
        color = tracker.get_color("func1", "nsamples")
    """

    # Threshold for determining if a value has changed significantly
    CHANGE_THRESHOLD = 0.001

    def __init__(self, colors: Dict[str, int], enabled: bool = True):
        """
        Initialize the trend tracker.

        Args:
            colors: Dictionary containing color attributes including
                   'trend_up', 'trend_down', 'trend_stable'
            enabled: Whether trend tracking is initially enabled
        """
        self._previous_values: Dict[Any, Dict[str, float]] = {}
        self._enabled = enabled
        self._colors = colors

    @property
    def enabled(self) -> bool:
        """Whether trend tracking is enabled."""
        return self._enabled

    def toggle(self) -> bool:
        """
        Toggle trend tracking on/off.

        Returns:
            New enabled state
        """
        self._enabled = not self._enabled
        return self._enabled

    def set_enabled(self, enabled: bool) -> None:
        """Set trend tracking enabled state."""
        self._enabled = enabled

    def update(self, key: Any, metric: str, value: float) -> TrendDirection:
        """
        Update a metric value and calculate its trend.

        Args:
            key: Identifier for the entity (e.g., function)
            metric: Name of the metric (e.g., 'nsamples', 'tottime')
            value: Current value of the metric

        Returns:
            Trend direction: 'up', 'down', or 'stable'
        """
        # Initialize storage for this key if needed
        if key not in self._previous_values:
            self._previous_values[key] = {}

        # Get previous value, defaulting to current if not tracked yet
        prev_value = self._previous_values[key].get(metric, value)

        # Calculate trend
        if value > prev_value + self.CHANGE_THRESHOLD:
            trend = "up"
        elif value < prev_value - self.CHANGE_THRESHOLD:
            trend = "down"
        else:
            trend = "stable"

        # Update previous value for next iteration
        self._previous_values[key][metric] = value

        return trend

    def get_trend(self, key: Any, metric: str) -> TrendDirection:
        """
        Get the current trend for a metric without updating.

        Args:
            key: Identifier for the entity
            metric: Name of the metric

        Returns:
            Trend direction, or 'stable' if not tracked
        """
        # This would require storing trends separately, which we don't do
        # For now, return stable if not found
        return "stable"

    def get_color(self, trend: TrendDirection) -> int:
        """
        Get the color attribute for a trend direction.

        Args:
            trend: The trend direction

        Returns:
            Curses color attribute (or A_NORMAL if disabled)
        """
        if not self._enabled:
            return curses.A_NORMAL

        if trend == "up":
            return self._colors.get("trend_up", curses.A_BOLD)
        elif trend == "down":
            return self._colors.get("trend_down", curses.A_BOLD)
        else:  # stable
            return self._colors.get("trend_stable", curses.A_NORMAL)

    def update_metrics(self, key: Any, metrics: Dict[str, float]) -> Dict[str, TrendDirection]:
        """
        Update multiple metrics at once and get their trends.

        Args:
            key: Identifier for the entity
            metrics: Dictionary of metric_name -> value

        Returns:
            Dictionary of metric_name -> trend_direction
        """
        trends = {}
        for metric, value in metrics.items():
            trends[metric] = self.update(key, metric, value)
        return trends

    def clear(self) -> None:
        """Clear all tracked values (useful on stats reset)."""
        self._previous_values.clear()

    def __repr__(self) -> str:
        """String representation for debugging."""
        status = "enabled" if self._enabled else "disabled"
        tracked = len(self._previous_values)
        return f"TrendTracker({status}, tracking {tracked} entities)"
