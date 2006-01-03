/******************************************************************************/
/*                                                                            */
/*  ZLIB                                                                      */
/*                                                                            */
/*    Compile sources into modules and link them into a service program.      */
/*                                                                            */
/******************************************************************************/

             PGM

/*      Configuration adjustable parameters.                                  */

             DCL        VAR(&SRCLIB) TYPE(*CHAR) LEN(10) +
                          VALUE('ZLIB')                         /* Source library. */
             DCL        VAR(&SRCFILE) TYPE(*CHAR) LEN(10) +
                          VALUE('SOURCES')                      /* Source member file. */
             DCL        VAR(&CTLFILE) TYPE(*CHAR) LEN(10) +
                          VALUE('TOOLS')                        /* Control member file. */

             DCL        VAR(&MODLIB) TYPE(*CHAR) LEN(10) +
                          VALUE('ZLIB')                         /* Module library. */

             DCL        VAR(&SRVLIB) TYPE(*CHAR) LEN(10) +
                          VALUE('LGPL')                         /* Service program library. */

             DCL        VAR(&CFLAGS) TYPE(*CHAR) +
                          VALUE('OPTIMIZE(40)')                 /* Compile options. */


/*      Working storage.                                                      */

             DCL        VAR(&CMDLEN) TYPE(*DEC) LEN(15 5) VALUE(300)    /* Command length. */
             DCL        VAR(&CMD) TYPE(*CHAR) LEN(512)


/*      Compile sources into modules.                                         */

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/ADLER32)               SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/COMPRESS)              SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/CRC32)                 SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/DEFLATE)               SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/GZIO)                  SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/INFBACK)               SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/INFFAST)               SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/INFLATE)               SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/INFTREES)              SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/TREES)                 SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/UNCOMPR)               SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)

             CHGVAR     VAR(&CMD) VALUE('CRTCMOD MODULE(' *TCAT &MODLIB *TCAT  +
                        '/ZUTIL)                 SRCFILE(' *TCAT               +
                        &SRCLIB *TCAT '/' *TCAT &SRCFILE *TCAT                 +
                        ') SYSIFCOPT(*IFSIO)' *BCAT &CFLAGS)
             CALL       PGM(QCMDEXC) PARM(&CMD &CMDLEN)


/*      Link modules into a service program.                                  */

             CRTSRVPGM  SRVPGM(&SRVLIB/ZLIB) +
                          MODULE(&MODLIB/ADLER32     &MODLIB/COMPRESS    +
                                 &MODLIB/CRC32       &MODLIB/DEFLATE     +
                                 &MODLIB/GZIO        &MODLIB/INFBACK     +
                                 &MODLIB/INFFAST     &MODLIB/INFLATE     +
                                 &MODLIB/INFTREES    &MODLIB/TREES       +
                                 &MODLIB/UNCOMPR     &MODLIB/ZUTIL)      +
                          SRCFILE(&SRCLIB/&CTLFILE) SRCMBR(BNDSRC) +
                          TEXT('ZLIB 1.2.3') TGTRLS(V4R4M0)

             ENDPGM
