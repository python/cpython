"""Test: plotly"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import plotly.graph_objects as go
    fig = go.Figure(data=[go.Scatter(x=[1, 2, 3], y=[4, 5, 6])])
    html = fig.to_html(include_plotlyjs=False)
    assert "scatter" in html.lower()
    print("plotly: PASS")
except Exception as e:
    print(f"plotly: FAIL: {e}")
    sys.exit(1)
