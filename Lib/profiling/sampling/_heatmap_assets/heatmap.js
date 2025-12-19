// Tachyon Profiler - Heatmap JavaScript
// Interactive features for the heatmap visualization
// Aligned with Flamegraph viewer design patterns

// ============================================================================
// State Management
// ============================================================================

let currentMenu = null;
let colorMode = 'self';  // 'self' or 'cumulative' - default to self
let coldCodeHidden = false;

// ============================================================================
// Theme Support
// ============================================================================

function toggleTheme() {
    const html = document.documentElement;
    const current = html.getAttribute('data-theme') || 'light';
    const next = current === 'light' ? 'dark' : 'light';
    html.setAttribute('data-theme', next);
    localStorage.setItem('heatmap-theme', next);

    // Update theme button icon
    const btn = document.getElementById('theme-btn');
    if (btn) {
        btn.querySelector('.icon-moon').style.display = next === 'dark' ? 'none' : '';
        btn.querySelector('.icon-sun').style.display = next === 'dark' ? '' : 'none';
    }
    applyLineColors();

    // Rebuild scroll marker with new theme colors
    buildScrollMarker();
}

function restoreUIState() {
    // Restore theme
    const savedTheme = localStorage.getItem('heatmap-theme');
    if (savedTheme) {
        document.documentElement.setAttribute('data-theme', savedTheme);
        const btn = document.getElementById('theme-btn');
        if (btn) {
            btn.querySelector('.icon-moon').style.display = savedTheme === 'dark' ? 'none' : '';
            btn.querySelector('.icon-sun').style.display = savedTheme === 'dark' ? '' : 'none';
        }
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

function createElement(tag, className, textContent = '') {
    const el = document.createElement(tag);
    if (className) el.className = className;
    if (textContent) el.textContent = textContent;
    return el;
}

function calculateMenuPosition(buttonRect, menuWidth, menuHeight) {
    const viewport = { width: window.innerWidth, height: window.innerHeight };
    const scroll = {
        x: window.pageXOffset || document.documentElement.scrollLeft,
        y: window.pageYOffset || document.documentElement.scrollTop
    };

    const left = buttonRect.right + menuWidth + 10 < viewport.width
        ? buttonRect.right + scroll.x + 10
        : Math.max(scroll.x + 10, buttonRect.left + scroll.x - menuWidth - 10);

    const top = buttonRect.bottom + menuHeight + 10 < viewport.height
        ? buttonRect.bottom + scroll.y + 5
        : Math.max(scroll.y + 10, buttonRect.top + scroll.y - menuHeight - 10);

    return { left, top };
}

// ============================================================================
// Menu Management
// ============================================================================

function closeMenu() {
    if (currentMenu) {
        currentMenu.remove();
        currentMenu = null;
    }
}

function showNavigationMenu(button, items, title) {
    closeMenu();

    const menu = createElement('div', 'callee-menu');
    menu.appendChild(createElement('div', 'callee-menu-header', title));

    items.forEach(linkData => {
        const item = createElement('div', 'callee-menu-item');

        const funcDiv = createElement('div', 'callee-menu-func');
        funcDiv.textContent = linkData.func;

        if (linkData.count !== undefined && linkData.count > 0) {
            const countBadge = createElement('span', 'count-badge');
            countBadge.textContent = linkData.count.toLocaleString();
            countBadge.title = `${linkData.count.toLocaleString()} samples`;
            funcDiv.appendChild(document.createTextNode(' '));
            funcDiv.appendChild(countBadge);
        }

        item.appendChild(funcDiv);
        item.appendChild(createElement('div', 'callee-menu-file', linkData.file));
        item.addEventListener('click', () => window.location.href = linkData.link);
        menu.appendChild(item);
    });

    const pos = calculateMenuPosition(button.getBoundingClientRect(), 350, 300);
    menu.style.left = `${pos.left}px`;
    menu.style.top = `${pos.top}px`;

    document.body.appendChild(menu);
    currentMenu = menu;
}

// ============================================================================
// Navigation
// ============================================================================

function handleNavigationClick(button, e) {
    e.stopPropagation();

    const navData = button.getAttribute('data-nav');
    if (navData) {
        window.location.href = JSON.parse(navData).link;
        return;
    }

    const navMulti = button.getAttribute('data-nav-multi');
    if (navMulti) {
        const items = JSON.parse(navMulti);
        const title = button.classList.contains('caller') ? 'Choose a caller:' : 'Choose a callee:';
        showNavigationMenu(button, items, title);
    }
}

function scrollToTargetLine() {
    if (!window.location.hash) return;
    const target = document.querySelector(window.location.hash);
    if (target) {
        target.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
}

// ============================================================================
// Sample Count & Intensity
// ============================================================================

function getSampleCount(line) {
    let text;
    if (colorMode === 'self') {
        text = line.querySelector('.line-samples-self')?.textContent.trim().replace(/,/g, '');
    } else {
        text = line.querySelector('.line-samples-cumulative')?.textContent.trim().replace(/,/g, '');
    }
    return parseInt(text) || 0;
}

// ============================================================================
// Scroll Minimap
// ============================================================================

function buildScrollMarker() {
    const existing = document.getElementById('scroll_marker');
    if (existing) existing.remove();

    if (document.body.scrollHeight <= window.innerHeight) return;

    const allLines = document.querySelectorAll('.code-line');
    const lines = Array.from(allLines).filter(line => line.style.display !== 'none');
    const markerScale = window.innerHeight / document.body.scrollHeight;
    const lineHeight = Math.min(Math.max(3, window.innerHeight / lines.length), 10);
    const maxSamples = Math.max(...Array.from(lines, getSampleCount));

    const scrollMarker = createElement('div', '');
    scrollMarker.id = 'scroll_marker';

    let prevLine = -99, lastMark, lastTop;

    lines.forEach((line, index) => {
        const samples = getSampleCount(line);
        if (samples === 0) return;

        const lineTop = Math.floor(line.offsetTop * markerScale);
        const lineNumber = index + 1;
        const intensityClass = maxSamples > 0 ? (intensityToClass(samples / maxSamples) || 'cold') : 'cold';

        if (lineNumber === prevLine + 1 && lastMark?.classList.contains(intensityClass)) {
            lastMark.style.height = `${lineTop + lineHeight - lastTop}px`;
        } else {
            lastMark = createElement('div', `marker ${intensityClass}`);
            lastMark.style.height = `${lineHeight}px`;
            lastMark.style.top = `${lineTop}px`;
            scrollMarker.appendChild(lastMark);
            lastTop = lineTop;
        }

        prevLine = lineNumber;
    });

    document.body.appendChild(scrollMarker);
}

function applyLineColors() {
    const lines = document.querySelectorAll('.code-line');
    lines.forEach(line => {
        let intensity;
        if (colorMode === 'self') {
            intensity = parseFloat(line.getAttribute('data-self-intensity')) || 0;
        } else {
            intensity = parseFloat(line.getAttribute('data-cumulative-intensity')) || 0;
        }

        const color = intensityToColor(intensity);
        line.style.background = color;
    });
}

// ============================================================================
// Toggle Controls
// ============================================================================

function updateToggleUI(toggleId, isOn) {
    const toggle = document.getElementById(toggleId);
    if (toggle) {
        const track = toggle.querySelector('.toggle-track');
        const labels = toggle.querySelectorAll('.toggle-label');
        if (isOn) {
            track.classList.add('on');
            labels[0].classList.remove('active');
            labels[1].classList.add('active');
        } else {
            track.classList.remove('on');
            labels[0].classList.add('active');
            labels[1].classList.remove('active');
        }
    }
}

function toggleColdCode() {
    coldCodeHidden = !coldCodeHidden;
    applyHotFilter();
    updateToggleUI('toggle-cold', coldCodeHidden);
    buildScrollMarker();
}

function applyHotFilter() {
    const lines = document.querySelectorAll('.code-line');

    lines.forEach(line => {
        const selfSamples = line.querySelector('.line-samples-self')?.textContent.trim();
        const cumulativeSamples = line.querySelector('.line-samples-cumulative')?.textContent.trim();

        let isCold;
        if (colorMode === 'self') {
            isCold = !selfSamples || selfSamples === '';
        } else {
            isCold = !cumulativeSamples || cumulativeSamples === '';
        }

        if (isCold) {
            line.style.display = coldCodeHidden ? 'none' : 'flex';
        } else {
            line.style.display = 'flex';
        }
    });
}

function toggleColorMode() {
    colorMode = colorMode === 'self' ? 'cumulative' : 'self';
    applyLineColors();

    updateToggleUI('toggle-color-mode', colorMode === 'cumulative');

    if (coldCodeHidden) {
        applyHotFilter();
    }

    buildScrollMarker();
}

// ============================================================================
// Initialization
// ============================================================================

document.addEventListener('DOMContentLoaded', function() {
    restoreUIState();
    applyLineColors();

    // Initialize navigation buttons
    document.querySelectorAll('.nav-btn').forEach(button => {
        button.addEventListener('click', e => handleNavigationClick(button, e));
    });

    // Initialize line number permalink handlers
    document.querySelectorAll('.line-number').forEach(lineNum => {
        lineNum.style.cursor = 'pointer';
        lineNum.addEventListener('click', e => {
            window.location.hash = `line-${e.target.textContent.trim()}`;
        });
    });

    // Initialize toggle buttons
    const toggleColdBtn = document.getElementById('toggle-cold');
    if (toggleColdBtn) toggleColdBtn.addEventListener('click', toggleColdCode);

    const colorModeBtn = document.getElementById('toggle-color-mode');
    if (colorModeBtn) colorModeBtn.addEventListener('click', toggleColorMode);

    // Initialize specialization view toggle (hide if no bytecode data)
    const hasBytecode = document.querySelectorAll('.bytecode-toggle').length > 0;

    const specViewBtn = document.getElementById('toggle-spec-view');
    if (specViewBtn) {
        if (hasBytecode) {
            specViewBtn.addEventListener('click', toggleSpecView);
        } else {
            specViewBtn.style.display = 'none';
        }
    }

    // Initialize expand-all bytecode button
    const expandAllBtn = document.getElementById('toggle-all-bytecode');
    if (expandAllBtn) {
        if (hasBytecode) {
            expandAllBtn.addEventListener('click', toggleAllBytecode);
        } else {
            expandAllBtn.style.display = 'none';
        }
    }

    // Initialize span tooltips
    initSpanTooltips();

    // Build scroll marker and scroll to target
    setTimeout(buildScrollMarker, 200);
    setTimeout(scrollToTargetLine, 100);
});

// Close menu when clicking outside
document.addEventListener('click', e => {
    if (currentMenu && !currentMenu.contains(e.target) && !e.target.classList.contains('nav-btn')) {
        closeMenu();
    }
});

// ========================================
// SPECIALIZATION VIEW TOGGLE
// ========================================

let specViewEnabled = false;

/**
 * Calculate heat color for given intensity (0-1)
 * Hot spans (>30%) get warm orange, cold spans get dimmed gray
 * @param {number} intensity - Value between 0 and 1
 * @returns {string} rgba color string
 */
function calculateHeatColor(intensity) {
    // Hot threshold: only spans with >30% of max samples get color
    if (intensity > 0.3) {
        // Normalize intensity above threshold to 0-1
        const normalizedIntensity = (intensity - 0.3) / 0.7;
        // Warm orange-red with increasing opacity for hotter spans
        const alpha = 0.25 + normalizedIntensity * 0.35;  // 0.25 to 0.6
        const hotColor = getComputedStyle(document.documentElement).getPropertyValue('--span-hot-base').trim();
        return `rgba(${hotColor}, ${alpha})`;
    } else if (intensity > 0) {
        // Cold spans: very subtle gray, almost invisible
        const coldColor = getComputedStyle(document.documentElement).getPropertyValue('--span-cold-base').trim();
        return `rgba(${coldColor}, 0.1)`;
    }
    return 'transparent';
}

/**
 * Apply intensity-based heat colors to source spans
 * Hot spans get orange highlight, cold spans get dimmed
 * @param {boolean} enable - Whether to enable or disable span coloring
 */
function applySpanHeatColors(enable) {
    document.querySelectorAll('.instr-span').forEach(span => {
        const samples = enable ? (parseInt(span.dataset.samples) || 0) : 0;
        if (samples > 0) {
            const intensity = samples / (parseInt(span.dataset.maxSamples) || 1);
            span.style.backgroundColor = calculateHeatColor(intensity);
            span.style.borderRadius = '2px';
            span.style.padding = '0 1px';
            span.style.cursor = 'pointer';
        } else {
            span.style.cssText = '';
        }
    });
}

// ========================================
// SPAN TOOLTIPS
// ========================================

let activeTooltip = null;

/**
 * Create and show tooltip for a span
 */
function showSpanTooltip(span) {
    hideSpanTooltip();

    const samples = parseInt(span.dataset.samples) || 0;
    const maxSamples = parseInt(span.dataset.maxSamples) || 1;
    const pct = span.dataset.pct || '0';
    const opcodes = span.dataset.opcodes || '';

    if (samples === 0) return;

    const intensity = samples / maxSamples;
    const isHot = intensity > 0.7;
    const isWarm = intensity > 0.3;
    const hotnessText = isHot ? 'Hot' : isWarm ? 'Warm' : 'Cold';
    const hotnessClass = isHot ? 'hot' : isWarm ? 'warm' : 'cold';

    // Build opcodes rows - each opcode on its own row
    let opcodesHtml = '';
    if (opcodes) {
        const opcodeList = opcodes.split(',').map(op => op.trim()).filter(op => op);
        if (opcodeList.length > 0) {
            opcodesHtml = `
                <div class="span-tooltip-section">Opcodes:</div>
                ${opcodeList.map(op => `<div class="span-tooltip-opcode">${op}</div>`).join('')}
            `;
        }
    }

    const tooltip = document.createElement('div');
    tooltip.className = 'span-tooltip';
    tooltip.innerHTML = `
        <div class="span-tooltip-header ${hotnessClass}">${hotnessText}</div>
        <div class="span-tooltip-row">
            <span class="span-tooltip-label">Samples:</span>
            <span class="span-tooltip-value${isHot ? ' highlight' : ''}">${samples.toLocaleString()}</span>
        </div>
        <div class="span-tooltip-row">
            <span class="span-tooltip-label">% of line:</span>
            <span class="span-tooltip-value">${pct}%</span>
        </div>
        ${opcodesHtml}
    `;

    document.body.appendChild(tooltip);
    activeTooltip = tooltip;

    // Position tooltip above the span
    const rect = span.getBoundingClientRect();
    const tooltipRect = tooltip.getBoundingClientRect();

    let left = rect.left + (rect.width / 2) - (tooltipRect.width / 2);
    let top = rect.top - tooltipRect.height - 8;

    // Keep tooltip in viewport
    if (left < 5) left = 5;
    if (left + tooltipRect.width > window.innerWidth - 5) {
        left = window.innerWidth - tooltipRect.width - 5;
    }
    if (top < 5) {
        top = rect.bottom + 8; // Show below if no room above
    }

    tooltip.style.left = `${left + window.scrollX}px`;
    tooltip.style.top = `${top + window.scrollY}px`;
}

/**
 * Hide active tooltip
 */
function hideSpanTooltip() {
    if (activeTooltip) {
        activeTooltip.remove();
        activeTooltip = null;
    }
}

/**
 * Initialize span tooltip handlers
 */
function initSpanTooltips() {
    document.addEventListener('mouseover', (e) => {
        const span = e.target.closest('.instr-span');
        if (span && specViewEnabled) {
            showSpanTooltip(span);
        }
    });

    document.addEventListener('mouseout', (e) => {
        const span = e.target.closest('.instr-span');
        if (span) {
            hideSpanTooltip();
        }
    });
}

function toggleSpecView() {
    specViewEnabled = !specViewEnabled;
    const lines = document.querySelectorAll('.code-line');

    if (specViewEnabled) {
        lines.forEach(line => {
            const specColor = line.getAttribute('data-spec-color');
            line.style.background = specColor || 'transparent';
        });
    } else {
        applyLineColors();
    }

    applySpanHeatColors(specViewEnabled);
    updateToggleUI('toggle-spec-view', specViewEnabled);

    // Disable/enable color mode toggle based on spec view state
    const colorModeToggle = document.getElementById('toggle-color-mode');
    if (colorModeToggle) {
        colorModeToggle.classList.toggle('disabled', specViewEnabled);
    }

    buildScrollMarker();
}

// ========================================
// BYTECODE PANEL TOGGLE
// ========================================

/**
 * Toggle bytecode panel visibility for a source line
 * @param {HTMLElement} button - The toggle button that was clicked
 */
function toggleBytecode(button) {
    const lineDiv = button.closest('.code-line');
    const lineId = lineDiv.id;
    const lineNum = lineId.replace('line-', '');
    const panel = document.getElementById(`bytecode-${lineNum}`);
    const wrapper = document.getElementById(`bytecode-wrapper-${lineNum}`);

    if (!panel || !wrapper) return;

    const isExpanded = panel.classList.contains('expanded');

    if (isExpanded) {
        panel.classList.remove('expanded');
        wrapper.classList.remove('expanded');
        button.classList.remove('expanded');
        button.innerHTML = '&#9654;';  // Right arrow
    } else {
        if (!panel.dataset.populated) {
            populateBytecodePanel(panel, button);
        }
        panel.classList.add('expanded');
        wrapper.classList.add('expanded');
        button.classList.add('expanded');
        button.innerHTML = '&#9660;';  // Down arrow
    }
}

/**
 * Populate bytecode panel with instruction data
 * @param {HTMLElement} panel - The panel element to populate
 * @param {HTMLElement} button - The button containing the bytecode data
 */
function populateBytecodePanel(panel, button) {
    const bytecodeJson = button.getAttribute('data-bytecode');
    if (!bytecodeJson) return;

    // Get line number from parent
    const lineDiv = button.closest('.code-line');
    const lineNum = lineDiv ? lineDiv.id.replace('line-', '') : null;

    try {
        const instructions = JSON.parse(bytecodeJson);
        if (!instructions.length) {
            panel.innerHTML = '<div class="bytecode-empty">No bytecode data</div>';
            panel.dataset.populated = 'true';
            return;
        }

        const maxSamples = Math.max(...instructions.map(i => i.samples), 1);

        // Calculate specialization stats
        const totalSamples = instructions.reduce((sum, i) => sum + i.samples, 0);
        const specializedSamples = instructions
            .filter(i => i.is_specialized)
            .reduce((sum, i) => sum + i.samples, 0);
        const specPct = totalSamples > 0 ? Math.round(100 * specializedSamples / totalSamples) : 0;
        const specializedCount = instructions.filter(i => i.is_specialized).length;

        // Determine specialization level class
        let specClass = 'low';
        if (specPct >= 67) specClass = 'high';
        else if (specPct >= 33) specClass = 'medium';

        // Build specialization summary
        let html = `<div class="bytecode-spec-summary ${specClass}">
            <span class="spec-pct">${specPct}%</span>
            <span class="spec-label">specialized</span>
            <span class="spec-detail">(${specializedCount}/${instructions.length} instructions, ${specializedSamples.toLocaleString()}/${totalSamples.toLocaleString()} samples)</span>
        </div>`;

        html += '<div class="bytecode-header">' +
            '<span class="bytecode-opname">Instruction</span>' +
            '<span class="bytecode-samples">Samples</span>' +
            '<span>Heat</span></div>';

        for (const instr of instructions) {
            const heatPct = (instr.samples / maxSamples) * 100;
            const isHot = heatPct > 50;
            const specializedClass = instr.is_specialized ? ' specialized' : '';
            const baseOpHtml = instr.is_specialized
                ? `<span class="base-op">(${escapeHtml(instr.base_opname)})</span>` : '';
            const badge = instr.is_specialized
                ? '<span class="specialization-badge">SPECIALIZED</span>' : '';

            // Build location data attributes for cross-referencing with source spans
            const hasLocations = instr.locations && instr.locations.length > 0;
            const locationData = hasLocations
                ? `data-locations='${JSON.stringify(instr.locations)}' data-line="${lineNum}" data-opcode="${instr.opcode}"`
                : '';

            html += `<div class="bytecode-instruction" ${locationData}>
                <span class="bytecode-opname${specializedClass}">${escapeHtml(instr.opname)}${baseOpHtml}${badge}</span>
                <span class="bytecode-samples${isHot ? ' hot' : ''}">${instr.samples.toLocaleString()}</span>
                <div class="bytecode-heatbar"><div class="bytecode-heatbar-fill" style="width:${heatPct}%"></div></div>
            </div>`;
        }

        panel.innerHTML = html;
        panel.dataset.populated = 'true';

        // Add hover handlers for bytecode instructions to highlight source spans
        panel.querySelectorAll('.bytecode-instruction[data-locations]').forEach(instrEl => {
            instrEl.addEventListener('mouseenter', highlightSourceFromBytecode);
            instrEl.addEventListener('mouseleave', unhighlightSourceFromBytecode);
        });
    } catch (e) {
        panel.innerHTML = '<div class="bytecode-error">Error loading bytecode</div>';
        console.error('Error parsing bytecode data:', e);
    }
}

/**
 * Highlight source spans when hovering over bytecode instruction
 */
function highlightSourceFromBytecode(e) {
    const instrEl = e.currentTarget;
    const lineNum = instrEl.dataset.line;
    const locationsStr = instrEl.dataset.locations;

    if (!lineNum) return;

    const lineDiv = document.getElementById(`line-${lineNum}`);
    if (!lineDiv) return;

    // Parse locations and highlight matching spans by column range
    try {
        const locations = JSON.parse(locationsStr || '[]');
        const spans = lineDiv.querySelectorAll('.instr-span');
        spans.forEach(span => {
            const spanStart = parseInt(span.dataset.colStart);
            const spanEnd = parseInt(span.dataset.colEnd);
            for (const loc of locations) {
                // Match if span's range matches instruction's location
                if (spanStart === loc.col_offset && spanEnd === loc.end_col_offset) {
                    span.classList.add('highlight-from-bytecode');
                    break;
                }
            }
        });
    } catch (err) {
        console.error('Error parsing locations:', err);
    }

    // Also highlight the instruction row itself
    instrEl.classList.add('highlight');
}

/**
 * Remove highlighting from source spans
 */
function unhighlightSourceFromBytecode(e) {
    const instrEl = e.currentTarget;
    const lineNum = instrEl.dataset.line;

    if (!lineNum) return;

    const lineDiv = document.getElementById(`line-${lineNum}`);
    if (!lineDiv) return;

    const spans = lineDiv.querySelectorAll('.instr-span.highlight-from-bytecode');
    spans.forEach(span => {
        span.classList.remove('highlight-from-bytecode');
    });

    instrEl.classList.remove('highlight');
}

/**
 * Escape HTML special characters
 * @param {string} text - Text to escape
 * @returns {string} Escaped HTML
 */
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

/**
 * Toggle all bytecode panels at once
 */
function toggleAllBytecode() {
    const buttons = document.querySelectorAll('.bytecode-toggle');
    if (buttons.length === 0) return;

    const someExpanded = Array.from(buttons).some(b => b.classList.contains('expanded'));
    const expandAllBtn = document.getElementById('toggle-all-bytecode');

    buttons.forEach(button => {
        const isExpanded = button.classList.contains('expanded');
        if (someExpanded ? isExpanded : !isExpanded) {
            toggleBytecode(button);
        }
    });

    // Update the expand-all button state
    if (expandAllBtn) {
        expandAllBtn.classList.toggle('expanded', !someExpanded);
    }
}

// Keyboard shortcut: 'b' toggles all bytecode panels, Enter/Space activates toggle switches
document.addEventListener('keydown', function(e) {
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') {
        return;
    }
    if (e.key === 'b' && !e.ctrlKey && !e.altKey && !e.metaKey) {
        toggleAllBytecode();
    }
    if ((e.key === 'Enter' || e.key === ' ') && e.target.classList.contains('toggle-switch')) {
        e.preventDefault();
        e.target.click();
    }
});

// Handle hash changes
window.addEventListener('hashchange', () => setTimeout(scrollToTargetLine, 50));

// Rebuild scroll marker on resize
window.addEventListener('resize', buildScrollMarker);
