document.addEventListener("DOMContentLoaded", function () {
  // add the search form and bind the events
  document
    .querySelector("h1")
    .insertAdjacentHTML(
      "afterend",
      [
        "<p>Filter entries by content:",
        '<input type="text" value="" id="searchbox" style="width: 50%">',
        '<input type="submit" id="searchbox-submit" value="Filter"></p>',
      ].join("\n"),
    );

  function doFilter() {
    let query;
    try {
      query = new RegExp(document.querySelector("#searchbox").value, "i");
    } catch (e) {
      return; // not a valid regex (yet)
    }
    // find headers for the versions (What's new in Python X.Y.Z?)
    const h2s = document.querySelectorAll("#changelog h2");
    for (const h2 of h2s) {
      let sections_found = 0;
      // find headers for the sections (Core, Library, etc.)
      const h3s = h2.parentNode.querySelectorAll("h3");
      for (const h3 of h3s) {
        let entries_found = 0;
        // find all the entries
        const lis = h3.parentNode.querySelectorAll("li");
        for (let li of lis) {
          // check if the query matches the entry
          if (query.test(li.textContent)) {
            li.style.display = "block";
            entries_found++;
          } else {
            li.style.display = "none";
          }
        }
        // if there are entries, show the section, otherwise hide it
        if (entries_found > 0) {
          h3.parentNode.style.display = "block";
          sections_found++;
        } else {
          h3.parentNode.style.display = "none";
        }
      }
      if (sections_found > 0) {
        h2.parentNode.style.display = "block";
      } else {
        h2.parentNode.style.display = "none";
      }
    }
  }
  document.querySelector("#searchbox").addEventListener("keyup", doFilter);
  document
    .querySelector("#searchbox-submit")
    .addEventListener("click", doFilter);
});
