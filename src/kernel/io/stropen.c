/* stropen(), strdelete(), strname(), strseek()
 *
 * STROPEN: open a STDIO stream much like fopen does, with these
 * additional features: 
 * (1) existing files cannot be opend for writing unless 
 *     mode = "w!" (mode = "a" is also permitted),
 * (2) names of form "-" map to stdin/stdout, depending on mode,
 * (3) names of the form "-num" for some small num set up a
 *     stream to read/write file descriptor num.
 * (4) a mode "s" is a scratch-file - will be deleted when
 *     strclose() is called
 *
 * fopen() itself officially recognizes the following modes:
 *         r, w, a, r+, w+, a+
 *
 * Portability:   stat(2) needs <sys/types.h> and <sys/stat.h> 
 *
 *	When a stream is eventually used by filestruct(3NEMO) - an
 *	internal table is maintained for that file.
 *      stropen() is also maintaining a similar table, keeping
 *      track of original file
 *	a similar table such that the original filename can be
 *	retrieved in case of fatal errors in I/O sections of
 *	filestruct(3NEMO)
 *
 *	(c) 1986     Created in the dark ages -        Joshua Barnes
 *	xx-oct-90    Added the 's' scratchfile option   Peter Teuben
 *      12-oct-91    strdelete returns 0 or 1, using NEMO_MAXFD  PJT
 *      25-feb-92    happy gcc2.0				 pjt
 *      19-may-92    added strseek() to aid filestruct           pjt
 *                   and repaired erroneous warning in strdelete
 *	20-feb-94    ansi
 *	 2-jul-94    correctly recognize '.' as a /dev/null sink	pjt
 *	12-apr-95    no more ARGS 
 *	22-mar-00    open scratch file in 'w' mode, not 'w+' and ensure
 *		     it does not exist yet				pjt
 *	28-nov-00    casted fdopen so compilers don't complain
 */
#include <sys/stat.h>
#include <stdinc.h>
#include <getparam.h>
#include <strlib.h>
#include <sys/types.h>
#include <sys/stat.h>

extern int unlink (string);		/* POSIX ??? */
extern int dup (int);			/* POSIX ??? */

#if !defined(NEMO_MAXFD)
#define NEMO_MAXFD  32
#endif

local struct {	          /*  our internal filetable for stropen/strdelete */
    char *name;
    stream str;
    bool scratch;
    bool seek;		/* flag to denote if you can seek; it also denotes deletability */
} ftable[NEMO_MAXFD];



/* stropen:
 *       name:   can also ne "-", or "-num" 
 *	 mode:   "r"ead, "w"rite, "w!"rite on!, "a"ppend 
 */


stream stropen(string name,string mode)		
{
    bool inflag;
    int fds;
    char tempname[128];   /* buffer for temp name in case of scratch file */
    stream res;
    struct stat buf;

    inflag = streq(mode, "r");
    if (name[0] == '-') {		/* see if '-' or '-num' special file */
        if (streq(mode,"s")) 
            error("stropen: no scratch mode allowed in %s",name);
	if (streq(name, "-")) {
	    fds = dup(fileno(inflag ? stdin : stdout));
	    if (fds == -1)
		error("stropen: cannot dup %s", inflag ? "stdin" : "stdout");
	} else {
	    fds = atoi(&name[1]);
        }
	res = (stream ) fdopen(fds, streq(mode, "w!") ? "w" : mode);
	if (res == NULL)
	    error("stropen: cannot open f.d. %d for %s\n",
		  fds, inflag ? "input" : "output");
        fds=fileno(res);
        if (fds>=NEMO_MAXFD) 
            error("stropen: file descriptor too large for ftable. fds=%d",fds);
        ftable[fds].name = scopy(name);
        ftable[fds].str = res;
        ftable[fds].scratch = FALSE;
        ftable[fds].seek = FALSE;
    } else {                                    /* regular file */
        strncpy(tempname,name,128);	/* MAXPATHLEN ??? */
        if (streq(mode,"s")) {          /* scratch mode */
            if (*name != '/') {     /* ignore name: make a new one */
                strcpy(tempname,"/tmp/scrNemo.XXXXXX");
                mktemp(tempname);
            } 
            if (stat(tempname,&buf)==0)
                error("stropen: scratch file \"%s\" already exists", tempname);
            res = fopen(tempname,"w");
            if (res==NULL) 
                error("stropen: cannot open scratch file \"%s\"",tempname);
        } else {                    /* "r" or "w" mode */
            if (streq(mode, "w") &&
		!streq(name,".") &&
		stat(tempname, &buf) == 0)
                error("stropen: file \"%s\" already exists\n", tempname);
            if (streq(name,"."))
            	res = fopen("/dev/null", "w!");
            else
            	res = fopen(tempname, streq(mode, "w!") ? "w" : mode);
            if (res == NULL)
                error("stropen: cannot open file \"%s\" for %s\n",
                     tempname, inflag ? "input" : "output");
        }
        fds=fileno(res);
        if (fds>=NEMO_MAXFD) 
            error("stropen: file descriptor too large for ftable. fds=%d",fds);
        ftable[fds].name = scopy(tempname);
        ftable[fds].str = res;
        ftable[fds].scratch = streq(mode,"s");
        ftable[fds].seek = TRUE;		/* this may not be true ??? */
    }
    return res;
}

/*
 *  STRDELETE:   delete a file, associated with a stream, and free our
 *               table entry. Called by strclose().
 *  Inputs:
 *	str	    stream to look for
 *	scratch	    if TRUE always delete it, if FALSE, only
 *                  delete it when 'str' was a scratch file
 *  Return value:   1 if all OK, 0 if no filename match found
 *                  or some error in trying to delete the file.
 *  Bug: this routine will complain about previously opened redirected
 *       files
 */
int strdelete( stream str, bool scratch)
{
    int i,  retval=1;

    for (i=0; i<NEMO_MAXFD; i++) {    /* check all entries */
        if (str == ftable[i].str) {         /* if match found */
            if (ftable[i].name == NULL) 
                error("strdelete: no file name");
            if (scratch || ftable[i].scratch) {
                dprintf(1,"Deleting scratch file %s\n",ftable[i].name);
                if (unlink(ftable[i].name) != 0) {
                    retval = 0;
                    warning("strdelete: could not delete %s\n",ftable[i].name);
                }
            } 
            free(ftable[i].name);        /* free all associated extra space */
            ftable[i].name = NULL;
            ftable[i].str = NULL;
            return retval;
        }
    }
    warning("strdelete: No matching file found in ftable");
    return retval;
}

/* 
 * STRNAME:     return name of file associated with a stream
 *
 *      Warning: returns pointer to static area 
 */

string strname(stream str)
{
    int i;

    for (i=0; i<NEMO_MAXFD; i++)     /* check all entries */
        if (str == ftable[i].str) return(ftable[i].name);
    return(NULL);
}

/* 
 * STRSEEK:     return seeakability of file associated with a stream
 *
 */

bool strseek(stream str)
{
    int i;

    for (i=0; i<NEMO_MAXFD; i++)     /* check all entries */
        if (str == ftable[i].str) return ftable[i].seek;
    error("Bad search in strseek");
    return FALSE;	/* Never Reached */
}


#ifdef TESTBED

#include <getparam.h>

string defv[] = {
    "name=foo.bar\n	Example filename to process",
    "mode=w\n		r(read),w(write),w!(write-on),a(append),s(scratch)",
    "text=boo hoo foo\n Text to write to file if w/a-option",
    "delete=f\n         Try and strdelete it?",
    "VERSION=1.2\n      19-may-92 PJT",
    NULL,
};

nemo_main()
{
    string name, mode, text;
    stream str;
    char buf[128];

    name = getparam("name");
    mode = getparam("mode");
    text = getparam("text");

    str = stropen(name, mode);
    dprintf(0,"%s has strname -> %s\n", name, strname(str));
    dprintf(0,"%s has strseek -> %d\n", name, strseek(str)?1:0);
    if (streq(mode, "r")) {
	while (fgets(buf, 127, str) != NULL)
	    printf("%s", buf);
    } else {
	sprintf(buf, "%s\n", text);
	fputs(buf, str);
    }
    if (getbparam("delete"))
        strdelete(str,TRUE);
    else
        strclose(str);
}

#endif
