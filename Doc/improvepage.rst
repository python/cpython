:orphan:

.. _improve-a-page:

****************************
Improve a documentation page
****************************

.. only:: html and not epub

   .. raw:: html

      <script>
         document.addEventListener('DOMContentLoaded', () => {
            const params = new URLSearchParams(window.location.search);
            document.body.innerHTML = document.body.innerHTML
               .replace(/PAGETITLE/g, params.get('pagetitle'))
               .replace(/PAGEURL/g, params.get('pageurl'))
               .replace(/PAGESOURCE/g, params.get('pagesource'));
         });
      </script>

We are always interested to hear about ways to improve the documentation.

You were reading "PAGETITLE" at `<PAGEURL>`_.  The source for that page is on
`GitHub <https://github.com/python/cpython/blob/main/Doc/PAGESOURCE?plain=1>`_.

You have a few options for asking questions or suggesting changes:

- You can start a discussion about the page on the Python discussion forum.
  This link will start a pre-populated topic:
  `Question about "PAGETITLE" <https://discuss.python.org/new-topic?category=documentation&title=Question+about+%22PAGETITLE%22&body=About+the+page+at+PAGEURL%3A>`_.

- You can open an issue on the Python GitHub issue tracker. This link will
  create a new issue pre-populated with some information for you:
  `Docs: problem with "PAGETITLE" <https://github.com/python/cpython/issues/new?title=Docs%3A+problem+with+%22PAGETITLE%22&labels=docs&body=The+page+at+PAGEURL+has+a+problem%3A>`_.

- You can `edit the page on GitHub <https://github.com/python/cpython/blob/main/Doc/PAGESOURCE?plain=1>`_
  and open a pull request, though you will need to have signed a contributor agreement before it can be merged.
