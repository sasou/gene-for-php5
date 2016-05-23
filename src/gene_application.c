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
#include "gene_config.h"
#include "gene_router.h"
#include "gene_common.h"

zend_class_entry * gene_application_ce;

/*
 * {{{ void load_file(char *key, int key_len,char *php_script, int validity TSRMLS_DC)
 */
void load_file(char *key, int key_len,char *php_script, int validity TSRMLS_DC)
{
	int import = 0,cur,times = 0;
	gene_cache_container_easy *val = NULL;
	if (key_len) {
		val =  gene_cache_get_easy(key, key_len TSRMLS_CC);
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
			gene_cache_set_easy(key, key_len,gene_file_modified(php_script,0 TSRMLS_CC),validity TSRMLS_CC);
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


/** {{{ void gene_ini_router(TSRMLS_DC)
*/
void gene_ini_router(TSRMLS_DC)
{
	zval *server = NULL,** temp;
	if (!GENE_G(method) && !GENE_G(path)) {
		server = request_query(TRACK_VARS_SERVER, NULL, 0 TSRMLS_CC);
		if (server) {
	    	if (zend_hash_find(HASH_OF(server), "REQUEST_METHOD", 15, (void **)&temp) == SUCCESS) {
	    		GENE_G(method) = estrndup(Z_STRVAL_PP(temp), Z_STRLEN_PP(temp));
	    		strtolower(GENE_G(method));
	    	}
	    	if (zend_hash_find(HASH_OF(server), "REQUEST_URI", 12, (void **)&temp) == SUCCESS) {
	    		GENE_G(path) = ecalloc(Z_STRLEN_PP(temp)+1,sizeof(char));
	    		leftByChar(GENE_G(path),Z_STRVAL_PP(temp), '?');
	    	}
		}
		server = NULL;
	}
}
/* }}} */

/** {{{ void gene_ini_router(TSRMLS_DC)
*/
void gene_router_set_uri(zval **leaf TSRMLS_DC)
{
	zval **key;
	if (zend_hash_find((*leaf)->value.ht, "key", 4, (void **)&key) == SUCCESS){
		if (Z_STRLEN_PP(key)) {
			if (GENE_G(router_path)){
			    if (GENE_G(router_path)) {
			    	efree(GENE_G(router_path));
			    	GENE_G(router_path) = NULL;
			    }
			}
			GENE_G(router_path) =  estrndup(Z_STRVAL_PP(key), Z_STRLEN_PP(key));
		}
	}
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
	char *php_script,*router_e;
	int php_script_len = 0,validity = 10,router_e_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sl", &php_script, &php_script_len, &validity) == FAILURE) {
		return;
	}
	if (GENE_G(app_key)) {
		router_e_len = spprintf(&router_e, 0, "%s:%s", GENE_G(app_key), php_script);
	} else {
		router_e_len = spprintf(&router_e, 0, ":%s", php_script);
	}
	load_file(router_e, router_e_len,php_script, validity TSRMLS_CC);
	efree(router_e);
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_application::urlParams()
 */
PHP_METHOD(gene_application, urlParams)
{
	zval *cache = NULL;
	int keyString_len;
	char *keyString = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &keyString, &keyString_len) == FAILURE) {
		return;
	}
    cache = gene_cache_get_by_config(PHP_GENE_URL_PARAMS, strlen(PHP_GENE_URL_PARAMS), keyString TSRMLS_CC);
    if (cache) {
    	RETURN_ZVAL(cache, 1, 1);
    }
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::getMethod()
 */
PHP_METHOD(gene_application, getMethod)
{
	if (GENE_G(method)) {
		RETURN_STRING(GENE_G(method),1);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::getPath()
 */
PHP_METHOD(gene_application, getPath)
{
	if (GENE_G(path)) {
		RETURN_STRING(GENE_G(path),1);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::getRouterUri()
 */
PHP_METHOD(gene_application, getRouterUri)
{
	if (GENE_G(router_path)) {
		RETURN_STRING(GENE_G(router_path),1);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::config()
 */
PHP_METHOD(gene_application, config)
{
	zval *cache = NULL;
	int router_e_len,keyString_len;
	char *router_e,*keyString;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &keyString, &keyString_len) == FAILURE) {
		return;
	}

	if (GENE_G(app_key)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", GENE_G(app_key), GENE_CONFIG_CACHE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_CONFIG_CACHE);
	}
    cache = gene_cache_get_by_config(router_e, router_e_len, keyString TSRMLS_CC);
    efree(router_e);
    if (cache) {
    	RETURN_ZVAL(cache, 1, 1);
    }
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_load::autoload($key)
 */
PHP_METHOD(gene_application, autoload)
{
	int fileName_len = 0,directory_len = 0;
	char *fileName = NULL,*directory = NULL;
	zval *self = getThis();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ss", &directory, &directory_len, &fileName, &fileName_len) == FAILURE) {
		return;
	}
	if (directory_len >0 ) {
	    if (!GENE_G(directory)) {
	    	GENE_G(directory) = estrndup(directory, directory_len);
	    }
	}
	if (fileName_len >0 ) {
		gene_loader_register_function(fileName TSRMLS_CC);
	} else {
		gene_loader_register_function(NULL TSRMLS_CC);
	}
    RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_application::run($method,$path)
 */
PHP_METHOD(gene_application, run)
{
	char *methodin = NULL,*pathin = NULL;
	int methodlen = 0,pathlen = 0;
	zval *self = getThis(),*safe = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|ss", &methodin, &methodlen,&pathin, &pathlen) == FAILURE)
    {
        RETURN_NULL();
    }
    if (methodin == NULL && pathin == NULL) {
    	gene_ini_router(TSRMLS_CC);
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
		PHP_ME(gene_application, autoload, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_application, run, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_application, urlParams, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_application, getMethod, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_application, getPath, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_application, getRouterUri, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_application, config, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
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
