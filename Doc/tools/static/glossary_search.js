"use strict";

const GLOSSARY_PAGE = "glossary.html";

const glossary_search = async () => {
  const response = await fetch("_static/glossary.json");
  if (!response.ok) {
    throw new Error("Failed to fetch glossary.json");
  }
  const glossary = await response.json();

  const params = new URLSearchParams(document.location.search).get("q");
  if (!params) {
    return;
  }

  const searchParam = params.toLowerCase();
  const glossaryItem = glossary[searchParam];
  if (!glossaryItem) {
    return;
  }

  const glossaryContainer = document.createElement("div");
  glossaryContainer.id = "glossary-result";
  glossaryContainer.className = "admonition seealso";
  const result_para = glossaryContainer.appendChild(
    document.createElement("p"),
  );
  result_para.className = "topic-title";
  const glossaryTitle = result_para.appendChild(document.createElement("a"));
  glossaryTitle.className = "glossary-title";
  glossaryTitle.href = "#";
  const glossaryBody = glossaryContainer.appendChild(
    document.createElement("div"),
  );
  glossaryBody.className = "glossary-body";

  // set up the title text with a link to the glossary page
  glossaryTitle.textContent = "Glossary: " + glossaryItem.title;
  const linkTarget = searchParam.replace(/ /g, "-");
  glossaryTitle.href = GLOSSARY_PAGE + "#term-" + linkTarget;

  // rewrite any anchor links (to other glossary terms)
  // to have a full reference to the glossary page
  const itemBody = glossaryBody.appendChild(document.createElement("div"));
  itemBody.innerHTML = glossaryItem.body;
  const anchorLinks = itemBody.querySelectorAll('a[href^="#"]');
  anchorLinks.forEach(function (link) {
    const currentUrl = link.getAttribute("href");
    link.href = GLOSSARY_PAGE + currentUrl;
  });

  const searchResults = document.getElementById("search-results");
  searchResults.insertAdjacentElement("afterbegin", glossaryContainer);
};

if (document.readyState !== "loading") {
  glossary_search().catch(console.error);
} else {
  document.addEventListener("DOMContentLoaded", glossary_search);
}
