prog='
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*tp_dealloc\*/\)$|\1(destructor)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*tp_print\*/\)$|\1(printfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*tp_getattr\*/\)$|\1(getattrfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*tp_setattr\*/\)$|\1(setattrfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*tp_compare\*/\)$|\1(cmpfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*tp_repr\*/\)$|\1(reprfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*tp_hash\*/\)$|\1(hashfunc)\2 \3|

s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*sq_length\*/\)$|\1(inquiry)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*sq_concat\*/\)$|\1(binaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*sq_repeat\*/\)$|\1(intargfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*sq_item\*/\)$|\1(intargfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*sq_slice\*/\)$|\1(intintargfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*sq_ass_item\*/\)$|\1(intobjargproc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*sq_ass_slice\*/\)$|\1(intintobjargproc)\2 \3|

s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*mp_length\*/\)$|\1(inquiry)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*mp_subscript\*/\)$|\1(binaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*mp_ass_subscript\*/\)$|\1(objobjargproc)\2 \3|

s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_nonzero*\*/\)$|\1(inquiry)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_coerce*\*/\)$|\1(coercion)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_negative*\*/\)$|\1(unaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_positive*\*/\)$|\1(unaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_absolute*\*/\)$|\1(unaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_invert*\*/\)$|\1(unaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_int*\*/\)$|\1(unaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_long*\*/\)$|\1(unaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_float*\*/\)$|\1(unaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_oct*\*/\)$|\1(unaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_hex*\*/\)$|\1(unaryfunc)\2 \3|
s|^\([ 	]*\)\([a-z_]*,\)[ 	]*\(/\*nb_[a-z]*\*/\)$|\1(binaryfunc)\2 \3|

'
for file
do
	sed -e "$prog" $file >$file.new || break
	if cmp -s $file $file.new
	then
		echo $file unchanged; rm $file.new
	else
		echo $file UPDATED
		mv $file $file~
		mv $file.new $file
	fi
done
