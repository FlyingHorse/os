# $FreeBSD$

PJDFSTEST_SRCDIR=	${.CURDIR:H:H:H:H}/contrib/pjdfstest

TESTSDIR=	${TESTSBASE}/sys/pjdfstest

.PATH: ${.CURDIR}

FILESDIR=	${TESTSDIR}
FILES=		conf
FILES+=		misc.sh
CLEANFILES+=	misc.sh misc.sh.tmp

misc.sh: ${PJDFSTEST_SRCDIR}/tests/misc.sh
	sed -e '\,maindir=,s,maindir=.*,maindir=`dirname $$0`/../,' < ${.ALLSRC} > ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

afterinstall: install-tests-symlink
install-tests-symlink: .PHONY
	rm -f ${DESTDIR}${TESTSDIR}/tests
	${INSTALL_SYMLINK} . ${DESTDIR}${TESTSDIR}/tests

TESTS_SUBDIRS=	chflags
TESTS_SUBDIRS+=	chmod
TESTS_SUBDIRS+=	chown
TESTS_SUBDIRS+=	ftruncate
TESTS_SUBDIRS+=	granular
TESTS_SUBDIRS+=	link
TESTS_SUBDIRS+=	mkdir
TESTS_SUBDIRS+=	mkfifo
TESTS_SUBDIRS+=	mknod
TESTS_SUBDIRS+=	open
TESTS_SUBDIRS+=	rename
TESTS_SUBDIRS+=	rmdir
TESTS_SUBDIRS+=	symlink
TESTS_SUBDIRS+=	truncate
TESTS_SUBDIRS+=	unlink

SUBDIR_PARALLEL=

.include <bsd.test.mk>
