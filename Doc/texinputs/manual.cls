%
% manual.cls for the Python documentation
%

\NeedsTeXFormat{LaTeX2e}[1995/12/01]
\ProvidesClass{manual}
             [1998/03/03 Document class (Python manual)]

\RequirePackage{pypaper}
\RequirePackage{fancybox}

% Change the options here to get a different set of basic options, but only
% if you have to.  Paper and font size should be adjusted in pypaper.sty.
%
\LoadClass[\py@paper,\py@ptsize,twoside,openright]{report}

\setcounter{secnumdepth}{2}

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


% Required packages:
%
% The "fncychap" package is used to get the nice chapter headers.  The
% .sty file is distributed with Python, so you should not need to disable
% it.  You'd also end up with a mixed page style; uglier than stock LaTeX!
%
\RequirePackage[Bjarne]{fncychap}\typeout{Using fancy chapter headings.}
% Do horizontal rules it this way to match:
\newcommand{\py@doHorizontalRule}{\mghrulefill{\RW}}
%
%
% This gives us all the Python-specific markup that we really want.
% This should come last.  Do not change this.
%
\RequirePackage{python}

% support for module synopsis sections:
\newcommand{\py@ModSynopsisFilename}{\jobname\thechapter.syn}
\let\py@OldChapter=\chapter
\renewcommand{\chapter}{
  \py@ProcessModSynopsis
  \py@closeModSynopsisFile
  \py@OldChapter
}


% Change the title page to look a bit better, and fit in with the
% fncychap ``Bjarne'' style a bit better.
%
\renewcommand{\maketitle}{%
  \begin{titlepage}%
    \let\footnotesize\small
    \let\footnoterule\relax
    \py@doHorizontalRule%
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
    \begin{flushright}%
      {\rm\Huge\py@HeaderFamily \@title \par}%
      {\em\LARGE\py@HeaderFamily \py@release\releaseinfo \par}
      \vfill
      {\LARGE\py@HeaderFamily \@author \par}
      \vfill\vfill
      {\large
       \@date \par
       \vfill
       \py@authoraddress \par
      }%
    \end{flushright}%\par
    \@thanks
  \end{titlepage}%
  \setcounter{footnote}{0}%
  \let\thanks\relax\let\maketitle\relax
  \gdef\@thanks{}\gdef\@author{}\gdef\@title{}
}


% Catch the end of the {abstract} environment, but here make sure the
% abstract is followed by a blank page if the 'openright' option is used.
%
\let\py@OldEndAbstract=\endabstract
\renewcommand{\endabstract}{
  \if@openright
    \ifodd\value{page}
      \typeout{Adding blank page after the abstract.}
      \vfil\pagebreak
    \fi
  \fi
  \py@OldEndAbstract
}

% This wraps the \tableofcontents macro with all the magic to get the
% spacing right and have the right number of pages if the 'openright'
% option has been used.  This eliminates a fair amount of crud in the
% individual document files.
%
\let\py@OldTableofcontents=\tableofcontents
\renewcommand{\tableofcontents}{%
  \setcounter{page}{1}%
  \pagebreak%
  \pagestyle{plain}%
  {%
    \parskip = 0mm%
    \py@OldTableofcontents%
    \if@openright%
      \ifodd\value{page}%
        \typeout{Adding blank page after the table of contents.}%
        \pagebreak\hspace{0pt}%
      \fi%
    \fi%
    \cleardoublepage%
  }%
  \pagenumbering{arabic}%
  \@ifundefined{fancyhf}{}{\pagestyle{normal}}%
  \py@doing@page@targetstrue%
}
% This is needed to get the width of the section # area wide enough in the
% library reference.  Doing it here keeps it the same for all the manuals.
%
\renewcommand*\l@section{\@dottedtocline{1}{1.5em}{2.6em}}
\renewcommand*\l@subsection{\@dottedtocline{2}{4.1em}{3.5em}}
\setcounter{tocdepth}{1}


% Fix the theindex environment to add an entry to the Table of
% Contents; this is much nicer than just having to jump to the end of
% the book and flip around, especially with multiple indexes.
%
\let\py@OldTheindex=\theindex
\renewcommand{\theindex}{
  \cleardoublepage
  \py@OldTheindex
  \addcontentsline{toc}{chapter}{\indexname}
}
