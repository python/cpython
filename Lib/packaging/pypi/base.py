"""Base class for index crawlers."""

from packaging.pypi.dist import ReleasesList


class BaseClient:
    """Base class containing common methods for the index crawlers/clients"""

    def __init__(self, prefer_final, prefer_source):
        self._prefer_final = prefer_final
        self._prefer_source = prefer_source
        self._index = self

    def _get_prefer_final(self, prefer_final=None):
        """Return the prefer_final internal parameter or the specified one if
        provided"""
        if prefer_final:
            return prefer_final
        else:
            return self._prefer_final

    def _get_prefer_source(self, prefer_source=None):
        """Return the prefer_source internal parameter or the specified one if
        provided"""
        if prefer_source:
            return prefer_source
        else:
            return self._prefer_source

    def _get_project(self, project_name):
        """Return an project instance, create it if necessary"""
        return self._projects.setdefault(project_name.lower(),
                    ReleasesList(project_name, index=self._index))

    def download_distribution(self, requirements, temp_path=None,
                              prefer_source=None, prefer_final=None):
        """Download a distribution from the last release according to the
        requirements.

        If temp_path is provided, download to this path, otherwise, create a
        temporary location for the download and return it.
        """
        prefer_final = self._get_prefer_final(prefer_final)
        prefer_source = self._get_prefer_source(prefer_source)
        release = self.get_release(requirements, prefer_final)
        if release:
            dist = release.get_distribution(prefer_source=prefer_source)
            return dist.download(temp_path)
