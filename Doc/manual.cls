%
% manual.cls for the Python documentation
%

\NeedsTeXFormat{LaTeX2e}[1995/12/01]
\ProvidesClass{manual}
             [1998/03/03 Python manual]

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


% Required package:
%
% This gives us all the Python-specific markup that we really want.
% This should come last.  Do not change this.
%
\RequirePackage{python}
