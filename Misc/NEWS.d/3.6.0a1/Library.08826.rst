Issue #23517: fromtimestamp() and utcfromtimestamp() methods of
datetime.datetime now round microseconds to nearest with ties going to
nearest even integer (ROUND_HALF_EVEN), as round(float), instead of rounding
towards -Infinity (ROUND_FLOOR).