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

  // set up the title text with a link to the glossary page
  const glossaryTitle = document.getElementById("glossary-title");
  glossaryTitle.textContent = "Glossary: " + glossaryItem.title;
  const linkTarget = searchParam.replace(/ /g, "-");
  glossaryTitle.href = GLOSSARY_PAGE + "#term-" + linkTarget;

  // rewrite any anchor links (to other glossary terms)
  // to have a full reference to the glossary page
  const glossaryBody = document.getElementById("glossary-body");
  glossaryBody.innerHTML = glossaryItem.body;
  const anchorLinks = glossaryBody.querySelectorAll('a[href^="#"]');
  anchorLinks.forEach(function (link) {
    const currentUrl = link.getAttribute("href");
    link.href = GLOSSARY_PAGE + currentUrl;
  });

  const glossaryResult = document.getElementById("glossary-result");
  glossaryResult.style.display = "";
};

if (document.readyState !== "loading") {
  glossary_search().catch(console.error);
} else {
  document.addEventListener("DOMContentLoaded", glossary_search);
}
