// Tachyon Profiler - Heatmap Index JavaScript
// Index page specific functionality

// ============================================================================
// Heatmap Bar Coloring
// ============================================================================

function applyHeatmapBarColors() {
    const bars = document.querySelectorAll('.heatmap-bar[data-intensity]');
    bars.forEach(bar => {
        const intensity = parseFloat(bar.getAttribute('data-intensity')) || 0;
        const color = intensityToColor(intensity);
        bar.style.backgroundColor = color;
    });
}

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

    applyHeatmapBarColors();
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
// Type Section Toggle (stdlib, project, etc)
// ============================================================================

function toggleTypeSection(header) {
    const section = header.parentElement;
    const content = section.querySelector('.type-content');
    const icon = header.querySelector('.type-icon');

    if (content.style.display === 'none') {
        content.style.display = 'block';
        icon.textContent = '\u25BC';
    } else {
        content.style.display = 'none';
        icon.textContent = '\u25B6';
    }
}

// ============================================================================
// Folder Toggle
// ============================================================================

function toggleFolder(header) {
    const folder = header.parentElement;
    const content = folder.querySelector('.folder-content');
    const icon = header.querySelector('.folder-icon');

    if (content.style.display === 'none') {
        content.style.display = 'block';
        icon.textContent = '\u25BC';
        folder.classList.remove('collapsed');
    } else {
        content.style.display = 'none';
        icon.textContent = '\u25B6';
        folder.classList.add('collapsed');
    }
}

// ============================================================================
// Expand/Collapse All
// ============================================================================

function expandAll() {
    // Expand all type sections
    document.querySelectorAll('.type-section').forEach(section => {
        const content = section.querySelector('.type-content');
        const icon = section.querySelector('.type-icon');
        content.style.display = 'block';
        icon.textContent = '\u25BC';
    });

    // Expand all folders
    document.querySelectorAll('.folder-node').forEach(folder => {
        const content = folder.querySelector('.folder-content');
        const icon = folder.querySelector('.folder-icon');
        content.style.display = 'block';
        icon.textContent = '\u25BC';
        folder.classList.remove('collapsed');
    });
}

function collapseAll() {
    document.querySelectorAll('.folder-node').forEach(folder => {
        const content = folder.querySelector('.folder-content');
        const icon = folder.querySelector('.folder-icon');
        content.style.display = 'none';
        icon.textContent = '\u25B6';
        folder.classList.add('collapsed');
    });
}

// ============================================================================
// Initialization
// ============================================================================

document.addEventListener('DOMContentLoaded', function() {
    restoreUIState();
    applyHeatmapBarColors();
});
