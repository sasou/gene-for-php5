/*
  +----------------------------------------------------------------------+
  | gene                                                                 |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sasou  <admin@caophp.com>                                    |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_API.h"
#include "zend_exceptions.h"


#include "php_gene.h"
#include "gene_view.h"
#include "gene_load.h"

zend_class_entry * gene_view_ce;


/** {{{ gene_file_modified
*/
int gene_view_display(char *file TSRMLS_DC)
{
	char *path;
	int path_len;
	if (GENE_G(app_root) && GENE_G(app_ext)) {
		path_len = spprintf(&path, 0, "%s/%s/%s%s", GENE_G(app_root), GENE_G(app_view), file, GENE_G(app_ext));
	} else {
		path_len = spprintf(&path, 0, "%s/%s/%s%s", GENE_G(app_root), GENE_VIEW_VIEW, file, GENE_VIEW_EXT);
	}
	gene_load_import(path TSRMLS_CC);
	return 1;
}
/* }}} */


/*
 * {{{ gene_view
 */
PHP_METHOD(gene_view, __construct)
{
	long debug = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|l", &debug) == FAILURE)
    {
        RETURN_NULL();
    }
}
/* }}} */



/** {{{ public gene_view::display(string $file)
*/
PHP_METHOD(gene_view, display) {
  char  *file;
  int  file_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &file, &file_len) == FAILURE) {
    return;
  }
  if (file_len) {
	  gene_view_display(file TSRMLS_CC);
  }
}
/* }}} */


/*
 * {{{ gene_view_methods
 */
zend_function_entry gene_view_methods[] = {
		PHP_ME(gene_view, display, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_view, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
		{NULL, NULL, NULL}
};
/* }}} */


/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(view)
{
    zend_class_entry gene_view;
    INIT_CLASS_ENTRY(gene_view,"gene_view",gene_view_methods);
    gene_view_ce = zend_register_internal_class(&gene_view TSRMLS_CC);

    //
	return SUCCESS;
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
