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
sub do_cmd_Cpp{ join('', 'C++', @_[0]); }
sub do_cmd_EOF{ join('', 'EOF', @_[0]); }

sub do_cmd_e{ local($_) = @_; '&#92;' . $_; }

sub do_cmd_optional{
	local($_) = @_;
	s/$any_next_pair_pr_rx/<\/VAR><BIG>\[<\/BIG><VAR>\2<\/VAR><BIG>\]<\/BIG><VAR>/;
	$_;
}

sub do_cmd_varvars{
	local($_) = @_;
	s/$any_next_pair_pr_rx/<VAR>\2<\/VAR>/;
	$_;
}

# texinfo-like formatting commands: \code{...} etc.

sub do_cmd_code{
	local($_) = @_;
	s/$any_next_pair_pr_rx/<CODE>\2<\/CODE>/;
	$_;
}

sub do_cmd_sectcode{ &do_cmd_code(@_); }

sub do_cmd_kbd{
	local($_) = @_;
	s/$any_next_pair_pr_rx/<KBD>\2<\/KBD>/;
	$_;
}

sub do_cmd_key{
	local($_) = @_;
	s/$any_next_pair_pr_rx/<TT>\2<\/TT>/;
	$_;
}

sub do_cmd_samp{
	local($_) = @_;
	s/$any_next_pair_pr_rx/`<SAMP>\2<\/SAMP>'/;
	$_;
}

sub do_cmd_var{
	local($_) = @_;
	s/$any_next_pair_pr_rx/<VAR>\2<\/VAR>/;
	$_;
}

sub do_cmd_file{
	local($_) = @_;
	s/$any_next_pair_pr_rx/`<CODE>\2<\/CODE>'/;
	$_;
}

sub do_cmd_dfn{
	local($_) = @_;
	s/$any_next_pair_pr_rx/<I><DFN>\2<\/DFN><\/I>/;
	$_;
}

sub do_cmd_emph{
	local($_) = @_;
	s/$any_next_pair_pr_rx/<EM>\2<\/EM>/;
	$_;
}

sub do_cmd_strong{
	local($_) = @_;
	s/$any_next_pair_pr_rx/<STRONG>\2<\/STRONG>/;
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
	&make_index_entry($br_id, "$str ($word)");
	$_;
}

sub do_cmd_bifuncindex{ &my_parword_index_helper('built-in function', @_); }
sub do_cmd_bimodindex{ &my_parword_index_helper('built-in module', @_); }
sub do_cmd_stmodindex{ &my_parword_index_helper('standard module', @_); }
sub do_cmd_bifuncindex{ &my_parword_index_helper('standard module', @_); }

sub do_cmd_nodename{ &do_cmd_label(@_); }

$any_next_pair_rx3 = "$O(\\d+)$C([\\s\\S]*)$O\\3$C";
$new_command{"indexsubitem"} = "";

sub get_indexsubitem{
  local($result) = $new_command{"indexsubitem"};
  #print "\nget_indexsubitem ==> $result\n";
  $result;
}

sub do_env_cfuncdesc{
  local($_) = @_;
  local($return_type,$function_name,$arg_list) = ('', '', '');
  local($cfuncdesc_rx) =
    "$next_pair_rx$any_next_pair_rx3$any_next_pair_rx5";
  $* = 1;
  if (/$cfuncdesc_rx/o) {
    $return_type = "$2";
    $function_name = "$4";
    $arg_list = "$6";
    &make_index_entry($3,"<TT>$function_name</TT> " . &get_indexsubitem);
  }
  $* = 0;
  "<DL><DT>$return_type <STRONG><A NAME=\"$3\">$function_name</A></STRONG>" .
    "(<VAR>$arg_list</VAR>)\n<DD>$'\n</DL>"
}

sub do_env_ctypedesc{
  local($_) = @_;
  local($type_name) = ('');
  local($cfuncdesc_rx) =
    "$next_pair_rx";
  $* = 1;
  if (/$cfuncdesc_rx/o) {
    $type_name = "$2";
    &make_index_entry($1,"<TT>$var_name</TT> " . &get_indexsubitem);
  }
  $* = 0;
  "<DL><DT><STRONG><A NAME=\"$1\">$type_name</A></STRONG>\n<DD>$'\n</DL>"
}

sub do_env_cvardesc{
  local($_) = @_;
  local($var_type,$var_name) = ('', '');
  local($cfuncdesc_rx) =
    "$next_pair_rx$any_next_pair_rx3";
  $* = 1;
  if (/$cfuncdesc_rx/o) {
    $var_type = "$2";
    $var_name = "$4";
    &make_index_entry($3,"<TT>$var_name</TT> " . &get_indexsubitem);
  }
  $* = 0;
  "<DL><DT>$var_type <STRONG><A NAME=\"$3\">$var_name</A></STRONG>\n" .
    "<DD>$'\n</DL>"
}

sub do_env_funcdesc{
  local($_) = @_;
  local($function_name,$arg_list) = ('', '');
  local($funcdesc_rx) = "$next_pair_rx$any_next_pair_rx3";
  $* = 1;
  if (/$funcdesc_rx/o) {
    $function_name = "$2";
    $arg_list = "$4";
    &make_index_entry($1,"<TT>$function_name</TT> " . &get_indexsubitem);
  }
  $* = 0;
  "<DL><DT><STRONG><A NAME=\"$3\">$function_name</A></STRONG>" .
    "(<VAR>$arg_list</VAR>)\n<DD>$'\n</DL>"
}

sub do_env_datadesc{
  local($_) = @_;
  local($data_name) = ('', '');
  local($datadesc_rx) = "$next_pair_rx";
  $* = 1;
  if (/$datadesc_rx/o) {
    $data_name = "$2";
    &make_index_entry($1,"<TT>$data_name</TT> " . &get_indexsubitem);
  }
  $* = 0;
  "<DL><DT><STRONG><A NAME=\"$3\">$data_name</A></STRONG>" .
    "\n<DD>$'\n</DL>"
}

sub do_env_excdesc{ &do_env_datadesc(@_); }

sub do_env_seealso{
  local($_) = @_;
  "<P><B>See Also:</B></P>\n" . $_;
}

sub do_cmd_seemodule{
  local($_) = @_;
  local($any_next_pair_pr_rx3) = "$OP(\\d+)$CP([\\s\\S]*)$OP\\3$CP";
  s/$next_pair_pr_rx$any_next_pair_pr_rx3/<P><CODE><B>\2<\/B><\/CODE> (\4)<\/P>/;
  $_;
}

sub do_cmd_seetext{
  local($_) = @_;
  "<p>" . $_;
}

1;				# This must be the last line
