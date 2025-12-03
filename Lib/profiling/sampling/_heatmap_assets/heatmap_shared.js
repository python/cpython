// Tachyon Profiler - Shared Heatmap JavaScript
// Common utilities shared between index and file views

// ============================================================================
// Color Mapping (Intensity to Heat Color)
// ============================================================================

function intensityToColor(intensity) {
    if (intensity <= 0) {
        return 'transparent';
    }

    const rootStyle = getComputedStyle(document.documentElement);

    if (intensity <= 0.125) {
        return rootStyle.getPropertyValue('--heat-1').trim();
    } else if (intensity <= 0.25) {
        return rootStyle.getPropertyValue('--heat-2').trim();
    } else if (intensity <= 0.375) {
        return rootStyle.getPropertyValue('--heat-3').trim();
    } else if (intensity <= 0.5) {
        return rootStyle.getPropertyValue('--heat-4').trim();
    } else if (intensity <= 0.625) {
        return rootStyle.getPropertyValue('--heat-5').trim();
    } else if (intensity <= 0.75) {
        return rootStyle.getPropertyValue('--heat-6').trim();
    } else if (intensity <= 0.875) {
        return rootStyle.getPropertyValue('--heat-7').trim();
    } else {
        return rootStyle.getPropertyValue('--heat-8').trim();
    }
}
