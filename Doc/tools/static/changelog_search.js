$(document).ready(function() {
    // add the search form and bind the events
    $('h1').after([
      '<p>Filter entries by content:',
      '<input type="text" value="" id="searchbox" style="width: 50%">',
      '<input type="submit" id="searchbox-submit" value="Filter"></p>'
    ].join('\n'));

    function dofilter() {
        try {
            var query = new RegExp($('#searchbox').val(), 'i');
        }
        catch (e) {
            return; // not a valid regex (yet)
        }
        // find headers for the versions (What's new in Python X.Y.Z?)
        $('#changelog h2').each(function(index1, h2) {
            var h2_parent = $(h2).parent();
            var sections_found = 0;
            // find headers for the sections (Core, Library, etc.)
            h2_parent.find('h3').each(function(index2, h3) {
                var h3_parent = $(h3).parent();
                var entries_found = 0;
                // find all the entries
                h3_parent.find('li').each(function(index3, li) {
                    var li = $(li);
                    // check if the query matches the entry
                    if (query.test(li.text())) {
                        li.show();
                        entries_found++;
                    }
                    else {
                        li.hide();
                    }
                });
                // if there are entries, show the section, otherwise hide it
                if (entries_found > 0) {
                    h3_parent.show();
                    sections_found++;
                }
                else {
                    h3_parent.hide();
                }
            });
            if (sections_found > 0)
                h2_parent.show();
            else
                h2_parent.hide();
        });
    }
    $('#searchbox').keyup(dofilter);
    $('#searchbox-submit').click(dofilter);
});
