:orphan:

****************************
Improve a documentation page
****************************

.. This is the JavaScript-enabled version of this page. Another version
   (for those with JavaScript disabled) is improve-page-nojs.rst. If you
   edit this page, please also edit that one, and vice versa.

.. only:: html and not epub

   .. raw:: html

      <script>
         function applyReplacements(text, params) {
            return text
               .replace(/PAGETITLE/g, params.get('pagetitle'))
               .replace(/PAGEURL/g, params.get('pageurl'))
               .replace(/PAGESOURCE/g, params.get('pagesource'));
         }

         document.addEventListener('DOMContentLoaded', () => {
            const params = new URLSearchParams(window.location.search);
            const walker = document.createTreeWalker(
               document.body,
               NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT,
               null
            );

            while (walker.nextNode()) {
               const node = walker.currentNode;

               if (node.nodeType === Node.TEXT_NODE) {
                  node.textContent = applyReplacements(node.textContent, params)
               } else if (node.nodeName === 'A' && node.href) {
                  node.setAttribute('href', applyReplacements(node.getAttribute('href'), params));
               }
            }
         });
      </script>

We are always interested to hear ideas about improvements to the documentation.

You were reading "PAGETITLE" at `<PAGEURL>`_.  The source for that page is on
`GitHub <https://github.com/python/cpython/blob/main/Doc/PAGESOURCE?plain=1>`_.

.. only:: translation

   If the bug or suggested improvement concerns the translation of this
   documentation, open an issue or edit the page in
   `translation's repository <TRANSLATION_REPO_>`_ instead.

You have a few ways to ask questions or suggest changes:

- You can start a discussion about the page on the Python discussion forum.
  This link will start a pre-populated topic:
  `Question about page "PAGETITLE" <https://discuss.python.org/new-topic?category=documentation&title=Question+about+page+%22PAGETITLE%22&body=About+the+page+at+PAGEURL%3A>`_.

- You can open an issue on the Python GitHub issue tracker. This link will
  create a new pre-populated issue:
  `Docs: problem with page "PAGETITLE" <https://github.com/python/cpython/issues/new?template=documentation.yml&title=Docs%3A+problem+with+page+%22PAGETITLE%22&description=The+page+at+PAGEURL+has+a+problem%3A>`_.

- You can `edit the page on GitHub <https://github.com/python/cpython/blob/main/Doc/PAGESOURCE?plain=1>`_
  to open a pull request and begin the contribution process.
