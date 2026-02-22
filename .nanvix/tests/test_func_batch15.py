"""Batch 15: plotly (very heavy import, runs alone)"""
import sys, json
results = []

try:
    import plotly.graph_objects as go
    fig = go.Figure(data=go.Bar(x=["a","b","c"], y=[1,2,3]))
    d = json.loads(fig.to_json())
    assert d["data"][0]["type"] == "bar"
    results.append(("plotly", "PASS"))
except Exception as e: results.append(("plotly", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
