#include "BBLMTypes.r"
#include "MacTypes.r"

#define 	kKeyWords	1057

resource	'BBLF'	(128, "Python Language Mappings", purgeable)
{
	kCurrentBBLFVersion,
	
	{
		kLanguagePython,
		(kBBLMScansFunctions|kBBLMColorsSyntax|kBBLMIsCaseSensitive),
		kKeyWords,
		"Python",
		{
			kNeitherSourceNorInclude, ".py",
		}
	}
};

#define VERSION	0x1, 0x0, final, 0x0

resource 'vers' (1) {
	VERSION,
	verUS,
	"1.1",
	"1.1,"
};

resource 'vers' (2) {
	VERSION,
	verUS,
	$$Date,
	$$Date
};
