from .isolated import build
from .legacy import make_sdist


def build_package(config, session):
    if not config.isolated_build:
        return make_sdist(config, session)
    else:
        return build(config, session)
