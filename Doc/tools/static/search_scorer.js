var Scorer = {
  score: function (result) {
    let [docname, title, anchor, descr, score, filename] = result;

    // boost the score of built-in functions and types
    const builtinPages = ["library/stdtypes", "library/functions"];
    if (builtinPages.includes(docname)) {
      score += 7;
    }

    return score;
  },

  // Additive scores depending on the priority of the object
  // Priority is set by object domains
  // (see https://www.sphinx-doc.org/en/master/extdev/domainapi.html)
  objPrio: {
    0: 15,
    1: 5,
    2: -5,
  },
  objPrioDefault: 0,

  objNameMatch: 11, // score if object's name exactly matches search query
  objPartialMatch: 6, // score if object's name contains search query

  title: 15, // score if title exactly matches search query
  partialTitle: 7, // score if title contains search query

  term: 5, // score if a term exactly matches search query
  partialTerm: 2, // score if a term contains search query
};
