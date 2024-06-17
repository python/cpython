 function onSwitch(event) {
     const option = event.target.selectedIndex;
     const item = event.target.options[option];
     window.location.href = item.dataset.url;
 }

 document.addEventListener("readthedocs-addons-data-ready", function(event) {
   const config = event.detail.data()

   // Add some mocked hardcoded versions pointing to the official
   // documentation while migrating to Read the Docs.
   // These are only for testing purposes.
   // TODO: remove them when managing all the versions on Read the Docs,
   // since all the "active, built and not hidden" versions will be shown automatically.
   let versions = config.versions.active.concat([
       {
           slug: "dev (3.14)",
           urls: {
               documentation: "https://docs.python.org/3.14/",
           }
       },
       {
           slug: "dev (3.13)",
           urls: {
               documentation: "https://docs.python.org/3.13/",
           }
       },
       {
           slug: "3.12",
           urls: {
               documentation: "https://docs.python.org/3.12/",
           }
       },
       {
           slug: "3.11",
           urls: {
               documentation: "https://docs.python.org/3.11/",
           }
       },
   ]);

   const versionSelect = `
   <select id="version_select">
   ${ versions.map(
       (version) => `
       <option
           value="${ version.slug }"
           ${ config.versions.current.slug === version.slug ? 'selected="selected"' : '' }
           data-url="${ version.urls.documentation }">
           ${ version.slug }
       </option>`
   ).join("\n") }
   </select>
   `;

   // Prepend the current language to the options on the selector
   let languages = config.projects.translations.concat(config.projects.current);
   languages = languages.sort((a, b) => a.language.name.localeCompare(b.language.name));

   const languageSelect = `
   <select id="language_select">
   ${ languages.map(
       (translation) => `
       <option
           value="${ translation.slug }"
           ${ config.projects.current.slug === translation.slug ? 'selected="selected"' : '' }
           data-url="${ translation.urls.documentation }">
           ${ translation.language.name }
       </option>`
   ).join("\n") }
   </select>
   `;

   // Query all the placeholders because there are different ones for Desktop/Mobile
   const versionPlaceholders = document.querySelectorAll(".version_switcher_placeholder");
   for (placeholder of versionPlaceholders) {
       placeholder.innerHTML = versionSelect;
       let selectElement = placeholder.querySelector("select");
       selectElement.addEventListener("change", onSwitch);
   }

   const languagePlaceholders = document.querySelectorAll(".language_switcher_placeholder");
   for (placeholder of languagePlaceholders) {
       placeholder.innerHTML = languageSelect;
       let selectElement = placeholder.querySelector("select");
       selectElement.addEventListener("change", onSwitch);
   }
 });
