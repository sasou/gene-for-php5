dnl $Id$
dnl config.m4 for extension gene

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(gene, for gene support,
dnl Make sure that the comment is aligned:
dnl [  --with-gene             Include gene support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(gene, whether to enable gene support,
[  --enable-gene           Enable gene support])

if test "$PHP_GENE" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-gene -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/gene.h"  # you most likely want to change this
  dnl if test -r $PHP_GENE/$SEARCH_FOR; then # path given as parameter
  dnl   GENE_DIR=$PHP_GENE
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for gene files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       GENE_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$GENE_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the gene distribution])
  dnl fi

  dnl # --with-gene -> add include path
  dnl PHP_ADD_INCLUDE($GENE_DIR/include)

  dnl # --with-gene -> check for lib and symbol presence
  dnl LIBNAME=gene # you may want to change this
  dnl LIBSYMBOL=gene # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $GENE_DIR/lib, GENE_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_GENELIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong gene lib version or lib not found])
  dnl ],[
  dnl   -L$GENE_DIR/lib -lm
  dnl ])
  dnl
  PHP_SUBST(GENE_SHARED_LIBADD)
  PHP_NEW_EXTENSION(gene, gene.c gene_application.c gene_load.c gene_router.c gene_execute.c gene_cache.c gene_common.c gene_config.c, $ext_shared)
fi
