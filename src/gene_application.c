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
  | Author: Sasou  <admin@php-gene.com> web:www.php-gene.com             |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "standard/php_filestat.h"
#include "main/SAPI.h"
#include "Zend/zend_API.h"
#include "zend_exceptions.h"


#include "php_gene.h"
#include "gene_application.h"
#include "gene_load.h"
#include "gene_cache.h"

zend_class_entry * gene_application_ce;

/*
 * {{{ void load_file(char *php_script, int php_script_len, int validity TSRMLS_DC)
 */
void load_file(char *php_script, int php_script_len, int validity TSRMLS_DC)
{
	int import = 0,cur,times = 0;
	gene_cache_container_easy *val = NULL;
	if (php_script_len) {
		val =  gene_cache_get_easy(php_script, php_script_len TSRMLS_CC);
		if (val) {
			cur = time(NULL);
			times = cur - val->stime;
			if (times > val->validity) {
				val->stime = cur;
				cur = gene_file_modified(php_script,0 TSRMLS_CC);
				if (cur != val->ftime) {
					import = 1;
					val->ftime = cur;
				}
			}
		} else {
			import = 1;
			gene_cache_set_easy(php_script, php_script_len,gene_file_modified(php_script,0 TSRMLS_CC),validity TSRMLS_CC);
		}
		val = NULL;
		if (import) {
			gene_load_import(php_script);
		}
	}
}
/* }}} */

/** {{{ gene_file_modified
*/
int gene_file_modified(char *file, long ctime TSRMLS_DC)
{
	zval  n_ctime;
	php_stat(file, strlen(file), 6 /* FS_MTIME */ , &n_ctime TSRMLS_CC);
	if (Z_TYPE(n_ctime) != IS_BOOL && ctime != Z_LVAL(n_ctime)) {
		return Z_LVAL(n_ctime);
	}
	return 0;
}
/* }}} */

/*
 * {{{ gene_application
 */
PHP_METHOD(gene_application, __construct)
{
	zval *safe = NULL;
	int len = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|z", &safe) == FAILURE)
    {
        RETURN_NULL();
    }
    if (safe && !GENE_G(app_key)) {
    	GENE_G(app_key) = estrndup(Z_STRVAL_P(safe), Z_STRLEN_P(safe));
    }
}
/* }}} */


/*
 * {{{ public gene_application::load($key,$validity)
 */
PHP_METHOD(gene_application, load)
{
	zval *self = getThis();
	char *php_script;
	int php_script_len = 0,validity = 10;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sl", &php_script, &php_script_len, &validity) == FAILURE) {
		return;
	}
	load_file(php_script, php_script_len, validity TSRMLS_CC);
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_application::run($method,$path)
 */
PHP_METHOD(gene_application, run)
{
	char *methodin = NULL,*pathin = NULL;
	int methodlen,pathlen;
	zval *self = getThis(),*safe = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|ss", &methodin, &methodlen,&pathin, &pathlen) == FAILURE)
    {
        RETURN_NULL();
    }
    if (GENE_G(app_key)) {
    	MAKE_STD_ZVAL(safe);
    	ZVAL_STRING(safe,GENE_G(app_key),1);
    }
	get_router_content_run(methodin,pathin,safe TSRMLS_CC);
	if (safe) {
		zval_ptr_dtor(&safe);
		safe = NULL;
	}
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ gene_application_methods
 */
zend_function_entry gene_application_methods[] = {
		PHP_ME(gene_application, load, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_application, run, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_application, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
		{NULL, NULL, NULL}
};
/* }}} */


/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(application)
{
    zend_class_entry gene_application;
    INIT_CLASS_ENTRY(gene_application,"gene_application",gene_application_methods);
    gene_application_ce = zend_register_internal_class(&gene_application TSRMLS_CC);

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
