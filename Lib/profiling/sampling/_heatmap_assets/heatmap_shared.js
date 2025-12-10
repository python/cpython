// Tachyon Profiler - Shared Heatmap JavaScript
// Common utilities shared between index and file views

// ============================================================================
// Heat Level Mapping (Single source of truth for intensity thresholds)
// ============================================================================

// Maps intensity (0-1) to heat level (0-8). Level 0 = no heat, 1-8 = heat levels.
function intensityToHeatLevel(intensity) {
    if (intensity <= 0) return 0;
    if (intensity <= 0.125) return 1;
    if (intensity <= 0.25) return 2;
    if (intensity <= 0.375) return 3;
    if (intensity <= 0.5) return 4;
    if (intensity <= 0.625) return 5;
    if (intensity <= 0.75) return 6;
    if (intensity <= 0.875) return 7;
    return 8;
}

// Class names corresponding to heat levels 1-8 (used by scroll marker)
const HEAT_CLASS_NAMES = ['cold', 'cool', 'mild', 'warm', 'hot', 'very-hot', 'intense', 'extreme'];

function intensityToClass(intensity) {
    const level = intensityToHeatLevel(intensity);
    return level === 0 ? null : HEAT_CLASS_NAMES[level - 1];
}

// ============================================================================
// Color Mapping (Intensity to Heat Color)
// ============================================================================

function intensityToColor(intensity) {
    const level = intensityToHeatLevel(intensity);
    if (level === 0) {
        return 'transparent';
    }
    const rootStyle = getComputedStyle(document.documentElement);
    return rootStyle.getPropertyValue(`--heat-${level}`).trim();
}

// ============================================================================
// Favicon (Reuse logo image as favicon)
// ============================================================================

(function() {
    const logo = document.querySelector('.brand-logo img');
    if (logo) {
        const favicon = document.createElement('link');
        favicon.rel = 'icon';
        favicon.type = 'image/png';
        favicon.href = logo.src;
        document.head.appendChild(favicon);
    }
})();
