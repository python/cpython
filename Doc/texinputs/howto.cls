%
% howto.cls for the Python documentation
%

\NeedsTeXFormat{LaTeX2e}[1995/12/01]
\ProvidesClass{howto}
             [1998/02/25 Document class (Python HOWTO)]

\RequirePackage{pypaper}

% Change the options here to get a different set of basic options,  This
% is where to add things like "a4paper" or "10pt".
%
\LoadClass[twoside]{article}

\setcounter{secnumdepth}{1}

% Optional packages:
%
% If processing of these documents fails at your TeX installation,
% these may be commented out (independently) to make things work.
% These are both supplied with the current version of the teTeX
% distribution.
%
% The "fancyhdr" package makes nicer page footers reasonable to
% implement, and is used to put the chapter and section information in 
% the footers.
%
\RequirePackage{fancyhdr}\typeout{Using fancier footers than usual.}


% Required package:
%
% This gives us all the Python-specific markup that we really want.
% This should come last.  Do not change this.
%
\RequirePackage{python}

% support for module synopsis sections:
\newcommand{\py@ModSynopsisFilename}{\jobname.syn}


% need to do one of these....
\newcommand{\py@doHorizontalRule}{\rule{\textwidth}{1pt}}


% Change the title page to look a bit better, and fit in with the
% fncychap ``Bjarne'' style a bit better.
%
\renewcommand{\maketitle}{
  \py@doHorizontalRule
  \@ifundefined{pdfinfo}{}{{
    % This \def is required to deal with multi-line authors; it
    % changes \\ to ', ' (comma-space), making it pass muster for
    % generating document info in the PDF file.
    \def\\{, }
    \pdfinfo{
      /Author (\@author)
      /Title (\@title)
    }
  }}
  \begin{flushright}
    {\rm\Huge\py@HeaderFamily \@title} \par
    {\em\large\py@HeaderFamily \py@release} \par
    \vspace{25pt}
    {\Large\py@HeaderFamily \@author} \par
    \vspace{25pt}
    \@date \par
    \py@authoraddress \par
  \end{flushright}
  \@thanks
  \setcounter{footnote}{0}
  \let\thanks\relax\let\maketitle\relax
  \gdef\@thanks{}\gdef\@author{}\gdef\@title{}
}


\let\py@OldTableofcontents=\tableofcontents
\renewcommand{\tableofcontents}{
  \begingroup
    \parskip = 0mm
    \py@OldTableofcontents
  \endgroup
  \py@doHorizontalRule
  \vspace{12pt}
  \py@doing@page@targetstrue
}  

% Fix the theindex environment to add an entry to the Table of
% Contents; this is much nicer than just having to jump to the end of
% the book and flip around, especially with multiple indexes.
%
\let\py@OldTheindex=\theindex
\renewcommand{\theindex}{
  \clearpage
  \py@OldTheindex
  \addcontentsline{toc}{section}{\indexname}
}

\@ifundefined{fancyhf}{
  \pagestyle{plain}}{
  \pagestyle{normal}}		% start this way; change for
\pagenumbering{arabic}		% ToC & chapters
\setcounter{secnumdepth}{2}

\thispagestyle{empty}
