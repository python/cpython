#!/usr/bin/env python3.8

import argparse
import os
import json

from typing import Dict, Any
from urllib.request import urlretrieve

argparser = argparse.ArgumentParser(
    prog="download_pypi_packages", description="Helper program to download PyPI packages",
)
argparser.add_argument(
    "-n", "--number", type=int, default=100, help="Number of packages to download"
)
argparser.add_argument(
    "-a", "--all", action="store_true", help="Download all packages listed in the json file"
)


def load_json(filename: str) -> Dict[Any, Any]:
    with open(os.path.join("data", f"{filename}.json"), "r") as f:
        j = json.loads(f.read())
    return j


def remove_json(filename: str) -> None:
    path = os.path.join("data", f"{filename}.json")
    os.remove(path)


def download_package_json(package_name: str) -> None:
    url = f"https://pypi.org/pypi/{package_name}/json"
    urlretrieve(url, os.path.join("data", f"{package_name}.json"))


def download_package_code(name: str, package_json: Dict[Any, Any]) -> None:
    source_index = -1
    for idx, url_info in enumerate(package_json["urls"]):
        if url_info["python_version"] == "source":
            source_index = idx
            break
    filename = package_json["urls"][source_index]["filename"]
    url = package_json["urls"][source_index]["url"]
    urlretrieve(url, os.path.join("data", "pypi", filename))


def main() -> None:
    args = argparser.parse_args()
    number_packages = args.number
    all_packages = args.all

    top_pypi_packages = load_json("top-pypi-packages-365-days")
    if all_packages:
        top_pypi_packages = top_pypi_packages["rows"]
    elif number_packages >= 0 and number_packages <= 4000:
        top_pypi_packages = top_pypi_packages["rows"][:number_packages]
    else:
        raise AssertionError("Unknown value for NUMBER_OF_PACKAGES")

    try:
        os.mkdir(os.path.join("data", "pypi"))
    except FileExistsError:
        pass

    for package in top_pypi_packages:
        package_name = package["project"]

        print(f"Downloading JSON Data for {package_name}... ", end="")
        download_package_json(package_name)
        print("Done")

        package_json = load_json(package_name)
        try:
            print(f"Downloading and compressing package {package_name} ... ", end="")
            download_package_code(package_name, package_json)
            print("Done")
        except (IndexError, KeyError):
            print(f"Could not locate source for {package_name}")
            continue
        finally:
            remove_json(package_name)


if __name__ == "__main__":
    main()
