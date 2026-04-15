"""Telemetry plugin registry and dispatch."""

from __future__ import annotations

import importlib
import pkgutil

from . import plugins as plugins_pkg


_PLUGIN_MODULES = None


def _discover_plugin_modules():
    modules = {}
    for module_info in pkgutil.iter_modules(plugins_pkg.__path__):
        name = module_info.name
        if name.startswith("_"):
            continue
        module = importlib.import_module(f"{plugins_pkg.__name__}.{name}")
        plugin_id = getattr(module, "PLUGIN_ID", None)
        if plugin_id:
            modules[plugin_id] = module
    return modules


def _get_plugin_modules():
    global _PLUGIN_MODULES
    if _PLUGIN_MODULES is None:
        _PLUGIN_MODULES = _discover_plugin_modules()
    return _PLUGIN_MODULES


def _get_plugin_module(plugin_id):
    return _get_plugin_modules().get(plugin_id)


def create_live_plugin(plugin_id):
    module = _get_plugin_module(plugin_id)
    if module is None:
        return None
    factory = getattr(module, "create_live_plugin", None)
    if callable(factory):
        return factory()
    return None


def resolve_helper_config(plugin_id, config=None):
    module = _get_plugin_module(plugin_id)
    if module is None:
        return dict(config or {})
    resolver = getattr(module, "resolve_helper_config", None)
    if callable(resolver):
        return resolver(config)
    return dict(config or {})


def run_helper_plugin(plugin_id, config, emit):
    module = _get_plugin_module(plugin_id)
    if module is None:
        emit(plugin_id, "metadata", {"note": "unknown plugin"})
        return
    runner = getattr(module, "run_helper", None)
    if callable(runner):
        runner(config, emit)
        return
    emit(plugin_id, "metadata", {"note": "plugin missing run_helper"})
