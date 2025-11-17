const EMBEDDED_DATA = {{FLAMEGRAPH_DATA}};

// Global string table for resolving string indices
let stringTable = [];
let originalData = null;
let currentThreadFilter = 'all';

// Function to resolve string indices to actual strings
function resolveString(index) {
    if (typeof index === 'number' && index >= 0 && index < stringTable.length) {
        return stringTable[index];
    }
    // Fallback for non-indexed strings or invalid indices
    return String(index);
}

// Function to recursively resolve all string indices in flamegraph data
function resolveStringIndices(node) {
    if (!node) return node;

    // Create a copy to avoid mutating the original
    const resolved = { ...node };

    // Resolve string fields
    if (typeof resolved.name === 'number') {
        resolved.name = resolveString(resolved.name);
    }
    if (typeof resolved.filename === 'number') {
        resolved.filename = resolveString(resolved.filename);
    }
    if (typeof resolved.funcname === 'number') {
        resolved.funcname = resolveString(resolved.funcname);
    }

    // Resolve source lines if present
    if (Array.isArray(resolved.source)) {
        resolved.source = resolved.source.map(index =>
            typeof index === 'number' ? resolveString(index) : index
        );
    }

    // Recursively resolve children
    if (Array.isArray(resolved.children)) {
        resolved.children = resolved.children.map(child => resolveStringIndices(child));
    }

    return resolved;
}

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

function ensureLibraryLoaded() {
  if (typeof flamegraph === "undefined") {
    console.error("d3-flame-graph library not loaded");
    document.getElementById("chart").innerHTML =
      '<h2 style="text-align: center; color: #d32f2f;">Error: d3-flame-graph library failed to load</h2>';
    throw new Error("d3-flame-graph library failed to load");
  }
}

function createPythonTooltip(data) {
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
        .style("word-wrap", "break-word")
        .style("overflow-wrap", "break-word")
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
            `<div style="font-family: 'SF Mono', 'Monaco', 'Consolas', ` +
            `monospace; font-size: 12px; color: ${
              line.startsWith("→") ? "#3776ab" : "#5a6c7d"
            }; white-space: pre-wrap; word-break: break-all; overflow-wrap: break-word; line-height: 1.4; padding: 2px 0;">${line
              .replace(/&/g, "&amp;")
              .replace(/</g, "&lt;")
              .replace(/>/g, "&gt;")}</div>`,
        )
        .join("");

      sourceSection = `
        <div style="margin-top: 16px; padding-top: 12px;
                    border-top: 1px solid #e9ecef;">
          <div style="color: #3776ab; font-size: 13px;
                      margin-bottom: 8px; font-weight: 600;">
            Source Code:
          </div>
          <div style="background: #f8f9fa; border: 1px solid #e9ecef;
                      border-radius: 6px; padding: 12px; max-height: 150px;
                      overflow-y: auto; overflow-x: hidden;">
            ${sourceLines}
          </div>
        </div>`;
    } else if (source) {
      // Show debug info if source exists but isn't an array
      sourceSection = `
        <div style="margin-top: 16px; padding-top: 12px;
                    border-top: 1px solid #e9ecef;">
          <div style="color: #d32f2f; font-size: 13px;
                      margin-bottom: 8px; font-weight: 600;">
            [Debug] - Source data type: ${typeof source}
          </div>
          <div style="background: #f8f9fa; border: 1px solid #e9ecef;
                      border-radius: 6px; padding: 12px; max-height: 150px;
                      overflow-y: auto; overflow-x: hidden; font-family: monospace; font-size: 11px; word-break: break-all; overflow-wrap: break-word;">
            ${JSON.stringify(source, null, 2)}
          </div>
        </div>`;
    }

    // Resolve strings for display
    const funcname = resolveString(d.data.funcname) || resolveString(d.data.name);
    const filename = resolveString(d.data.filename) || "";

    // Don't show file location for special frames like <GC> and <native>
    const isSpecialFrame = filename === "~";
    const fileLocationHTML = isSpecialFrame ? "" : `
        <div style="color: #5a6c7d; font-size: 13px; margin-bottom: 12px;
                    font-family: monospace; background: #f8f9fa;
                    padding: 4px 8px; border-radius: 4px; word-break: break-all; overflow-wrap: break-word;">
          ${filename}${d.data.lineno ? ":" + d.data.lineno : ""}
        </div>`;

    const tooltipHTML = `
      <div>
        <div style="color: #3776ab; font-weight: 600; font-size: 16px;
                    margin-bottom: 8px; line-height: 1.3; word-break: break-word; overflow-wrap: break-word;">
          ${funcname}
        </div>
        ${fileLocationHTML}
        <div style="display: grid; grid-template-columns: auto 1fr;
                    gap: 8px 16px; font-size: 14px;">
          <span style="color: #5a6c7d; font-weight: 500;">Execution Time:</span>
          <strong style="color: #2e3338;">${timeMs} ms</strong>

          <span style="color: #5a6c7d; font-weight: 500;">Percentage:</span>
          <strong style="color: #3776ab;">${percentage}%</strong>

          ${calls > 0 ? `
            <span style="color: #5a6c7d; font-weight: 500;">Function Calls:</span>
            <strong style="color: #2e3338;">${calls.toLocaleString()}</strong>
          ` : ''}

          ${childCount > 0 ? `
            <span style="color: #5a6c7d; font-weight: 500;">Child Functions:</span>
            <strong style="color: #2e3338;">${childCount}</strong>
          ` : ''}
        </div>
        ${sourceSection}
        <div style="margin-top: 16px; padding-top: 12px;
                    border-top: 1px solid #e9ecef; font-size: 13px;
                    color: #5a6c7d; text-align: center;">
          ${childCount > 0 ?
            "Click to focus on this function" :
            "Leaf function - no children"}
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
  return pythonTooltip;
}

function createFlamegraph(tooltip, rootValue) {
  let chart = flamegraph()
    .width(window.innerWidth - 80)
    .cellHeight(20)
    .transitionDuration(300)
    .minFrameSize(1)
    .tooltip(tooltip)
    .inverted(true)
    .setColorMapper(function (d) {
      const percentage = d.data.value / rootValue;
      let colorIndex;
      if (percentage >= 0.6) colorIndex = 7;
      else if (percentage >= 0.35) colorIndex = 6;
      else if (percentage >= 0.18) colorIndex = 5;
      else if (percentage >= 0.12) colorIndex = 4;
      else if (percentage >= 0.06) colorIndex = 3;
      else if (percentage >= 0.03) colorIndex = 2;
      else if (percentage >= 0.01) colorIndex = 1;
      else colorIndex = 0; // <1%
      return pythonColors[colorIndex];
    });
  return chart;
}

function renderFlamegraph(chart, data) {
  d3.select("#chart").datum(data).call(chart);
  window.flamegraphChart = chart; // for controls
  window.flamegraphData = data;   // for resize/search
  populateStats(data);
}

function attachPanelControls() {
  const infoBtn = document.getElementById("show-info-btn");
  const infoPanel = document.getElementById("info-panel");
  const closeBtn = document.getElementById("close-info-btn");
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
}

function updateSearchHighlight(searchTerm, searchInput) {
  d3.selectAll("#chart rect")
    .style("stroke", null)
    .style("stroke-width", null)
    .style("opacity", null);
  if (searchTerm && searchTerm.length > 0) {
    d3.selectAll("#chart rect").style("opacity", 0.3);
    let matchCount = 0;
    d3.selectAll("#chart rect").each(function (d) {
      if (d && d.data) {
        const name = resolveString(d.data.name) || "";
        const funcname = resolveString(d.data.funcname) || "";
        const filename = resolveString(d.data.filename) || "";
        const term = searchTerm.toLowerCase();
        const matches =
          name.toLowerCase().includes(term) ||
          funcname.toLowerCase().includes(term) ||
          filename.toLowerCase().includes(term);
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
    if (searchInput) {
      if (matchCount > 0) {
        searchInput.style.borderColor = "rgba(40, 167, 69, 0.8)";
        searchInput.style.boxShadow = "0 6px 20px rgba(40, 167, 69, 0.2)";
      } else {
        searchInput.style.borderColor = "rgba(220, 53, 69, 0.8)";
        searchInput.style.boxShadow = "0 6px 20px rgba(220, 53, 69, 0.2)";
      }
    }
  } else if (searchInput) {
    searchInput.style.borderColor = "rgba(255, 255, 255, 0.2)";
    searchInput.style.boxShadow = "0 4px 12px rgba(0, 0, 0, 0.1)";
  }
}

function initSearchHandlers() {
  const searchInput = document.getElementById("search-input");
  if (!searchInput) return;
  let searchTimeout;
  function performSearch() {
    const term = searchInput.value.trim();
    updateSearchHighlight(term, searchInput);
  }
  searchInput.addEventListener("input", function () {
    clearTimeout(searchTimeout);
    searchTimeout = setTimeout(performSearch, 150);
  });
  window.performSearch = performSearch;
}

function handleResize(chart, data) {
  window.addEventListener("resize", function () {
    if (chart && data) {
      const newWidth = window.innerWidth - 80;
      chart.width(newWidth);
      d3.select("#chart").datum(data).call(chart);
    }
  });
}

function initFlamegraph() {
  ensureLibraryLoaded();

  // Extract string table if present and resolve string indices
  let processedData = EMBEDDED_DATA;
  if (EMBEDDED_DATA.strings) {
    stringTable = EMBEDDED_DATA.strings;
    processedData = resolveStringIndices(EMBEDDED_DATA);
  }

  // Store original data for filtering
  originalData = processedData;

  // Initialize thread filter dropdown
  initThreadFilter(processedData);

  const tooltip = createPythonTooltip(processedData);
  const chart = createFlamegraph(tooltip, processedData.value);
  renderFlamegraph(chart, processedData);
  attachPanelControls();
  initSearchHandlers();
  handleResize(chart, processedData);
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", initFlamegraph);
} else {
  initFlamegraph();
}

function populateStats(data) {
  const totalSamples = data.value || 0;

  // Collect all functions with their metrics, aggregated by function name
  const functionMap = new Map();

  function collectFunctions(node) {
    if (!node) return;

    let filename = typeof node.filename === 'number' ? resolveString(node.filename) : node.filename;
    let funcname = typeof node.funcname === 'number' ? resolveString(node.funcname) : node.funcname;

    if (!filename || !funcname) {
      const nameStr = typeof node.name === 'number' ? resolveString(node.name) : node.name;
      if (nameStr?.includes('(')) {
        const match = nameStr.match(/^(.+?)\s*\((.+?):(\d+)\)$/);
        if (match) {
          funcname = funcname || match[1];
          filename = filename || match[2];
        }
      }
    }

    filename = filename || 'unknown';
    funcname = funcname || 'unknown';

    if (filename !== 'unknown' && funcname !== 'unknown' && node.value > 0) {
      // Calculate direct samples (this node's value minus children's values)
      let childrenValue = 0;
      if (node.children) {
        childrenValue = node.children.reduce((sum, child) => sum + child.value, 0);
      }
      const directSamples = Math.max(0, node.value - childrenValue);

      // Use file:line:funcname as key to ensure uniqueness
      const funcKey = `${filename}:${node.lineno || '?'}:${funcname}`;

      if (functionMap.has(funcKey)) {
        const existing = functionMap.get(funcKey);
        existing.directSamples += directSamples;
        existing.directPercent = (existing.directSamples / totalSamples) * 100;
        // Keep the most representative file/line (the one with more samples)
        if (directSamples > existing.maxSingleSamples) {
          existing.filename = filename;
          existing.lineno = node.lineno || '?';
          existing.maxSingleSamples = directSamples;
        }
      } else {
        functionMap.set(funcKey, {
          filename: filename,
          lineno: node.lineno || '?',
          funcname: funcname,
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
    if (i < hotSpots.length && hotSpots[i]) {
      const hotspot = hotSpots[i];
      const filename = hotspot.filename || 'unknown';
      const lineno = hotspot.lineno ?? '?';
      let funcDisplay = hotspot.funcname || 'unknown';
      if (funcDisplay.length > 35) {
        funcDisplay = funcDisplay.substring(0, 32) + '...';
      }

      // Don't show file:line for special frames like <GC> and <native>
      const isSpecialFrame = filename === '~' && (lineno === 0 || lineno === '?');
      let fileDisplay;
      if (isSpecialFrame) {
        fileDisplay = '--';
      } else {
        const basename = filename !== 'unknown' ? filename.split('/').pop() : 'unknown';
        fileDisplay = `${basename}:${lineno}`;
      }

      document.getElementById(`hotspot-file-${num}`).textContent = fileDisplay;
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

function initThreadFilter(data) {
  const threadFilter = document.getElementById('thread-filter');
  const threadWrapper = document.querySelector('.thread-filter-wrapper');

  if (!threadFilter || !data.threads) {
    return;
  }

  // Clear existing options except "All Threads"
  threadFilter.innerHTML = '<option value="all">All Threads</option>';

  // Add thread options
  const threads = data.threads || [];
  threads.forEach(threadId => {
    const option = document.createElement('option');
    option.value = threadId;
    option.textContent = `Thread ${threadId}`;
    threadFilter.appendChild(option);
  });

  // Show filter if more than one thread
  if (threads.length > 1 && threadWrapper) {
    threadWrapper.style.display = 'inline-flex';
  }
}

function filterByThread() {
  const threadFilter = document.getElementById('thread-filter');
  if (!threadFilter || !originalData) return;

  const selectedThread = threadFilter.value;
  currentThreadFilter = selectedThread;

  let filteredData;
  if (selectedThread === 'all') {
    // Show all data
    filteredData = originalData;
  } else {
    // Filter data by thread
    const threadId = parseInt(selectedThread);
    filteredData = filterDataByThread(originalData, threadId);

    if (filteredData.strings) {
      stringTable = filteredData.strings;
      filteredData = resolveStringIndices(filteredData);
    }
  }

  // Re-render flamegraph with filtered data
  const tooltip = createPythonTooltip(filteredData);
  const chart = createFlamegraph(tooltip, filteredData.value);
  renderFlamegraph(chart, filteredData);
}

function filterDataByThread(data, threadId) {
  function filterNode(node) {
    if (!node.threads || !node.threads.includes(threadId)) {
      return null;
    }

    const filteredNode = {
      ...node,
      children: []
    };

    if (node.children && Array.isArray(node.children)) {
      filteredNode.children = node.children
        .map(child => filterNode(child))
        .filter(child => child !== null);
    }

    return filteredNode;
  }

  const filteredRoot = {
    ...data,
    children: []
  };

  if (data.children && Array.isArray(data.children)) {
    filteredRoot.children = data.children
      .map(child => filterNode(child))
      .filter(child => child !== null);
  }

  function recalculateValue(node) {
    if (!node.children || node.children.length === 0) {
      return node.value || 0;
    }
    const childrenValue = node.children.reduce((sum, child) => sum + recalculateValue(child), 0);
    node.value = Math.max(node.value || 0, childrenValue);
    return node.value;
  }

  recalculateValue(filteredRoot);

  return filteredRoot;
}

