"""
Provides .props file.
"""

import os

from .constants import *

__all__ = ["get_props_layout"]

PYTHON_PROPS_NAME = "python.props"

PROPS_DATA = {
    "PYTHON_TAG": VER_DOT,
    "PYTHON_VERSION": os.getenv("PYTHON_NUSPEC_VERSION"),
    "PYTHON_PLATFORM": os.getenv("PYTHON_PROPS_PLATFORM"),
    "PYTHON_TARGET": "",
}

if not PROPS_DATA["PYTHON_VERSION"]:
    PROPS_DATA["PYTHON_VERSION"] = "{}.{}{}{}".format(
        VER_DOT, VER_MICRO, "-" if VER_SUFFIX else "", VER_SUFFIX
    )

PROPS_DATA["PYTHON_TARGET"] = "_GetPythonRuntimeFilesDependsOn{}{}_{}".format(
    VER_MAJOR, VER_MINOR, PROPS_DATA["PYTHON_PLATFORM"]
)

PROPS_TEMPLATE = r"""<?xml version="1.0" encoding="utf-8"?>
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="$(Platform) == '{PYTHON_PLATFORM}'">
    <PythonHome Condition="$(PythonHome) == ''">$([System.IO.Path]::GetFullPath("$(MSBuildThisFileDirectory)\..\..\tools"))</PythonHome>
    <PythonInclude>$(PythonHome)\include</PythonInclude>
    <PythonLibs>$(PythonHome)\libs</PythonLibs>
    <PythonTag>{PYTHON_TAG}</PythonTag>
    <PythonVersion>{PYTHON_VERSION}</PythonVersion>

    <IncludePythonExe Condition="$(IncludePythonExe) == ''">true</IncludePythonExe>
    <IncludeVEnv Condition="$(IncludeVEnv) == ''">false</IncludeVEnv>

    <GetPythonRuntimeFilesDependsOn>{PYTHON_TARGET};$(GetPythonRuntimeFilesDependsOn)</GetPythonRuntimeFilesDependsOn>
  </PropertyGroup>

  <ItemDefinitionGroup Condition="$(Platform) == '{PYTHON_PLATFORM}'">
    <ClCompile>
      <AdditionalIncludeDirectories>$(PythonInclude);%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
    </ClCompile>
    <Link>
      <AdditionalLibraryDirectories>$(PythonLibs);%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
    </Link>
  </ItemDefinitionGroup>

  <Target Name="GetPythonRuntimeFiles" Returns="@(PythonRuntime)" DependsOnTargets="$(GetPythonRuntimeFilesDependsOn)" />

  <Target Name="{PYTHON_TARGET}" Returns="@(PythonRuntime)">
    <ItemGroup>
      <_PythonRuntimeExe Include="$(PythonHome)\python*.dll" />
      <_PythonRuntimeExe Include="$(PythonHome)\python*.exe" Condition="$(IncludePythonExe) == 'true'" />
      <_PythonRuntimeExe>
        <Link>%(Filename)%(Extension)</Link>
      </_PythonRuntimeExe>
      <_PythonRuntimeDlls Include="$(PythonHome)\DLLs\*.pyd" />
      <_PythonRuntimeDlls Include="$(PythonHome)\DLLs\*.dll" />
      <_PythonRuntimeDlls>
        <Link>DLLs\%(Filename)%(Extension)</Link>
      </_PythonRuntimeDlls>
      <_PythonRuntimeLib Include="$(PythonHome)\Lib\**\*" Exclude="$(PythonHome)\Lib\**\*.pyc;$(PythonHome)\Lib\site-packages\**\*" />
      <_PythonRuntimeLib Remove="$(PythonHome)\Lib\ensurepip\**\*" Condition="$(IncludeVEnv) != 'true'" />
      <_PythonRuntimeLib Remove="$(PythonHome)\Lib\venv\**\*" Condition="$(IncludeVEnv) != 'true'" />
      <_PythonRuntimeLib>
        <Link>Lib\%(RecursiveDir)%(Filename)%(Extension)</Link>
      </_PythonRuntimeLib>
      <PythonRuntime Include="@(_PythonRuntimeExe);@(_PythonRuntimeDlls);@(_PythonRuntimeLib)" />
    </ItemGroup>

    <Message Importance="low" Text="Collected Python runtime from $(PythonHome):%0D%0A@(PythonRuntime->'  %(Link)','%0D%0A')" />
  </Target>
</Project>
"""


def get_props_layout(ns):
    if ns.include_all or ns.include_props:
        # TODO: Filter contents of props file according to included/excluded items
        d = dict(PROPS_DATA)
        if not d.get("PYTHON_PLATFORM"):
            d["PYTHON_PLATFORM"] = {
                "win32": "Win32",
                "amd64": "X64",
                "arm32": "ARM",
                "arm64": "ARM64",
            }[ns.arch]
        props = PROPS_TEMPLATE.format_map(d)
        yield "python.props", ("python.props", props.encode("utf-8"))
