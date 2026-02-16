// Tachyon Profiler - Shared JavaScript
// Common utilities shared between flamegraph and heatmap views

// ============================================================================
// Theme Support
// ============================================================================

// Storage key for theme preference
const THEME_STORAGE_KEY = 'tachyon-theme';

// Get the preferred theme from localStorage or system preference
function getPreferredTheme() {
  const saved = localStorage.getItem(THEME_STORAGE_KEY);
  if (saved) return saved;
  return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
}

// Apply theme and update UI
function applyTheme(theme) {
  document.documentElement.setAttribute('data-theme', theme);
  const btn = document.getElementById('theme-btn');
  if (btn) {
    const moonIcon = btn.querySelector('.icon-moon');
    const sunIcon = btn.querySelector('.icon-sun');
    if (moonIcon) moonIcon.style.display = theme === 'dark' ? 'none' : '';
    if (sunIcon) sunIcon.style.display = theme === 'dark' ? '' : 'none';
  }
}

// Toggle theme and save preference. Returns the new theme.
function toggleAndSaveTheme() {
  const current = document.documentElement.getAttribute('data-theme') || 'light';
  const next = current === 'light' ? 'dark' : 'light';
  applyTheme(next);
  localStorage.setItem(THEME_STORAGE_KEY, next);
  return next;
}
