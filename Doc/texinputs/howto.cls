%
% howto.cls for the Python documentation
%

\NeedsTeXFormat{LaTeX2e}[1995/12/01]
\ProvidesClass{howto}
             [1998/02/25 Document class (Python HOWTO)]


% Change the options here to get a different set of basic options,  This
% is where to add things like "a4paper" or "10pt".
%
\LoadClass[twoside]{article}


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


% Required package:
%
% This gives us all the Python-specific markup that we really want.
% This should come last.  Do not change this.
%
\RequirePackage{python}


% Change the title page to look a bit better, and fit in with the
% fncychap ``Bjarne'' style a bit better.
%
\renewcommand{\maketitle}{
  \@ifundefined{ChTitleVar}{}{
    \mghrulefill{\RW}}
  \@ifundefined{pdfinfo}{}{
    \pdfinfo{
      /Author (\@author)
      /Title (\@title)
    }
  }
  \begin{flushright}
    {\rm\Huge\HeaderFamily \@title} \par
    {\em\large\HeaderFamily \@release} \par
    \vspace{25pt}
    {\Large\HeaderFamily \@author} \par
    \vspace{25pt}
    \@date \par
    \@authoraddress \par
  \end{flushright}
  \@thanks
  \setcounter{footnote}{0}
  \let\thanks\relax\let\maketitle\relax
  \gdef\@thanks{}\gdef\@author{}\gdef\@title{}
}


\let\OldTableofcontents=\tableofcontents
\renewcommand{\tableofcontents}{
  \begingroup
    \parskip = 0mm
    \OldTableofcontents
  \endgroup
  \@ifundefined{ChTitleVar}{}{
    \mghrulefill{\RW}}
  \vspace{12pt}
}  

\@ifundefined{fancyhf}{
  \pagestyle{plain}}{
  \pagestyle{normal}}		% start this way; change for
\pagenumbering{arabic}		% ToC & chapters
\setcounter{secnumdepth}{2}

\thispagestyle{empty}
