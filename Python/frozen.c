/* In order to test the support for frozen modules, by default we
   define a single frozen module, __hello__.  Loading it will print
   some famous words... */

static unsigned char M___hello__[] = {
	99,0,0,0,0,0,0,115,15,0,0,0,127,0,0,127,
	1,0,100,0,0,71,72,100,1,0,83,40,2,0,0,0,
	115,14,0,0,0,72,101,108,108,111,32,119,111,114,108,100,
	46,46,46,78,40,0,0,0,0,40,0,0,0,0,115,8,
	0,0,0,104,101,108,108,111,46,112,121,115,1,0,0,0,
	63,
};

struct frozen {
	char *name;
	unsigned char *code;
	int size;
} frozen_modules[] = {
	{"__hello__", M___hello__, 81},
	{0, 0, 0} /* sentinel */
};
