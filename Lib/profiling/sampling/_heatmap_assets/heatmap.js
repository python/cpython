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
        btn.innerHTML = next === 'dark' ? '&#9788;' : '&#9790;';  // sun or moon
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
            btn.innerHTML = savedTheme === 'dark' ? '&#9788;' : '&#9790;';
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
    // Restore UI state (theme, etc.)
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
    if (toggleColdBtn) {
        toggleColdBtn.addEventListener('click', toggleColdCode);
    }

    const colorModeBtn = document.getElementById('toggle-color-mode');
    if (colorModeBtn) {
        colorModeBtn.addEventListener('click', toggleColorMode);
    }

    // Build scroll marker
    setTimeout(buildScrollMarker, 200);

    // Setup scroll-to-line behavior
    setTimeout(scrollToTargetLine, 100);
});

// Close menu when clicking outside
document.addEventListener('click', e => {
    if (currentMenu && !currentMenu.contains(e.target) && !e.target.classList.contains('nav-btn')) {
        closeMenu();
    }
});

// Handle hash changes
window.addEventListener('hashchange', () => setTimeout(scrollToTargetLine, 50));

// Rebuild scroll marker on resize
window.addEventListener('resize', buildScrollMarker);
