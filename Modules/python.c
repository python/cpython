/* Minimal main program -- everything is loaded from the library */

extern int Py_Main();

main(argc, argv)
	int argc;
	char **argv;
{
	return Py_Main(argc, argv);
}
