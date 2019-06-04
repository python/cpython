"""
Provides .props file.
"""

import os

from .constants import *

__all__ = ["PYTHON_PROPS_NAME"]


def public(f):
    __all__.append(f.__name__)
    return f


PYTHON_PROPS_NAME = "python.props"

PROPS_DATA = {
    "PYTHON_TAG": VER_DOT,
    "PYTHON_VERSION": os.getenv("PYTHON_NUSPEC_VERSION"),
    "PYTHON_PLATFORM": os.getenv("PYTHON_PROPS_PLATFORM"),
    "PYTHON_TARGET": "",
}

if not PROPS_DATA["PYTHON_VERSION"]:
    if VER_NAME:
        PROPS_DATA["PYTHON_VERSION"] = "{}.{}-{}{}".format(
            VER_DOT, VER_MICRO, VER_NAME, VER_SERIAL
        )
    else:
        PROPS_DATA["PYTHON_VERSION"] = "{}.{}".format(VER_DOT, VER_MICRO)

if not PROPS_DATA["PYTHON_PLATFORM"]:
    PROPS_DATA["PYTHON_PLATFORM"] = "x64" if IS_X64 else "Win32"

PROPS_DATA["PYTHON_TARGET"] = "_GetPythonRuntimeFilesDependsOn{}{}_{}".format(
    VER_MAJOR, VER_MINOR, PROPS_DATA["PYTHON_PLATFORM"]
)

PROPS_TEMPLATE = r"""<?xml version="1.0" encoding="utf-8"?>
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="$(Platform) == '{PYTHON_PLATFORM}'">
    <PythonHome Condition="$(Configuration) == 'Debug'">$([msbuild]::GetDirectoryNameOfFileAbove($(MSBuildThisFileDirectory), "python_d.exe")</PythonHome>
    <PythonHome Condition="$(PythonHome) == ''">$([msbuild]::GetDirectoryNameOfFileAbove($(MSBuildThisFileDirectory), "python.exe")</PythonHome>
    <PythonInclude>$(PythonHome)\include</PythonInclude>
    <PythonLibs>$(PythonHome)\libs</PythonLibs>
    <PythonTag>{PYTHON_TAG}</PythonTag>
    <PythonVersion>{PYTHON_VERSION}</PythonVersion>

    <IncludePythonExe Condition="$(IncludePythonExe) == ''">true</IncludePythonExe>
    <IncludeDistutils Condition="$(IncludeDistutils) == ''">false</IncludeDistutils>
    <IncludeLib2To3 Condition="$(IncludeLib2To3) == ''">false</IncludeLib2To3>
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
      <_PythonRuntimeLib Remove="$(PythonHome)\Lib\distutils\**\*" Condition="$(IncludeDistutils) != 'true'" />
      <_PythonRuntimeLib Remove="$(PythonHome)\Lib\lib2to3\**\*" Condition="$(IncludeLib2To3) != 'true'" />
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


@public
def get_props_layout(ns):
    if ns.include_all or ns.include_props:
        yield "python.props", ns.temp / "python.props"


@public
def get_props(ns):
    # TODO: Filter contents of props file according to included/excluded items
    props = PROPS_TEMPLATE.format_map(PROPS_DATA)
    return props.encode("utf-8")
