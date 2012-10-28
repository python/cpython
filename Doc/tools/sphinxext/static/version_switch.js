(function() {
  'use strict';

  var all_versions = {
    '3.4': 'dev (3.4)',
    '3.3': '3.3',
    '3.2': '3.2',
    '2.7': '2.7',
    '2.6': '2.6'
  };

  function build_select(current_version, current_release) {
    var buf = ['<select>'];

    $.each(all_versions, function(version, title) {
      buf.push('<option value="' + version + '"');
      if (version == current_version)
        buf.push(' selected="selected">' + current_release + '</option>');
      else
        buf.push('>' + title + '</option>');
    });

    buf.push('</select>');
    return buf.join('');
  }

  function patch_url(url, new_version) {
    var url_re = /\.org\/(\d|py3k|dev|((release\/)?\d\.\d[\w\d\.]*))\//,
        new_url = url.replace(url_re, '.org/' + new_version + '/');

    if (new_url == url && !new_url.match(url_re)) {
      // python 2 url without version?
      new_url = url.replace(/\.org\//, '.org/' + new_version + '/');
    }
    return new_url;
  }

  function on_switch() {
    var selected = $(this).children('option:selected').attr('value');

    var url = window.location.href,
        new_url = patch_url(url, selected);

    if (new_url != url) {
      // check beforehand if url exists, else redirect to version's start page
      $.ajax({
        url: new_url,
        success: function() {
           window.location.href = new_url;
        },
        error: function() {
           window.location.href = 'http://docs.python.org/' + selected;
        }
      });
    }
  }

  $(document).ready(function() {
    var release = DOCUMENTATION_OPTIONS.VERSION;
    var version = release.substr(0, 3);
    var select = build_select(version, release);

    $('.version_switcher_placeholder').html(select);
    $('.version_switcher_placeholder select').bind('change', on_switch);
  });
})();
