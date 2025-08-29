var Scorer = {
  score: function (result) {
    let [docname, title, anchor, descr, score, filename] = result;
    if (docname == "library/stdtypes" || docname == "library/functions") {
      score += 10;
    }
    return score;
  },

  objPrio: {
    0: 15,
    1: 5,
    2: -5,
  },
  objPrioDefault: 0,

  objNameMatch: 20,
  objPartialMatch: 6,

  title: 15,
  partialTitle: 7,
  term: 5,
  partialTerm: 2,
};
