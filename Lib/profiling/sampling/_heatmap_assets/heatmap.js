// Tachyon Profiler - Heatmap JavaScript
// Interactive features for the heatmap visualization

// State management
let currentMenu = null;
let colorMode = 'self';  // 'self' or 'cumulative' - default to self
let coldCodeHidden = false;

// Apply background colors on page load
document.addEventListener('DOMContentLoaded', function() {
    // Apply background colors
    document.querySelectorAll('.code-line[data-bg-color]').forEach(line => {
        const bgColor = line.getAttribute('data-bg-color');
        if (bgColor) {
            line.style.background = bgColor;
        }
    });
});

// Utility: Create element with class and content
function createElement(tag, className, textContent = '') {
    const el = document.createElement(tag);
    if (className) el.className = className;
    if (textContent) el.textContent = textContent;
    return el;
}

// Utility: Calculate smart menu position
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

// Close and remove current menu
function closeMenu() {
    if (currentMenu) {
        currentMenu.remove();
        currentMenu = null;
    }
}

// Show navigation menu for multiple options
function showNavigationMenu(button, items, title) {
    closeMenu();

    const menu = createElement('div', 'callee-menu');
    menu.appendChild(createElement('div', 'callee-menu-header', title));

    items.forEach(linkData => {
        const item = createElement('div', 'callee-menu-item');

        // Create function name with count
        const funcDiv = createElement('div', 'callee-menu-func');
        funcDiv.textContent = linkData.func;

        // Add count badge if available
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

// Handle navigation button clicks
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

// Initialize navigation buttons
document.querySelectorAll('.nav-btn').forEach(button => {
    button.addEventListener('click', e => handleNavigationClick(button, e));
});

// Close menu when clicking outside
document.addEventListener('click', e => {
    if (currentMenu && !currentMenu.contains(e.target) && !e.target.classList.contains('nav-btn')) {
        closeMenu();
    }
});

// Scroll to target line (centered using CSS scroll-margin-top)
function scrollToTargetLine() {
    if (!window.location.hash) return;
    const target = document.querySelector(window.location.hash);
    if (target) {
        target.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
}

// Initialize line number permalink handlers
document.querySelectorAll('.line-number').forEach(lineNum => {
    lineNum.style.cursor = 'pointer';
    lineNum.addEventListener('click', e => {
        window.location.hash = `line-${e.target.textContent.trim()}`;
    });
});

// Setup scroll-to-line behavior
setTimeout(scrollToTargetLine, 100);
window.addEventListener('hashchange', () => setTimeout(scrollToTargetLine, 50));

// Get sample count from line element based on current color mode
function getSampleCount(line) {
    let text;
    if (colorMode === 'self') {
        text = line.querySelector('.line-samples-self')?.textContent.trim().replace(/,/g, '');
    } else {
        text = line.querySelector('.line-samples-cumulative')?.textContent.trim().replace(/,/g, '');
    }
    return parseInt(text) || 0;
}

// Classify intensity based on ratio
function getIntensityClass(ratio) {
    if (ratio > 0.75) return 'vhot';
    if (ratio > 0.5) return 'hot';
    if (ratio > 0.25) return 'warm';
    return 'cold';
}

// Build scroll minimap showing hotspot locations
function buildScrollMarker() {
    const existing = document.getElementById('scroll_marker');
    if (existing) existing.remove();

    if (document.body.scrollHeight <= window.innerHeight) return;

    const lines = document.querySelectorAll('.code-line');
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
        const intensityClass = maxSamples > 0 ? getIntensityClass(samples / maxSamples) : 'cold';

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

// Build scroll marker on load and resize
setTimeout(buildScrollMarker, 200);
window.addEventListener('resize', buildScrollMarker);

// ========================================
// HIDE COLD CODE TOGGLE
// ========================================

function toggleColdCode() {
    coldCodeHidden = !coldCodeHidden;
    applyHotFilter();
    updateToggleUI('toggle-cold', coldCodeHidden);
}

// Apply hot filter based on current coldCodeHidden and colorMode state
function applyHotFilter() {
    const lines = document.querySelectorAll('.code-line');

    lines.forEach(line => {
        const selfSamples = line.querySelector('.line-samples-self')?.textContent.trim();
        const cumulativeSamples = line.querySelector('.line-samples-cumulative')?.textContent.trim();

        // Determine if line should be hidden based on color mode
        let isCold;
        if (colorMode === 'self') {
            // In self mode, hide lines with no self samples
            isCold = !selfSamples || selfSamples === '';
        } else {
            // In cumulative mode, hide lines with no cumulative samples
            isCold = !cumulativeSamples || cumulativeSamples === '';
        }

        if (isCold) {
            line.style.display = coldCodeHidden ? 'none' : 'flex';
        } else {
            // Line has samples, always show it
            line.style.display = 'flex';
        }
    });
}

// Update toggle UI state
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

// Initialize toggle button
document.addEventListener('DOMContentLoaded', () => {
    const toggleBtn = document.getElementById('toggle-cold');
    if (toggleBtn) {
        toggleBtn.addEventListener('click', toggleColdCode);
    }
});

// ========================================
// COLOR MODE TOGGLE (SELF vs CUMULATIVE)
// ========================================

function toggleColorMode() {
    colorMode = colorMode === 'self' ? 'cumulative' : 'self';
    const lines = document.querySelectorAll('.code-line');

    lines.forEach(line => {
        let bgColor;
        if (colorMode === 'self') {
            bgColor = line.getAttribute('data-self-color');
        } else {
            bgColor = line.getAttribute('data-cumulative-color');
        }

        if (bgColor) {
            line.style.background = bgColor;
        }
    });

    updateToggleUI('toggle-color-mode', colorMode === 'cumulative');

    // Reapply hot filter if enabled (filtering criteria depends on color mode)
    if (coldCodeHidden) {
        applyHotFilter();
    }

    // Rebuild scroll marker with new color mode
    buildScrollMarker();
}

// Initialize color mode toggle button
document.addEventListener('DOMContentLoaded', () => {
    const colorModeBtn = document.getElementById('toggle-color-mode');
    if (colorModeBtn) {
        colorModeBtn.addEventListener('click', toggleColorMode);
    }
});
