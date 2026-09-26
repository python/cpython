import importlib.resources


def get_combined_css(component: str) -> str:
    template_dir = importlib.resources.files(__package__)

    base_css = (template_dir / "_shared_assets" / "base.css").read_text(encoding="utf-8")

    if component == "flamegraph":
        component_css = (
            template_dir / "_flamegraph_assets" / "flamegraph.css"
        ).read_text(encoding="utf-8")
    elif component == "heatmap":
        component_css = (template_dir / "_heatmap_assets" / "heatmap.css").read_text(
            encoding="utf-8"
        )
    else:
        raise ValueError(
            f"Unknown component: {component}. Expected 'flamegraph' or 'heatmap'."
        )

    return f"{base_css}\n\n{component_css}"
