# Generated automatically from Makefile.in by configure.
VERSION = NEMO V3.0 1-apr-01 PJT
################################################################################
# 	$(NEMO)/Makefile: top level Makefile for full NEMO installation
################################################################################
# The following subdirectories (1&2) are needed for installation, although only
# some may have been part of the tar file that was imported. 
# The 2nd set of files need to exist, and must have world write permission :-(
# They are always architecture dependant files, that never get exported and
# created during a new installation.

# relative to $(NEMO)
CHECKDIR1 = data inc news tmp csh \
            man man/man1 man/man3 man/man5 man/man8 man/manl \
            demo etc adm bin lib obj \
	    local \
            adm/import adm/export adm/install adm/submit adm/purge 

# absolute (derived) directories & files
CHECKDIR2 = $(NEMOBIN) $(NEMOLIB) $(NEMOOBJ) \
            $(NEMOOBJ)/bodytrans $(NEMOOBJ)/potential

CHECKFIL2 = $(NEMO)/adm/Usage \
	    $(NEMO)/adm/install/mknemo.log \
	    $(NEMOOBJ)/bodytrans/BTNAMES

################################################################################
#  Top level sub-directories in NEMO that contain Makefiles for NEMO install.
#  Currently accepted are:
#	src 		- standard export
#	usr		- user contributions (not really supported anymore)
MAKEDIRS = src 

################################################################################
# The following files/directories will be saved to a tar file
# and used in incremental export tar files (Nightly/export.csh)
# SRC is to select subsections of the whole src tree
# To get all of NEMO, choose SRC=src usr (see e.g. tarkernel below)
SRC = src
USR =
#
ASCIIFILES = COPYING MANIFEST NEMORC NEMORC.gen PORTING README VERSION \
	     Makefile src/Makefile \
	     configure configure.in \
	     Makefile.in makedefs.in config.h.in makedefs.in \
             install-sh config.sub config.guess \
	     nemo.rc nemo_end \
	     man/tmac.an man/whatis man/Makefile 
ASCIIDIRS  = inc text data bugs csh demo \
             man/doc man/man? $(SRC) $(USR)

EXPORTA = $(ASCIIFILES) $(ASCIIDIRS)
EXPORTB = $(EXPORTA) $(NEMOBIN) $(NEMOLIB) $(NEMOOBJ)
CHKFILE = adm/chkfile

#	Some default macro names, they can be overridden by supplying them
#	on the commandline, e.g. "make tarfile /usr/moi/nemo.tar

TAPE   = /dev/tape
FILE   = $(NEMO)/nemo.tar
USRTAR = $(NEMO)/nemo_usr.tar
BACKUP = $(NEMO)/nemo-fullbck.tar

#
MOTD = $(NEMO)/etc/motd
TIMESTAMP=$(NEMO)/adm/TIMESTAMP

#  Master site (Maryland) specific, and probably not used anywhere else
FTPDIR = /home/ftp/progs/nemo
WWWDIR = $(NEMO)/local/www
################################################################################
# INSTALL:
#	----------Installation starts here------------
# make sure you're in $(NEMO) from here

SHELL = /bin/sh

# this should remain the first target in this Makefile
helphelp:
	@echo ""
	@echo " ### NEMO installation help menu ###"
	@echo ""
	@echo Some important environment variables to be used are:
	@echo NEMO=$(NEMO)
	@echo NEMOSITE=$(NEMOSITE)
	@echo NEMOHOST=$(NEMOHOST)
	@echo NEMOBIN=$(NEMOBIN)
	@echo NEMOLIB=$(NEMOLIB)
	@echo NEMOOBJ=$(NEMOOBJ)
	@echo FLOAT_OPTION=$(FLOAT_OPTION)
	@echo "HOSTTYPE=$(HOSTTYPE) (deprecated - see NEMOHOST above)"
	@echo SHELL=$(SHELL)
	@echo CC=$(CC)  CFLAGS=$(CFLAGS)
	@echo FC=$(FC)  FFLAGS=$(FFLAGS)
	@echo MAKE=$(MAKE) MFLAGS=$(MFLAGS)
	@echo ""
	@echo check your .cshrc file, installation manual and the README file 
	@echo in case some of the above environment variables are not defined.
	@echo \(Your system may then need special cc/make scripts!\)
	@echo ""
	@make helpme
helpme:
	@echo "Additional help targets: helpme, helpman, helpfind, helptar"

dirs: nemo_start
	@echo Checking directory structure on `date`
	@echo By `whoami` on `hostname`
	-@for i in $(CHECKDIR1) $(ASCIIDIRS); do \
	(if [ -d $(NEMO)/$$i ]; \
	then \
	    echo DIR $(NEMO)/$$i exists; \
	else \
	    echo DIR $(NEMO)/$$i created; \
	    mkdir $(NEMO)/$$i; \
	    chmod a+w $(NEMO)/$$i; \
	fi); done
	-@for i in $(CHECKDIR2); do \
	(if [ -d $$i ]; \
	then \
	    echo DIR $$i exists; \
	else \
	    echo DIR $$i created; \
	    mkdir $$i; \
	    chmod a+w $$i; \
	fi); done
	-@for i in $(CHECKFIL2); do \
	(if [ -f $$i ]; \
	then \
	    echo FILE $$i exists; \
	    chmod a+w $$i; \
	else \
	    echo FILE $$i created; \
	    touch $$i; \
	    chmod a+w $$i; \
	fi); done
	@if [ ! -f $(TIMESTAMP) ]; then \
            echo Updating $(TIMESTAMP) file on `date`; \
	    echo `date` `hostname` `whoami` > $(TIMESTAMP); \
            echo "`cat $(NEMO)/VERSION` `date +%m%d%H%M%y` `whoami` `date` (install)" >> $(CHKFILE); \
	else \
	    echo "TIMESTAMP exists:";\
	    ls -l $(TIMESTAMP);\
	fi
	@if [ ! -f $(MOTD) ]; then \
            echo Creating $(MOTD) file on `date`; \
            echo "***********************************************" > $(MOTD);\
            echo "Nemo Release 2 ($(VERSION)) installed: " >> $(MOTD);\
	    echo "  NEMOVER = $(NEMOVER)" >> $(MOTD); \
	    echo "  `date` on `whoami`@`hostname`" >> $(MOTD); \
            echo "***********************************************">> $(MOTD);\
	else \
	    echo "MOTD exists:";\
	    ls -l $(MOTD);\
	fi
	@echo "`cat VERSION` `date`" >> .version
	@if [ ! -f NEMORC.local ]; then \
            echo Copying NEMORC.gen to NEMORC.local file on `date`;\
	    cp NEMORC.gen NEMORC.local;\
	else \
	    echo "NEMORC.local exists:";\
	    ls -l NEMORC.local;\
	fi
	@echo "OK, all main directories and files should be there now"
	@echo Done at `date`

pjt:
	@if [ ! -f $(NEMOLIB)/makedefs ]; then \
	    if [ -f $(NEMO)/makedefs ]; then \
              echo Copying $(NEMO)/makedefs $(NEMOLIB);\
	      cp $(NEMO)/makedefs $(NEMOLIB); \
            else \
              echo Creating dummy  $(NEMOLIB)/makedefs;\
              touch  $(NEMOLIB)/makedefs ;\
	    fi \
	else \
	    echo "$(NEMOLIB)/makedefs exists:";\
	    ls -l $(NEMOLIB)/makedefs;\
	fi


nemo_start:
	echo '# this file has been automatically created ' > nemo_start
	echo '# by the root makefile: make nemo_start'    >> nemo_start
	echo 'if ($$?NEMO == 0) setenv NEMO' $(NEMO)      >> nemo_start
	echo 'source $$NEMO/nemo.rc'                      >> nemo_start

start:
	make nemo_start NEMO=`pwd`

alias:
	@echo "# add these lines to your .cshrc file:"
	@echo "alias nemo 'setenv NEMO "$(NEMO)" ; source $$NEMO/nemo_start'"

# fake a hosttype for current NEMO: (usage: make fakehost FAKEHOST=alias)
FAKEHOST=unknown
fakehost:
	@echo "Creating dummy (bin,lib,obj) link for FAKEHOST=$(FAKEHOST)"
	ln -s $(NEMOBIN) $(NEMO)/bin/$(FAKEHOST)
	ln -s $(NEMOLIB) $(NEMO)/lib/$(FAKEHOST)
	ln -s $(NEMOOBJ) $(NEMO)/obj/$(FAKEHOST)


OS = $(NEMOHOST)

# 

#SCRIPTS = cc f77 make ranlib	## this was NEMO V2
SCRIPTS = 

scripts:
	@echo Installing scripts on `date` 
	@echo By `whoami` on `hostname`
	-@for i in $(SCRIPTS); do \
		(cd $(NEMO)/src/scripts; \
		echo "Available are: " ; ls $$i.* ; \
		$(MAKE) $$i OS=$(OS)); \
		( echo "hashed..." ; hash $(NEMOBIN)/$$i ); \
		echo "$$i will now be: `which $$i`" ; done
	(cd $(NEMO)/src/scripts; make install)

# The next four targets are not currently supported 
all:	nemo_lib nemo_bin

nemo_lib:
	@echo Make all subdirectories on `date`
	@echo By `whoami` on `hostname`
	-@for i in ${MAKEDIRS}; do \
		(cd $$i; echo `date` MAKE NEMO_LIB in `pwd`; \
		$(MAKE) nemo_lib); done
	@echo NEMO is installed in $(NEMO)
	@echo all done `date`

nemo_bin:
	-@for i in ${MAKEDIRS}; do \
		(cd $$i; echo `date` MAKE NEMO_BIN in `pwd`; \
		$(MAKE) nemo_bin); done
	@echo all done `date`

nemo_src:
	-@for i in ${MAKEDIRS}; do \
	(cd $$i; $(MAKE) nemo_src); done


#
##############################################################################


##############################################################################
# FIND utilities
#
FINDDIR=src

helpfind:
	@echo "Some expert targets here are (FINDDIR=$(FINDDIR)) :"
	@echo "new:         show all new files since CHKFILE=$(CHKFILE)"
	@echo "new_patch:   show all new files since last patch $(PATCHTAR)"
	@echo "clobber:     delete all .o and .a files"
	@echo "findbad:     find where all .[aio] files under NEMO/src are"
	@echo "findexec:    find where all the executables are"
	@echo "findold:     find where backup files are"
	@echo "timestamp:   list all files newer than TIMESTAMP"
	@echo "timestamp_ls:list all files newer than TIMESTAMP in ls -l format"

new:
	-@for i in $(EXPORTA); do \
	(find $$i -newer $(CHKFILE) -type f  \! -name '.[a-z,A-Z]*' -print); done

newtar:
	@echo Slow process, using CHKFILE=$(CHKFILE)
	@touch tmp/newtar;tar cvf new.tar tmp/newtar
	-@for i in $(EXPORTA); do \
	(find $$i -newer $(CHKFILE) -type f -exec tar rvf new.tar '{}' \;); done
	@rm tmp/newtar

clobber:
	@echo "Deleting all .a and .o files - sleeping 5 seconds first"
	sleep 5
	find $(FINDDIR) -type f \( -name '*.[oa]' \) -print -exec rm '{}' \;

findbad:
	find $(FINDDIR) -type f \( -name '*.[aio]' -o -name \*.def -o -name \*.log \) -print

finddot:
	find $(FINDDIR) -type f \( -name .\* \) -print

findold:
	find $(FINDDIR) -name \*~ -print


findexec:
	@echo "Showing where executable files live..."
	@find . -type f -perm -0111 -print

timestamp:
	@echo "Showing new file since last local timestamp:"
	@ls -l $(TIMESTAMP)
	@find . -type f -newer $(TIMESTAMP) -print

timestamp_ls:
	@echo "Showing new file since last local timestamp:"
	@ls -l $(TIMESTAMP)
	@find . -type f -newer $(TIMESTAMP) -exec ls -l '{}' \;


##############################################################################
# EXPORT

helptar:
	@echo "Install tar targets:"
	@echo "  % make tartape [TAPE=$(TAPE)]"
	@echo "  % make tarfile [FILE=$(FILE)]"
	@echo "  % make tarbackup [BACKUP=$(BACKUP)]"
	@echo "  % make purge"
	@echo "  % make ftp"
	@echo "  % make usrftp [USR=$(USR)]"
	@echo "  % make tarkernel"
	@echo "  % make tarbima"
	@echo "  % make tarorbit"
	@echo ""

tardu:
	@echo ""
	@echo $(EXPORTA)
	@echo "" 
	@echo Let me show you the space this will take:
	@du -s $(EXPORTA) | tee /tmp/nemo.du
	@echo " ### Total blocksize:";
	@awk 'BEGIN{n=0} {n=n+$1} END{print n}' /tmp/nemo.du
	@echo A third possibility is to use 'make backup' for a full tar image
	@echo Check with: 'make help' to see default devices.

tarfile:
	@echo NEMO=$(NEMO)
	@echo A tar file FILE=$(FILE) will be created.
	@echo -n Hang on ...
	tar -cf $(FILE) $(EXPORTA)
	@echo ... All done!

# autoconf/CVS based export
#

NEMOVER = `cat VERSION`
DIST_DIR = nemo_$(NEMOVER)

dist:
	rm -rf $(DIST_DIR)
	cvs -q export -D tomorrow -d $(DIST_DIR) nemo 2>&1 > /tmp/nemodist.log
	tar -zcf $(DIST_DIR).tar.gz $(DIST_DIR)
	rm -rf $(DIST_DIR)


# The following 'ftp' targets only supposed to work at the master site
# Warning:  (COMPRESS) .gz extensions have been hardcoded here

COMPRESS = gzip -f
NEMOTAR=nemo_`cat $(NEMO)/VERSION`.tar
PATCHTAR=patch_`cat $(NEMO)/VERSION`.tar

export:
	@echo "Steps to update NEMO:"
	@echo "make new                         to see what's new for ftp"
	@echo "make ftp_patch                   export a new patch (can redo)"
	@echo "make new_patch                   show new since last patch"
	@echo "make ftp                         full  compressed (src) export"
	@echo "src/scripts/version inc          increase patch level"
	@echo "  or:"
	@echo "src/scripts/version set MA.MI.PA set new Major/Minor/Patch"
	@echo "FTPDIR=$(FTPDIR)"
	@echo "NEMOTAR=$(NEMOTAR)"


usrftp:
	@echo Creating usr tarfile for USR=$(USR)
	@make tarfile EXPORTA=$(USR) FILE=$(FTPDIR)/$(USRTAR)

ftp:
	@echo Working from FTPDIR=$(FTPDIR) :;df -k $(FTPDIR)
	@make tarfile "SRC=$(SRC)" "USR=$(USR)" FILE=$(FTPDIR)/$(NEMOTAR)
	@echo Compressing
	@$(COMPRESS) $(FTPDIR)/$(NEMOTAR)
	@cp VERSION src/scripts/bootstrap README $(FTPDIR)
	@(cd $(FTPDIR); rm nemo.tar.gz; ln -s $(NEMOTAR).gz nemo.tar.gz)
	@ls -l $(FTPDIR)
	@echo "<A HREF=ftp://ftp.astro.umd.edu/pub/nemo/$(NEMOTAR).gz> $(NEMOTAR).gz </A>"  > $(WWWDIR)/lastftp
	@echo "(`date`)" >> $(WWWDIR)/lastftp
#	@(cd $(FTPDIR) ; ls -l $(NEMOTAR).gz > $(WWWDIR)/lastftp)
#	@echo `ls -l $(FTPDIR)/$(NEMOTAR).gz` > $(WWWDIR)/lastftp
#	@echo "Last updated on `date` by `whoami`" > $(WWWDIR)/lastftp

#		only do this when you really are done with "ftp"
stamp:
	@echo "`cat $(NEMO)/VERSION` `date +%m%d%H%M%y` `whoami` `date`" >> $(CHKFILE)
	@tail -1 $(CHKFILE)
	@echo You should probably now increment the version:
	@echo "   src/scripts/version inc"

ftp_patch:
	tar cvf $(FTPDIR)/$(PATCHTAR) `make new CHKFILE=$(CHKFILE)`
	@echo "<A HREF=ftp://ftp.astro.umd.edu/pub/nemo/$(PATCHTAR)> $(PATCHTAR) </A>"  > $(WWWDIR)/lastpatch
	@echo "(`date`)" >> $(WWWDIR)/lastpatch

new_patch:
	@make new CHKFILE=$(FTPDIR)/$(PATCHTAR)

new_time:
	@echo "`cat $(NEMO)/VERSION` `date +%m%d%H%M%y` `whoami` `date`" >> $(CHKFILE)
	@tail -1 $(CHKFILE)

test1:
	@echo "<A HREF=ftp://ftp.astro.umd.edu/pub/nemo/$(PATCHTAR)> $(PATCHTAR) </A>"  > $(WWWDIR)/lastpatch
	@echo "(`date`)" >> $(WWWDIR)/lastpatch
test2:
	@echo "<A HREF=ftp://ftp.astro.umd.edu/pub/nemo/$(NEMOTAR).gz> $(NEMOTAR).gz </A>"  > $(WWWDIR)/lastftp
	@echo "(`date`)" >> $(WWWDIR)/lastftp

new_ftp:
	-@for i in $(EXPORTA); do \
	(find $$i -newer $(FTPDIR)/$(NEMOTAR).gz -type f -print); done


tarkernel:
	@echo KERNEL+SCRIPTS only for small bootstrap
	@make tarfile ASCIIDIRS="inc src/kernel src/scripts" FILE=kernel.tar
	@$(COMPRESS) kernel.tar
	@echo All done
	@ls -l kernel.tar*

tarnbody:
	@echo NBODY only 
	@make tarfile ASCIIDIRS="src/nbody" FILE=nbody.tar
	@$(COMPRESS) nbody.tar
	@echo All done
	@ls -l nbody.tar*

tarorbit:
	@echo ORBIT only 
	@make tarfile ASCIIDIRS="src/orbit" FILE=orbit.tar
	@$(COMPRESS) orbit.tar
	@echo All done
	@ls -l orbit.tar*

tarman:
	@echo MAN only 
	@make tarfile ASCIIDIRS= FILE=man.tar \
           ASCIIFILES="man/man? man/Makefile man/tmac.an man/w*"
	@$(COMPRESS) man.tar
	@echo All done
	@ls -l man.tar*

tarbima: 
	make tarfile FILE=checker.tar \
	 "ASCIIDIRS=inc usr/bima\
	  src/Makefile src/kernel/Makefile \
	  src/kernel/io src/kernel/core src/kernel/misc src/scripts \
	  src/tools/unproto"



tartape:
	@echo NEMO=$(NEMO) 
	@echo You must be logged in on tape host machine to do this
	@tar -coFFvf $(TAPE) $(EXPORTA)

tarfileb:
	@echo NEMO=$(NEMO)
	@echo A binary tar file $(FILE) will be silently created for you.
	@echo -n Hang on ...
	@tar -coFFf $(FILE) $(EXPORTB)
	@echo ... All done!

tartapeb:
	@echo NEMO=$(NEMO) 
	@echo You must be logged in on tape host machine to do this
	@tar -coFFvf $(TAPE) $(EXPORTB)

# silent backup of whole image

tarbackup:
	@echo NEMO=$(NEMO)
	@echo Creating big tar backup of whole NEMO on $(BACKUP)
	@tar -cf $(BACKUP) $(NEMO)

purge:
	@(if [ -f  adm/purge/`date +%d%h%y`.tar ]; \
	then \
		echo "Backup already done today (check your makefile)"; \
	else \
		echo "Creating list of old version files in /tmp/list.bck:" ; \
		find . -name '.*.*[0-9]' -print > /tmp/list.bck ; \
		echo "Found `wc -l /tmp/list.bck` in there." ; \
		echo "Creating backup file adm/purge/`date +%d%h%y`.tar";\
		tar cf adm/purge/`date +%d%h%y`.tar `cat /tmp/list.bck`; \
		echo "deleting old version files now..." ;\
		find . -name '.*.*[0-9]' -exec rm '{}' \; ; \
	fi);


##############################################################################
#	Machine non-NEMO targets, mostly for specific machines

ds:
	@echo "Installing DS...needs libnemo.a"
	@if test -f $(NEMOLIB)/libnemo.a ; \
	then \
	   cd $(NEMO)/src/tools/ds ; \
           grep NEMO conf.h ;\
	   $(MAKE) ds EXTRALIBS=$(NEMOLIB)/libnemo.a ; \
	   mv ds $(NEMOBIN) ; \
	   $(MAKE) clean ; \
        else \
	   echo "Library libnemo.a is absent." ; \
	fi

movie:
	@echo "Installing MOVIEs for a SUN3/4..."
	@echo "### [movie compiled with /bin/cc ]"
	(cd $(NEMO)/src/tools/movie; \
	 $(MAKE) movie movie_sv CC=/bin/cc; \
	 mv movie movie_sv $(NEMOBIN) )
	@echo "...done"

miriad:
	@echo "Installing miriad shells..."
	(cd $(NEMO)/src/tools/miriad/miriad;\
	 $(MAKE) miriad ; mv miriad $(NEMOBIN); make clean)
	(cd $(NEMO)/src/tools/miriad/mirtool;\
	 $(MAKE) nemotool ; mv nemotool $(NEMOBIN); make clean)

makeindex:
	@echo "Installing makeindex utility for LaTeX manuals"
	(cd $(NEMO)/src/tools/makeindex;\
	 make;mv indexsrc/index $(NEMOBIN)/makeindex;make clean)
#	END


#	non-advertised help target

OSS = sun3 sun4 mf sun3_old sun4_old sparc

install:
	@echo "There is no official install from the root menu yet"
	@echo "see also src/scripts/bootstrap how to bootstrap NEMO"
	@echo ""
	@echo "However, typically you would do the following: (must be in (t)csh)"
	@echo '      ./configure        -- run configure, with lots of options'
	@echo '      setenv NEMO `pwd`  -- define NEMO'
	@echo "      source NEMORC      -- this is a bootstrap, but we need it here"
	@echo "      make dirs          -- creates directory structure"
	@echo "      make config_extra  -- copy configure created files into dirs"
	@echo "      make scripts       -- destribute basic scripts to NEMOBIN"
	@echo '      set path=(. $$NEMOBIN $$path); rehash'
	@echo '            you may want to adjust NEMORC.local now,and then:'
	@echo '      source NEMORC.local'
	@echo "      cd src;make install"

helpconfig:
	@echo config, configure, config_extra, config_clean

config:	configure
	./configure
	cp config.h makedefs $(NEMOLIB)

configure:	configure.in
	autoconf

config_extra:
	@echo DEPRECATED: should now call config

#	useful if you want to start with a clean slate
config_clean:
	rm -f $(CONFIG_CLEAN)

CLEAN = bin lib obj adm/TIMESTAMP etc/motd $(CONFIG_CLEAN)

CONFIG_CLEAN = config.h makedefs config.cache config.log config.status

clean:
	@echo There is no clean, cleanall would delete the following files and directories:
	@echo There is also a config_clean, which only removes:
	@echo
	@echo $(CONFIG_CLEAN)
	@echo
	@echo '"make cleanall" would remove' $(CLEAN)
	@echo
	@echo $(CLEAN)

cleanall:
	rm -rf $(CLEAN)

cleansrc:
	(cd src; make clean)


sun3 sun4 sparc:	sun

sun:
	@echo Starting at `date`
	(cd src;make install)
	@echo Done at `date`

