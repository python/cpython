(function() {
  'use strict';

  // Parses versions in URL segments like:
  // "/3", "/dev", "/release/2.7" or "/3.6rc2"
  var version_regexs = [
    '(?:\\d)',
    '(?:\\d\\.\\d[\\w\\d\\.]*)',
    '(?:py3k)',
    '(?:dev)',
    '(?:release/\\d.\\d[\\x\\d\\.]*)'];

  var all_versions = {
    '3.7': 'dev (3.7)',
    '3.6': '3.6',
    '3.5': '3.5',
    '3.4': '3.4',
    '3.3': '3.3',
    '2.7': '2.7',
  };

  var all_languages = {
      'en': 'English',
      'fr': 'Français',
  };

  function build_version_select(current_version, current_release) {
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

  function build_language_select(current_language) {
    var buf = ['<select>'];

    $.each(all_languages, function(language, title) {
      if (language == current_language)
        buf.push('<option value="' + language + '" selected="selected">' +
                 all_languages[current_language] + '</option>');
      else
        buf.push('<option value="' + language + '">' + title + '</option>');
    });
    buf.push('</select>');
    return buf.join('');
  }

  function naviagate_if_exists(url, default_url) {
    // check beforehand if url exists, else redirect to default_url.
    $.ajax({
      url: url,
      success: function() {
         window.location.href = url;
      },
      error: function() {
         window.location.href = default_url;
      }
    });
  }

  function on_version_switch() {
    var selected_version = $(this).children('option:selected').attr('value') + '/';
    var url = window.location.href;
    var current_language = find_language_in_url(url);
    var current_version = find_version_in_url(url);
    var new_url = url.replace('.org/' + current_language + current_version,
                              '.org/' + current_language + selected_version);
    if (new_url != url) {
      naviagate_if_exists(new_url, 'https://docs.python.org/' +
                          current_language + selected_version);
    }
  }

  function on_language_switch() {
    var selected_language = $(this).children('option:selected').attr('value') + '/';
    var url = window.location.href;
    var current_language = find_language_in_url(url);
    var current_version = find_version_in_url(url);
    if (selected_language == 'en/') // Special 'default' case for english.
      selected_language = '';
    var new_url = url.replace('.org/' + current_language + current_version,
                              '.org/' + selected_language + current_version);
    if (new_url != url) {
      naviagate_if_exists(new_url, 'https://docs.python.org/' +
                          selected_language + current_version);
    }
  }

  // Returns the path segment as a string, like 'fr/' or '' if not found.
  function find_language_in_url(url) {
    var language_regexp = '\.org/(' + Object.keys(all_languages).join('|') + '/)';
    var match = url.match(language_regexp);
    if (match !== null)
        return match[1];
    return '';
  }

  // Returns the path segment as a string, like '3.6/' or '' if not found.
  function find_version_in_url(url) {
    var language_segment = '(?:(?:' + Object.keys(all_languages).join('|') + ')/)';
    var version_segment = '(?:(?:' + version_regexs.join('|') + ')/)';
    var version_regexp = '\\.org/' + language_segment + '?(' + version_segment + ')';
    var match = url.match(version_regexp);
    if (match !== null)
      return match[1];
    return ''
  }

  $(document).ready(function() {
    var release = DOCUMENTATION_OPTIONS.VERSION;
    var current_language = find_language_in_url(window.location.href).replace(
            /\/+$/g, '') || 'en';
    var version = release.substr(0, 3);
    var version_select = build_version_select(version, release);

    $('.version_switcher_placeholder').html(version_select);
    $('.version_switcher_placeholder select').bind('change', on_version_switch);

    var language_select = build_language_select(current_language);

    $('.language_switcher_placeholder').html(language_select);
    $('.language_switcher_placeholder select').bind('change', on_language_switch);
  });
})();
