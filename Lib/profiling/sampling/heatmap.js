// Tachyon Profiler - Heatmap JavaScript
// Interactive features for the heatmap visualization

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

// State management
let currentMenu = null;

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
        item.appendChild(createElement('div', 'callee-menu-func', linkData.func));
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

// Get sample count from line element
function getSampleCount(line) {
    const text = line.querySelector('.line-samples')?.textContent.trim().replace(/,/g, '');
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
