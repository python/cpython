/*
 *
 * This is a simple module to allow the 
 * user to compile and execute an applescript
 * which is passed in as a text item.
 *
 *  Sean Hummel <seanh@prognet.com>
 *  1/20/98
 *  RealNetworks
 *
 *  Jay Painter <jpaint@serv.net> <jpaint@gimp.org> <jpaint@real.com>
 *  
 *
 */

#include <Resources.h>
#include <Files.h>
#include <OSA.h>
#include <string.h>
#include "ScriptRunner.h"
#include <script.h>
#include <resources.h>

#ifdef TARGET_API_MAC_CARBON
static
p2cstr(StringPtr p)
{
    unsigned char *c = p;
    int len = c[0];
    strncpy((char *)c+1, (char *)c, len);
    c[len] = 0;
}

static c2pstr(const char *cc)
{
    char *c = (char *)cc; /* Ouch */
    int len = strlen(c);
    
    if ( len > 255 ) len = 255;
    strncpy(c, c+1, len);
    c[0] = len;
}
#endif

OSAError LoadScriptingComponent (ComponentInstance * scriptingComponent);


/*
 * store the script as a compile script so that OSA 
 * components may load and execute the script easily
 */
OSAError 
CompileAndSave (const char *text, 
		const char *outfile,
		OSAActiveUPP proc, 
		AEDesc * result)
{

  OSAError err2 = 0;
  AEDesc theScript;
  OSAID compiledScriptID = 0;
  ComponentInstance scriptingComponent;
  FSSpec outfilespec;
  AEDesc theCompiledScript;
  OSAID scriptid = kOSANullScript;
  short saveres = 0;



  /* Initialize theScript here because it is a struct */
  theScript.dataHandle = NULL;
  theCompiledScript.dataHandle = NULL;


  /* open the component manager */
  err2 = LoadScriptingComponent (&scriptingComponent);
  if (err2)
    return err2;		/* <<< Fail quietly?? */


  /* construct the AppleEvent Descriptor to contain the text of script */
  AECreateDesc ('TEXT', text, strlen (text), &theScript);

  err2 = OSACompile (scriptingComponent, 
		     &theScript, 
		     kOSAModeCompileIntoContext, 
		     &scriptid);
  if (err2)
    {
      OSAScriptError (scriptingComponent, kOSAErrorMessage, 'TEXT', result);
      goto CleanUp;
    }


  err2 = OSAStore (scriptingComponent, 
		   scriptid, 
		   typeOSAGenericStorage,
		   kOSAModeCompileIntoContext, 
		   &theCompiledScript);
  if (err2)
    {
      OSAScriptError (scriptingComponent, kOSAErrorMessage, 'TEXT', result);
      goto CleanUp;
    }


  c2pstr (outfile);
  FSMakeFSSpec (0, 0, (StringPtr) outfile, &outfilespec);
  p2cstr ((StringPtr) outfile);

  FSpDelete (&outfilespec);

  FSpCreateResFile (&outfilespec, 'ToyS', 'osas', smRoman);

  saveres = CurResFile ();

  if (saveres)
    {
      short myres = 0;
      myres = FSpOpenResFile (&outfilespec, fsWrPerm);

      UseResFile (myres);
      AddResource (theCompiledScript.dataHandle, 'scpt', 128, "\p");
      CloseResFile (myres);
      UseResFile (saveres);
    }


CleanUp:

  if (theScript.dataHandle)
    AEDisposeDesc (&theScript);

  if (theCompiledScript.dataHandle)
    AEDisposeDesc (&theCompiledScript);

  if (scriptid)
    OSADispose (scriptingComponent, scriptid);

  if (scriptingComponent != 0)
    CloseComponent (scriptingComponent);


  return err2;
}


OSAError 
CompileAndExecute (const char *text,
		   AEDesc * result,
		   OSAActiveUPP proc)
{
  OSAError err2 = 0;
  AEDesc theScript;
  OSAID compiledScriptID = 0;
  ComponentInstance scriptingComponent;


  /* initialize theScript here because it is a struct */
  theScript.dataHandle = NULL;

  /* Open the component manager */
  err2 = LoadScriptingComponent (&scriptingComponent);
  if (err2)
    return err2;		/* <<< Fail quietly?? */


  /* construct the AppleEvent Descriptor to contain the text of script */
  AECreateDesc ('TEXT', text, strlen (text), &theScript);


  err2 = OSASetActiveProc (scriptingComponent, proc, NULL);
  if (err2)
    goto CleanUp;


  err2 = OSADoScript (scriptingComponent, &theScript, kOSANullScript, 'TEXT', 0, result);
  if (err2)
    {
      OSAScriptError (scriptingComponent, kOSAErrorMessage, 'TEXT', result);
      goto CleanUp;
    }


CleanUp:

  if (theScript.dataHandle)
    AEDisposeDesc (&theScript);

  if (scriptingComponent != 0)
    CloseComponent (scriptingComponent);


  return err2;
}


/*
 * This routine reads in a saved script file and executes 
 * the script contained within (from a 'scpt' resource.)
 */
OSAError 
ExecuteScriptFile (const char *theFilePath,
		   OSAActiveUPP proc,
		   AEDesc * result)
{
  OSAError err2;
  short resRefCon;
  AEDesc theScript;
  OSAID compiledScriptID, scriptResultID;
  ComponentInstance scriptingComponent;
  FSSpec theFile;


  c2pstr (theFilePath);
  FSMakeFSSpec (0, 0, (StringPtr) theFilePath, &theFile);
  p2cstr ((StringPtr) theFilePath);


  /* open a connection to the OSA */
  err2 = LoadScriptingComponent (&scriptingComponent);
  if (err2)
    return err2;		/* <<< Fail quietly?? */


  err2 = OSASetActiveProc (scriptingComponent, proc, NULL);
  if (err2)
    goto error;


  /* now, try and read in the script
   * Open the script file and get the resource
   */
  resRefCon = FSpOpenResFile (&theFile, fsRdPerm);
  if (resRefCon == -1)
    return ResError ();

  theScript.dataHandle = Get1IndResource (typeOSAGenericStorage, 1);

  if ((err2 = ResError ()) || (err2 = resNotFound, theScript.dataHandle == NULL))
    {
      CloseResFile (resRefCon);
      return err2;
    }

  theScript.descriptorType = typeOSAGenericStorage;
  DetachResource (theScript.dataHandle);
  CloseResFile (resRefCon);
  err2 = noErr;


  /* give a copy of the script to AppleScript */
  err2 = OSALoad (scriptingComponent, 
		  &theScript, 
		  0L, 
		  &compiledScriptID);
  if (err2)
    goto error;

  AEDisposeDesc (&theScript);
  theScript.dataHandle = NULL;


  err2 = OSAExecute (scriptingComponent, 
		     compiledScriptID,
		     kOSANullScript,
		     0, 
		     &scriptResultID);

  if (compiledScriptID) 
    OSAScriptError (scriptingComponent, kOSAErrorMessage, 'TEXT', result);

  if (err2)
    goto error;

  /* If there was an error, return it. If there was a result, return it. */
  (void) OSADispose (scriptingComponent, compiledScriptID);

  if (err2)
    goto error;
  else
    goto done;

error:
  if (theScript.dataHandle)
    AEDisposeDesc (&theScript);


done:


  return err2;
}


OSAError 
LoadScriptingComponent (ComponentInstance * scriptingComponent)
{
  OSAError err2;

  /* Open a connection to the Open Scripting Architecture  */
  *scriptingComponent = OpenDefaultComponent (kOSAComponentType,
					      kOSAGenericScriptingComponentSubtype);

  err2 = GetComponentInstanceError (*scriptingComponent);

  return err2;
}
