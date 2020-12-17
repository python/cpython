import logging
import os

from pip._internal.utils.subprocess import runner_with_spinner_message
from pip._internal.utils.typing import MYPY_CHECK_RUNNING

if MYPY_CHECK_RUNNING:
    from typing import List, Optional
    from pip._vendor.pep517.wrappers import Pep517HookCaller

logger = logging.getLogger(__name__)


def build_wheel_pep517(
    name,  # type: str
    backend,  # type: Pep517HookCaller
    metadata_directory,  # type: str
    build_options,  # type: List[str]
    tempd,  # type: str
):
    # type: (...) -> Optional[str]
    """Build one InstallRequirement using the PEP 517 build process.

    Returns path to wheel if successfully built. Otherwise, returns None.
    """
    assert metadata_directory is not None
    if build_options:
        # PEP 517 does not support --build-options
        logger.error('Cannot build wheel for %s using PEP 517 when '
                     '--build-option is present', name)
        return None
    try:
        logger.debug('Destination directory: %s', tempd)

        runner = runner_with_spinner_message(
            'Building wheel for {} (PEP 517)'.format(name)
        )
        with backend.subprocess_runner(runner):
            wheel_name = backend.build_wheel(
                tempd,
                metadata_directory=metadata_directory,
            )
    except Exception:
        logger.error('Failed building wheel for %s', name)
        return None
    return os.path.join(tempd, wheel_name)
