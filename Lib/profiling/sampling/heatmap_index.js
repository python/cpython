function filterTable() {
    const searchTerm = document.getElementById('searchInput').value.toLowerCase();
    const moduleFilter = document.getElementById('moduleFilter').value;
    const table = document.getElementById('fileTable');
    const rows = table.getElementsByTagName('tr');

    for (let i = 1; i < rows.length; i++) {
        const row = rows[i];
        const text = row.textContent.toLowerCase();
        const moduleType = row.getAttribute('data-module-type');

        const matchesSearch = text.includes(searchTerm);
        const matchesModule = moduleFilter === 'all' || moduleType === moduleFilter;

        row.style.display = (matchesSearch && matchesModule) ? '' : 'none';
    }
}

// Track current sort state
let currentSortColumn = -1;
let currentSortAscending = true;

function sortTable(columnIndex) {
    const table = document.getElementById('fileTable');
    const tbody = table.querySelector('tbody');
    const rows = Array.from(tbody.querySelectorAll('tr'));

    // Determine sort direction
    let ascending = true;
    if (currentSortColumn === columnIndex) {
        // Same column - toggle direction
        ascending = !currentSortAscending;
    } else {
        // New column - default direction based on type
        // For numeric columns (samples, lines, %), descending is default
        // For text columns (file, module, type), ascending is default
        ascending = columnIndex <= 2; // Columns 0-2 are text, 3+ are numeric
    }

    rows.sort((a, b) => {
        let aVal = a.cells[columnIndex].textContent.trim();
        let bVal = b.cells[columnIndex].textContent.trim();

        // Try to parse as number
        const aNum = parseFloat(aVal.replace(/,/g, '').replace('%', ''));
        const bNum = parseFloat(bVal.replace(/,/g, '').replace('%', ''));

        let result;
        if (!isNaN(aNum) && !isNaN(bNum)) {
            result = aNum - bNum;  // Numeric comparison
        } else {
            result = aVal.localeCompare(bVal);  // String comparison
        }

        return ascending ? result : -result;
    });

    rows.forEach(row => tbody.appendChild(row));

    // Update sort state
    currentSortColumn = columnIndex;
    currentSortAscending = ascending;
}
