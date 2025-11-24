// Toggle type section (stdlib, project, etc)
function toggleTypeSection(header) {
    const section = header.parentElement;
    const content = section.querySelector('.type-content');
    const icon = header.querySelector('.type-icon');

    if (content.style.display === 'none') {
        content.style.display = 'block';
        icon.textContent = '▼';
    } else {
        content.style.display = 'none';
        icon.textContent = '▶';
    }
}

// Toggle individual folder
function toggleFolder(header) {
    const folder = header.parentElement;
    const content = folder.querySelector('.folder-content');
    const icon = header.querySelector('.folder-icon');

    if (content.style.display === 'none') {
        content.style.display = 'block';
        icon.textContent = '▼';
        folder.classList.remove('collapsed');
    } else {
        content.style.display = 'none';
        icon.textContent = '▶';
        folder.classList.add('collapsed');
    }
}

// Expand all folders
function expandAll() {
    // Expand all type sections
    document.querySelectorAll('.type-section').forEach(section => {
        const content = section.querySelector('.type-content');
        const icon = section.querySelector('.type-icon');
        content.style.display = 'block';
        icon.textContent = '▼';
    });

    // Expand all folders
    document.querySelectorAll('.folder-node').forEach(folder => {
        const content = folder.querySelector('.folder-content');
        const icon = folder.querySelector('.folder-icon');
        content.style.display = 'block';
        icon.textContent = '▼';
        folder.classList.remove('collapsed');
    });
}

// Collapse all folders (but keep type sections expanded)
function collapseAll() {
    document.querySelectorAll('.folder-node').forEach(folder => {
        const content = folder.querySelector('.folder-content');
        const icon = folder.querySelector('.folder-icon');
        content.style.display = 'none';
        icon.textContent = '▶';
        folder.classList.add('collapsed');
    });
}
