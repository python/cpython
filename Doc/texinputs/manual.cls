%
% manual.cls for the Python documentation
%

\NeedsTeXFormat{LaTeX2e}[1995/12/01]
\ProvidesClass{manual}
             [1998/03/03 Document class (Python manual)]


% Change the options here to get a different set of basic options,  This
% is where to add things like "a4paper" or "10pt".
%
\LoadClass[twoside,openright]{report}


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
% The "times" package makes the default font the PostScript Times
% font, which makes for smaller PostScript and a font that more people 
% like.
%
\RequirePackage{fancyhdr}\typeout{Using fancier footers than usual.}
\RequirePackage{times}\typeout{Using times fonts instead of Computer Modern.}


% Required packages:
%
% The "fncychap" package is used to get the nice chapter headers.  The
% .sty file is distributed with Python, so you should not need to disable
% it.  You'd also end up with a mixed page style; uglier than stock LaTeX!
%
\RequirePackage[Bjarne]{fncychap}\typeout{Using fancy chapter headings.}
% Do horizontal rules it this way to match:
\newcommand{\@doHorizontalRule}{\mghrulefill{\RW}}
%
%
% This gives us all the Python-specific markup that we really want.
% This should come last.  Do not change this.
%
\RequirePackage{python}


% Change the title page to look a bit better, and fit in with the
% fncychap ``Bjarne'' style a bit better.
%
\renewcommand{\maketitle}{%
  \begin{titlepage}%
    \let\footnotesize\small
    \let\footnoterule\relax
    \@doHorizontalRule%
    \@ifundefined{pdfinfo}{}{
      \pdfinfo{
        /Author (\@author)
        /Title (\@title)
      }
    }
    \begin{flushright}%
      {\rm\Huge\HeaderFamily \@title \par}%
      {\em\LARGE\HeaderFamily \@release \par}
      \vfill
      {\LARGE\HeaderFamily \@author \par}
      \vfill\vfill
      {\large
       \@date \par
       \vfill
       \@authoraddress \par
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
\let\OldEndAbstract=\endabstract
\renewcommand{\endabstract}{
  \if@openright
    \ifodd\value{page}
      \typeout{Adding blank page after the abstract.}
      \vfil\pagebreak
    \fi
  \fi
  \OldEndAbstract
}

% This wraps the \tableofcontents macro with all the magic to get the
% spacing right and have the right number of pages if the 'openright'
% option has been used.  This eliminates a fair amount of crud in the
% individual document files.
%
\let\OldTableofcontents=\tableofcontents
\renewcommand{\tableofcontents}{%
  \setcounter{page}{1}%
  \pagebreak%
  \pagestyle{plain}%
  {%
    \parskip = 0mm%
    \OldTableofcontents%
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
  \@doing@page@targetstrue%
}


% Fix the theindex environment to add an entry to the Table of
% Contents; this is much nicer than just having to jump to the end of
% the book and flip around, especially with multiple indexes.
%
\let\OldTheindex=\theindex
\renewcommand{\theindex}{
  \cleardoublepage
  \OldTheindex
  \addcontentsline{toc}{chapter}{\indexname}
}
