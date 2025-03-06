/* Python DTrace provider */

provider python {
    probe gc__start(int);
    probe gc__done(long);
    probe import__find__load__start(const char *);
    probe import__find__load__done(const char *, int);
    probe audit(const char *, void *);
};

#pragma D attributes Evolving/Evolving/Common provider python provider
#pragma D attributes Evolving/Evolving/Common provider python module
#pragma D attributes Evolving/Evolving/Common provider python function
#pragma D attributes Evolving/Evolving/Common provider python name
#pragma D attributes Evolving/Evolving/Common provider python args
