function main() {
  const data = {{FLAMEGRAPH_DATA}}

  if (typeof flamegraph === "undefined") {
    console.error("d3-flame-graph library not loaded");
    document.getElementById("chart").innerHTML =
      '<h2 style="text-align: center; color: #d32f2f;">Error: d3-flame-graph library failed to load</h2>';
    throw new Error("d3-flame-graph library failed to load");
  }

  const pythonTooltip = flamegraph.tooltip.defaultFlamegraphTooltip();

  pythonTooltip.show = function (d, element) {
    if (!this._tooltip) {
      this._tooltip = d3
        .select("body")
        .append("div")
        .attr("class", "python-tooltip")
        .style("position", "absolute")
        .style("padding", "20px")
        .style("background", "white")
        .style("color", "#2e3338")
        .style("border-radius", "8px")
        .style("font-size", "14px")
        .style("border", "1px solid #e9ecef")
        .style("box-shadow", "0 8px 30px rgba(0, 0, 0, 0.15)")
        .style("z-index", "1000")
        .style("pointer-events", "none")
        .style("font-weight", "400")
        .style("line-height", "1.5")
        .style("max-width", "500px")
        .style("font-family", "'Source Sans Pro', sans-serif")
        .style("opacity", 0);
    }

    const timeMs = (d.data.value / 1000).toFixed(2);
    const percentage = ((d.data.value / data.value) * 100).toFixed(2);
    const calls = d.data.calls || 0;
    const childCount = d.children ? d.children.length : 0;
    const source = d.data.source;

    // Create source code section if available
    let sourceSection = "";
    if (source && Array.isArray(source) && source.length > 0) {
      const sourceLines = source
        .map(
          (line) =>
            `<div style="font-family: 'SF Mono', 'Monaco', 'Consolas', monospace; font-size: 12px; color: ${line.startsWith("→") ? "#3776ab" : "#5a6c7d"}; white-space: pre; line-height: 1.4; padding: 2px 0;">${line.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")}</div>`,
        )
        .join("");

      sourceSection = `
                    <div style="margin-top: 16px; padding-top: 12px; border-top: 1px solid #e9ecef;">
                        <div style="color: #3776ab; font-size: 13px; margin-bottom: 8px; font-weight: 600;">📄 Source Code:</div>
                        <div style="background: #f8f9fa; border: 1px solid #e9ecef; border-radius: 6px; padding: 12px; max-height: 150px; overflow-y: auto;">
                            ${sourceLines}
                        </div>
                    </div>
                `;
    } else if (source) {
      // Show debug info if source exists but isn't an array
      sourceSection = `
                    <div style="margin-top: 16px; padding-top: 12px; border-top: 1px solid #e9ecef;">
                        <div style="color: #d32f2f; font-size: 13px; margin-bottom: 8px; font-weight: 600;">🐛 Debug - Source data type: ${typeof source}</div>
                        <div style="background: #f8f9fa; border: 1px solid #e9ecef; border-radius: 6px; padding: 12px; max-height: 150px; overflow-y: auto; font-family: monospace; font-size: 11px;">
                            ${JSON.stringify(source, null, 2)}
                        </div>
                    </div>
                `;
    }

    const tooltipHTML = `
                <div>
                    <div style="color: #3776ab; font-weight: 600; font-size: 16px; margin-bottom: 8px; line-height: 1.3;">
                        ${d.data.funcname || d.data.name}
                    </div>
                    <div style="color: #5a6c7d; font-size: 13px; margin-bottom: 12px; font-family: monospace; background: #f8f9fa; padding: 4px 8px; border-radius: 4px;">
                        ${d.data.filename || ""}${d.data.lineno ? ":" + d.data.lineno : ""}
                    </div>
                    <div style="display: grid; grid-template-columns: auto 1fr; gap: 8px 16px; font-size: 14px;">
                        <span style="color: #5a6c7d; font-weight: 500;">Execution Time:</span>
                        <strong style="color: #2e3338;">${timeMs} ms</strong>

                        <span style="color: #5a6c7d; font-weight: 500;">Percentage:</span>
                        <strong style="color: #3776ab;">${percentage}%</strong>

                        ${
                          calls > 0
                            ? `
                            <span style="color: #5a6c7d; font-weight: 500;">Function Calls:</span>
                            <strong style="color: #2e3338;">${calls.toLocaleString()}</strong>
                        `
                            : ""
                        }

                        ${
                          childCount > 0
                            ? `
                            <span style="color: #5a6c7d; font-weight: 500;">Child Functions:</span>
                            <strong style="color: #2e3338;">${childCount}</strong>
                        `
                            : ""
                        }
                    </div>
                    ${sourceSection}
                    <div style="margin-top: 16px; padding-top: 12px; border-top: 1px solid #e9ecef; font-size: 13px; color: #5a6c7d; text-align: center;">
                        ${childCount > 0 ? "👆 Click to focus on this function" : "📄 Leaf function - no children"}
                    </div>
                </div>
            `;

    // Get mouse position
    const event = d3.event || window.event;
    const mouseX = event.pageX || event.clientX;
    const mouseY = event.pageY || event.clientY;

    // Calculate tooltip dimensions (default to 320px width if not rendered yet)
    let tooltipWidth = 320;
    let tooltipHeight = 200;
    if (this._tooltip && this._tooltip.node()) {
      const node = this._tooltip
        .style("opacity", 0)
        .style("display", "block")
        .node();
      tooltipWidth = node.offsetWidth || 320;
      tooltipHeight = node.offsetHeight || 200;
      this._tooltip.style("display", null);
    }

    // Calculate horizontal position: if overflow, show to the left of cursor
    const padding = 10;
    const rightEdge = mouseX + padding + tooltipWidth;
    const viewportWidth = window.innerWidth;
    let left;
    if (rightEdge > viewportWidth) {
      left = mouseX - tooltipWidth - padding;
      if (left < 0) left = padding; // prevent off left edge
    } else {
      left = mouseX + padding;
    }

    // Calculate vertical position: if overflow, show above cursor
    const bottomEdge = mouseY + padding + tooltipHeight;
    const viewportHeight = window.innerHeight;
    let top;
    if (bottomEdge > viewportHeight) {
      top = mouseY - tooltipHeight - padding;
      if (top < 0) top = padding; // prevent off top edge
    } else {
      top = mouseY + padding;
    }

    this._tooltip
      .html(tooltipHTML)
      .style("left", left + "px")
      .style("top", top + "px")
      .transition()
      .duration(200)
      .style("opacity", 1);
  };

  // Override the hide method
  pythonTooltip.hide = function () {
    if (this._tooltip) {
      this._tooltip.transition().duration(200).style("opacity", 0);
    }
  };

  // Store root value globally for color mapping
  let globalRootValue = data.value;
  // Store data globally for resize handling
  window.flamegraphData = data;

  // Create the flamegraph with proper color mapping
  let chart = flamegraph()
    .width(window.innerWidth - 80)
    .cellHeight(20)
    .transitionDuration(300)
    .minFrameSize(1)
    .tooltip(pythonTooltip)
    .inverted(true)
    .setColorMapper(function (d) {
      // Use the stored global root value
      const percentage = d.data.value / globalRootValue;

      // Realistic thresholds for profiling data based on debug output
      let colorIndex;
      if (percentage >= 0.6)
        colorIndex = 7; // Hottest - ≥60% (like your 100%, 80%)
      else if (percentage >= 0.35)
        colorIndex = 6; // Very hot - 35-60% (like your 50%)
      else if (percentage >= 0.18)
        colorIndex = 5; // Hot - 18-35% (like your 30%, 25%, 20%)
      else if (percentage >= 0.12)
        colorIndex = 4; // Warm - 12-18% (like your 15%)
      else if (percentage >= 0.06)
        colorIndex = 3; // Medium - 6-12%
      else if (percentage >= 0.03)
        colorIndex = 2; // Cool - 3-6% (like your 5%)
      else if (percentage >= 0.01)
        colorIndex = 1; // Cold - 1-3%
      else colorIndex = 0; // Coldest - <1%

      const color = pythonColors[colorIndex];

      return color;
    });

  // Render the flamegraph
  d3.select("#chart").datum(data).call(chart);

  // Make chart globally accessible for controls
  window.flamegraphChart = chart;

  // Populate stats cards
  populateStats(data);
}

// Wait for libraries to load
document.addEventListener("DOMContentLoaded", function () {
  main();

  const infoBtn = document.getElementById("show-info-btn");
  const infoPanel = document.getElementById("info-panel");
  const closeBtn = document.getElementById("close-info-btn");
  const searchInput = document.getElementById("search-input");
  
  if (infoBtn && infoPanel) {
    infoBtn.addEventListener("click", function () {
      const isOpen = infoPanel.style.display === "block";
      infoPanel.style.display = isOpen ? "none" : "block";
    });
  }
  if (closeBtn && infoPanel) {
    closeBtn.addEventListener("click", function () {
      infoPanel.style.display = "none";
    });
  }
  
  // Add search functionality - wait for chart to be ready
  if (searchInput) {
    let searchTimeout;
    
    function performSearch() {
      const searchTerm = searchInput.value.trim();
      
      // Clear previous search highlighting
      d3.selectAll("#chart rect")
        .style("stroke", null)
        .style("stroke-width", null)
        .style("opacity", null);
      
      if (searchTerm && searchTerm.length > 0) {
        // First, dim all rectangles
        d3.selectAll("#chart rect")
          .style("opacity", 0.3);
          
        // Then highlight and restore opacity for matching nodes
        let matchCount = 0;
        d3.selectAll("#chart rect")
          .each(function(d) {
            if (d && d.data) {
              const name = d.data.name || "";
              const funcname = d.data.funcname || "";
              const filename = d.data.filename || "";
              
              const matches = name.toLowerCase().includes(searchTerm.toLowerCase()) ||
                             funcname.toLowerCase().includes(searchTerm.toLowerCase()) ||
                             filename.toLowerCase().includes(searchTerm.toLowerCase());
              
              if (matches) {
                matchCount++;
                d3.select(this)
                  .style("opacity", 1)
                  .style("stroke", "#ff6b35")
                  .style("stroke-width", "2px")
                  .style("stroke-dasharray", "3,3");
              }
            }
          });
          
        // Update search input style based on results
        if (matchCount > 0) {
          searchInput.style.borderColor = "#28a745";
          searchInput.style.boxShadow = "0 0 0 3px rgba(40, 167, 69, 0.1)";
        } else {
          searchInput.style.borderColor = "#dc3545";
          searchInput.style.boxShadow = "0 0 0 3px rgba(220, 53, 69, 0.1)";
        }
      } else {
        // Reset search input style
        searchInput.style.borderColor = "#e9ecef";
        searchInput.style.boxShadow = "0 2px 4px rgba(0, 0, 0, 0.06)";
      }
    }
    
    searchInput.addEventListener("input", function() {
      clearTimeout(searchTimeout);
      searchTimeout = setTimeout(performSearch, 150);
    });
    
    // Make search function globally accessible
    window.performSearch = performSearch;
  }
});

// Python color palette - cold to hot
const pythonColors = [
  "#fff4bf", // Coldest - light yellow (<1%)
  "#ffec9e", // Cold - yellow (1-3%)
  "#ffe47d", // Cool - golden yellow (3-6%)
  "#ffdc5c", // Medium - golden (6-12%)
  "#ffd43b", // Warm - Python gold (12-18%)
  "#5592cc", // Hot - light blue (18-35%)
  "#4584bb", // Very hot - medium blue (35-60%)
  "#3776ab", // Hottest - Python blue (≥60%)
];

function populateStats(data) {
  const totalSamples = data.value || 0;
  
  // Collect all functions with their metrics, aggregated by function name
  const functionMap = new Map();
  
  function collectFunctions(node) {
    if (node.filename && node.funcname) {
      // Calculate direct samples (this node's value minus children's values)
      let childrenValue = 0;
      if (node.children) {
        childrenValue = node.children.reduce((sum, child) => sum + child.value, 0);
      }
      const directSamples = Math.max(0, node.value - childrenValue);
      
      // Use file:line:funcname as key to ensure uniqueness
      const funcKey = `${node.filename}:${node.lineno || '?'}:${node.funcname}`;
      
      if (functionMap.has(funcKey)) {
        const existing = functionMap.get(funcKey);
        existing.directSamples += directSamples;
        existing.directPercent = (existing.directSamples / totalSamples) * 100;
        // Keep the most representative file/line (the one with more samples)
        if (directSamples > existing.maxSingleSamples) {
          existing.filename = node.filename;
          existing.lineno = node.lineno || '?';
          existing.maxSingleSamples = directSamples;
        }
      } else {
        functionMap.set(funcKey, {
          filename: node.filename,
          lineno: node.lineno || '?',
          funcname: node.funcname,
          directSamples,
          directPercent: (directSamples / totalSamples) * 100,
          maxSingleSamples: directSamples
        });
      }
    }
    
    if (node.children) {
      node.children.forEach(child => collectFunctions(child));
    }
  }
  
  collectFunctions(data);
  
  // Convert map to array and get top 3 hotspots
  const hotSpots = Array.from(functionMap.values())
    .filter(f => f.directPercent > 0.5) // At least 0.5% to be significant
    .sort((a, b) => b.directPercent - a.directPercent)
    .slice(0, 3);
    
  // Populate the 3 cards
  for (let i = 0; i < 3; i++) {
    const num = i + 1;
    if (i < hotSpots.length) {
      const hotspot = hotSpots[i];
      const basename = hotspot.filename.split('/').pop();
      let funcDisplay = hotspot.funcname;
      if (funcDisplay.length > 35) {
        funcDisplay = funcDisplay.substring(0, 32) + '...';
      }
      
      document.getElementById(`hotspot-file-${num}`).textContent = `${basename}:${hotspot.lineno}`;
      document.getElementById(`hotspot-func-${num}`).textContent = funcDisplay;
      document.getElementById(`hotspot-detail-${num}`).textContent = `${hotspot.directPercent.toFixed(1)}% samples (${hotspot.directSamples.toLocaleString()})`;
    } else {
      document.getElementById(`hotspot-file-${num}`).textContent = '--';
      document.getElementById(`hotspot-func-${num}`).textContent = '--';
      document.getElementById(`hotspot-detail-${num}`).textContent = '--';
    }
  }
}

// Control functions
function resetZoom() {
  if (window.flamegraphChart) {
    window.flamegraphChart.resetZoom();
  }
}

function exportSVG() {
  const svgElement = document.querySelector("#chart svg");
  if (svgElement) {
    const serializer = new XMLSerializer();
    const svgString = serializer.serializeToString(svgElement);
    const blob = new Blob([svgString], { type: "image/svg+xml" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = "python-performance-flamegraph.svg";
    a.click();
    URL.revokeObjectURL(url);
  }
}

function toggleLegend() {
  const legendPanel = document.getElementById("legend-panel");
  const isHidden =
    legendPanel.style.display === "none" || legendPanel.style.display === "";
  legendPanel.style.display = isHidden ? "block" : "none";
}

function clearSearch() {
  const searchInput = document.getElementById("search-input");
  if (searchInput) {
    searchInput.value = "";
    if (window.flamegraphChart) {
      window.flamegraphChart.clear();
    }
  }
}

// Handle window resize
window.addEventListener("resize", function () {
  if (window.flamegraphChart && window.flamegraphData) {
    const newWidth = window.innerWidth - 80;
    window.flamegraphChart.width(newWidth);
    d3.select("#chart")
      .datum(window.flamegraphData)
      .call(window.flamegraphChart);
  }
});
