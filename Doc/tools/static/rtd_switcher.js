 function onSwitch(event) {
     const option = event.target.selectedIndex;
     const item = event.target.options[option];
     window.location.href = item.dataset.url;
 }

 document.addEventListener("readthedocs-addons-data-ready", function(event) {
   const config = event.detail.data()
   const versionSelect = `
   <select id="version_select" aria-label="Python version">
   ${ config.versions.active.map(
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
   <select id="language_select" aria-label="Language">
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
