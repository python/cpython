_clearers = []

def register(callback):
    _clearers.append(callback)

def clear_caches():
    for clear in _clearers[::-1]:
        clear()
