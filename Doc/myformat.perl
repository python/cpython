# myformat.perl by Guido van Rossum <guido@cwi.nl> 25 Jan 1994
#
# Extension to LaTeX2HTML for documents using myformat.sty.
# Subroutines of the form do_cmd_<name> here define translations
# for LaTeX commands \<name> defined in the corresponding .sty file.
#
# XXX Not complete: \indexii etc.; \funcitem etc.

package main; 

# \bcode and \ecode brackets around verbatim

sub do_cmd_bcode{ @_[0]; }
sub do_cmd_ecode{ @_[0]; }

# words typeset in a special way (not in HTML though)

sub do_cmd_ABC{ join('', 'ABC', @_[0]); }
sub do_cmd_UNIX{ join('', 'Unix', @_[0]); }
sub do_cmd_ASCII{ join('', 'ASCII', @_[0]); }
sub do_cmd_C{ join('', 'C', @_[0]); }
sub do_cmd_EOF{ join('', 'EOF', @_[0]); }

# texinfo-like formatting commands: \code{...} etc.

sub do_cmd_code{
	local($_) = @_;
	s/(<#[0-9]+#>)(.*)(\1)/<CODE>\2<\/CODE>/;
	$_;
}

sub do_cmd_kbd{
	local($_) = @_;
	s/(<#[0-9]+#>)(.*)(\1)/<KBD>\2<\/KBD>/;
	$_;
}

sub do_cmd_key{
	local($_) = @_;
	s/(<#[0-9]+#>)(.*)(\1)/<TT>\2<\/TT>/;
	$_;
}

sub do_cmd_samp{
	local($_) = @_;
	s/(<#[0-9]+#>)(.*)(\1)/`<SAMP>\2<\/SAMP>'/;
	$_;
}

sub do_cmd_var{
	local($_) = @_;
	s/(<#[0-9]+#>)(.*)(\1)/<VAR>\2<\/VAR>/;
	$_;
}

sub do_cmd_file{
	local($_) = @_;
	s/(<#[0-9]+#>)(.*)(\1)/`<CODE>\2<\/CODE>'/;
	$_;
}

sub do_cmd_dfn{
	local($_) = @_;
	s/(<#[0-9]+#>)(.*)(\1)/<I><DFN>\2<\/DFN><\/I>/;
	$_;
}

sub do_cmd_emph{
	local($_) = @_;
	s/(<#[0-9]+#>)(.*)(\1)/<EM>\2<\/EM>/;
	$_;
}

sub do_cmd_strong{
	local($_) = @_;
	s/(<#[0-9]+#>)(.*)(\1)/<STRONG>\2<\/STRONG>/;
	$_;
}

# index commands

sub do_cmd_indexii{
	local($_) = @_;
	s/$next_pair_pr_rx//o;
	local($br_id1, $str1) = ($1, $2);
	s/$next_pair_pr_rx//o;
	local($br_id2, $str2) = ($1, $2);
	join('', &make_index_entry($br_id1, "$str1 $str2"),
		 &make_index_entry($br_id2, "$str2, $str1"), $_);
}

sub do_cmd_indexiii{
	local($_) = @_;
	s/$next_pair_pr_rx//o;
	local($br_id1, $str1) = ($1, $2);
	s/$next_pair_pr_rx//o;
	local($br_id2, $str2) = ($1, $2);
	s/$next_pair_pr_rx//o;
	local($br_id3, $str3) = ($1, $2);
	join('', &make_index_entry($br_id1, "$str1 $str2 $str3"),
		 &make_index_entry($br_id2, "$str2 $str3, $str1"),
		 &make_index_entry($br_id3, "$str3, $str1 $str2"),
		  $_);
}

sub do_cmd_indexiv{
	local($_) = @_;
	s/$next_pair_pr_rx//o;
	local($br_id1, $str1) = ($1, $2);
	s/$next_pair_pr_rx//o;
	local($br_id2, $str2) = ($1, $2);
	s/$next_pair_pr_rx//o;
	local($br_id3, $str3) = ($1, $2);
	s/$next_pair_pr_rx//o;
	local($br_id4, $str4) = ($1, $2);
	join('', &make_index_entry($br_id1, "$str1 $str2 $str3 $str4"),
		 &make_index_entry($br_id2, "$str2 $str3 $str4, $str1"),
		 &make_index_entry($br_id3, "$str3 $str4, $str1 $str2"),
		 &make_index_entry($br_id4, "$str4, $str1 $str2 $str3"),
		  $_);
}

sub do_cmd_ttindex{
	&do_cmd_index(@_);
}

sub my_typed_index_helper{
	local($word, $_) = @_;
	s/$next_pair_pr_rx//o;
	local($br_id, $str) = ($1, $2);
	join('', &make_index_entry($br_id, "$str $word"),
		 &make_index_entry($br_id, "$word, $str"), $_);
}

sub do_cmd_stindex{ &my_typed_index_helper('statement', @_); }
sub do_cmd_kwindex{ &my_typed_index_helper('keyword', @_); }
sub do_cmd_opindex{ &my_typed_index_helper('operator', @_); }
sub do_cmd_exindex{ &my_typed_index_helper('exception', @_); }
sub do_cmd_obindex{ &my_typed_index_helper('object', @_); }

sub my_parword_index_helper{
	local($word, $_) = @_;
	s/$next_pair_pr_rx//o;
	local($br_id, $str) = ($1, $2);
	join('', &make_index_entry($br_id, "$str ($word)"), $_);
}

sub do_cmd_bifuncindex{ &my_parword_index_helper('built-in function', @_); }
sub do_cmd_bimodindex{ &my_parword_index_helper('built-in module', @_); }
sub do_cmd_bifuncindex{ &my_parword_index_helper('standard module', @_); }

1;				# This must be the last line
