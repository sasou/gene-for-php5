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
#include "gene_request.h"
#include "gene_view.h"
#include "gene_common.h"
#include "gene_exception.h"

zend_class_entry * gene_application_ce;

/*
 * {{{ void load_file(char *key, int key_len,char *php_script, int validity TSRMLS_DC)
 */
void load_file(char *key, int key_len, char *php_script, int validity TSRMLS_DC) {
	int import = 0, cur, times = 0;
	gene_cache_container_easy *val = NULL;
	if (key_len) {
		val = gene_cache_get_easy(key, key_len TSRMLS_CC);
		if (val) {
			cur = time(NULL);
			times = cur - val->stime;
			if (times > val->validity) {
				val->stime = cur;
				cur = gene_file_modified(php_script, 0 TSRMLS_CC);
				if (cur != val->ftime) {
					import = 1;
					val->ftime = cur;
				}
			}
		} else {
			import = 1;
			gene_cache_set_easy(key, key_len,
					gene_file_modified(php_script, 0 TSRMLS_CC),
					validity TSRMLS_CC);
		}
		val = NULL;
		if (import) {
			gene_load_import(php_script TSRMLS_CC);
		}
	}
}
/* }}} */

/** {{{ gene_file_modified
 */
int gene_file_modified(char *file, long ctime TSRMLS_DC) {
	zval n_ctime;
	php_stat(file, strlen(file), 6 /* FS_MTIME */, &n_ctime TSRMLS_CC);
	if (Z_TYPE(n_ctime) != IS_BOOL && ctime != Z_LVAL(n_ctime)) {
		return Z_LVAL(n_ctime);
	}
	return 0;
}
/* }}} */

/** {{{ void gene_ini_router(TSRMLS_DC)
 */
void gene_ini_router(TSRMLS_D) {
	zval *server = NULL, **temp = NULL;
	if (!GENE_G(method) && !GENE_G(path) && !GENE_G(directory)) {
		server = request_query(TRACK_VARS_SERVER, NULL, 0 TSRMLS_CC);
		if (server) {
			if (zend_hash_find(HASH_OF(server), "DOCUMENT_ROOT", 14, (void **) &temp) == SUCCESS) {
				GENE_G(directory) = estrndup(Z_STRVAL_PP(temp), Z_STRLEN_PP(temp));
			}
			if (zend_hash_find(HASH_OF(server), "REQUEST_METHOD", 15, (void **) &temp) == SUCCESS) {
				GENE_G(method) = estrndup(Z_STRVAL_PP(temp), Z_STRLEN_PP(temp));
				strtolower(GENE_G(method));
			}
			if (zend_hash_find(HASH_OF(server), "REQUEST_URI", 12, (void **) &temp) == SUCCESS) {
				GENE_G(path) = ecalloc(Z_STRLEN_PP(temp)+1, sizeof(char));
				leftByChar(GENE_G(path), Z_STRVAL_PP(temp), '?');
			}
		}
		server = NULL;
		temp = NULL;
	}
}
/* }}} */

/*
 * {{{ gene_application
 */
PHP_METHOD(gene_application, __construct) {
	zval *safe = NULL;
	int len = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &safe) == FAILURE) {
		RETURN_NULL();
	}
	gene_ini_router(TSRMLS_C);
	if (safe && !GENE_G(app_key)) {
		GENE_G(app_key) = estrndup(Z_STRVAL_P(safe), Z_STRLEN_P(safe));
	}

	if (GENE_G(directory)) {
		spprintf(&GENE_G(app_root), 0, "%s/app/", GENE_G(directory));
	}

}
/* }}} */

/*
 * {{{ public gene_application::load($key,$validity)
 */
PHP_METHOD(gene_application, load) {
	zval *self = getThis();
	char *file, *path, *cache_key;
	int file_len = 0, path_len = 0, validity = 10, cache_key_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ssl", &file, &file_len, &path, &path_len, &validity) == FAILURE) {
		return;
	}
	if (path_len > 0) {
		cache_key_len = spprintf(&cache_key, 0, "%s/%s", path, file);
	} else {
		cache_key_len = spprintf(&cache_key, 0, "%sConfig/%s", GENE_G(app_root), file);
	}
	load_file(cache_key, cache_key_len, cache_key, validity TSRMLS_CC);
	efree(cache_key);
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_application::urlParams()
 */
PHP_METHOD(gene_application, urlParams) {
	zval **ret = NULL, **val= NULL;
	int keyString_len;
	char *keyString = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &keyString, &keyString_len) == FAILURE) {
		return;
	}
	if (zend_hash_find(&EG(symbol_table), "params", 7, (void **) &ret) == SUCCESS) {
		if (keyString) {
			if (zend_hash_find(Z_ARRVAL_PP(ret), keyString, keyString_len + 1, (void **) &val) == SUCCESS) {
				RETURN_ZVAL(*val, 1, 0);
			}
		}
		RETURN_ZVAL(*ret, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::getMethod()
 */
PHP_METHOD(gene_application, getMethod) {
	if (GENE_G(method)) {
		RETURN_STRING(GENE_G(method), 1);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::getPath()
 */
PHP_METHOD(gene_application, getPath) {
	if (GENE_G(path)) {
		RETURN_STRING(GENE_G(path), 1);
	}
	RETURN_NULL();
}
/* }}} */


/*
 * {{{ public gene_application::getModule()
 */
PHP_METHOD(gene_application, getModule) {
	zval **ret = NULL;
	if (zend_hash_find(&EG(symbol_table), "m", 2, (void **) &ret) == SUCCESS) {
		RETURN_ZVAL(*ret, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::getController()
 */
PHP_METHOD(gene_application, getController) {
	zval **ret = NULL;
	if (zend_hash_find(&EG(symbol_table), "c", 2, (void **) &ret) == SUCCESS) {
		RETURN_ZVAL(*ret, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::getAction()
 */
PHP_METHOD(gene_application, getAction) {
	zval **ret = NULL;
	if (zend_hash_find(&EG(symbol_table), "a", 2, (void **) &ret) == SUCCESS) {
		RETURN_ZVAL(*ret, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::getRouterUri()
 */
PHP_METHOD(gene_application, getRouterUri) {
	if (GENE_G(router_path)) {
		RETURN_STRING(GENE_G(router_path), 1);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_application::getEnvironment()
 */
PHP_METHOD(gene_application, getEnvironment) {
	RETURN_BOOL(GENE_G(run_environment));
}
/* }}} */

/*
 * {{{ public gene_application::config()
 */
PHP_METHOD(gene_application, config) {
	zval *cache = NULL;
	int router_e_len, keyString_len;
	char *router_e, *keyString;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &keyString,
			&keyString_len) == FAILURE) {
		return;
	}

	if (GENE_G(app_key)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", GENE_G(app_key), GENE_CONFIG_CACHE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s%s", GENE_G(directory), GENE_CONFIG_CACHE);
	}
	cache = gene_cache_get_by_config(router_e, router_e_len,
			keyString TSRMLS_CC);
	efree(router_e);
	if (cache) {
		RETURN_ZVAL(cache, 1, 1);
	}
	RETURN_NULL()
	;
}
/* }}} */

/*
 * {{{ public gene_load::autoload($key)
 */
PHP_METHOD(gene_application, autoload) {
	int fileName_len = 0, directory_len = 0;
	char *fileName = NULL, *app_root = NULL;
	zval *self = getThis();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ss", &app_root,
			&directory_len, &fileName, &fileName_len) == FAILURE) {
		return;
	}
	if (directory_len > 0) {
		GENE_G(app_root) = estrndup(app_root, directory_len);
	}
	if (fileName_len > 0) {
		GENE_G(auto_load_fun) = estrndup(fileName, fileName_len);
	}
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_load::error($callback, $type)
 */
PHP_METHOD(gene_application, error) {
	zval *callback = NULL, *error_type = NULL, *self = getThis();
	long type = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|lzz", &type,
			&callback, &error_type) == FAILURE) {
		return;
	}
	if (type > 0) {
		GENE_G(gene_error) = 1;
	} else {
		GENE_G(gene_error) = 0;
	}
	gene_exception_error_register(callback, error_type TSRMLS_CC);
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ public gene_exception::exception(string $callbacak[, int $error_types = E_ALL | E_STRICT ] )
 */
PHP_METHOD(gene_application, exception) {
	zval *callback = NULL, *error_type = NULL, *self = getThis();
	long type = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|lzz", &type,
			&callback, &error_type) == FAILURE) {
		return;
	}
	if (type > 0) {
		GENE_G(gene_exception) = 1;
	} else {
		GENE_G(gene_exception) = 0;
	}
	gene_exception_register(callback, error_type TSRMLS_CC);
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_load::error($callback, $type)
 */
PHP_METHOD(gene_application, setMode) {
	zval *self = getThis();
	long error_type = 0, exception_type = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll", &error_type,
			&exception_type) == FAILURE) {
		return;
	}
	if (error_type > 0) {
		GENE_G(gene_error) = 1;
	} else {
		GENE_G(gene_error) = 0;
	}
	if (exception_type > 0) {
		GENE_G(gene_exception) = 1;
	} else {
		GENE_G(gene_exception) = 0;
	}
	gene_exception_error_register(NULL, NULL TSRMLS_CC);
	gene_exception_register(NULL, NULL TSRMLS_CC);
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_application::setView($view, $ext)
 */
PHP_METHOD(gene_application, setView) {
	zval *self = getThis();
	char *view, *tpl;
	long view_len = 0, tpl_len = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ss", &view,
			&view_len, &tpl, &tpl_len) == FAILURE) {
		return;
	}
	if (view_len > 0) {
		GENE_G(app_view) = estrndup(view, view_len);
	} else {
		GENE_G(app_view) = estrndup(GENE_VIEW_VIEW, strlen(GENE_VIEW_VIEW));
	}
	if (tpl_len > 0) {
		GENE_G(app_ext) = estrndup(tpl, tpl_len);
	} else {
		GENE_G(app_ext) = estrndup(GENE_VIEW_EXT, strlen(GENE_VIEW_EXT));
	}
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_application::run($method,$path)
 */
PHP_METHOD(gene_application, run) {
	char *methodin = NULL, *pathin = NULL;
	int methodlen = 0, pathlen = 0;
	zval *self = getThis(), *safe = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ss", &methodin, &methodlen, &pathin, &pathlen) == FAILURE) {
		RETURN_NULL();
	}

	gene_loader_register_function(NULL TSRMLS_CC);

	MAKE_STD_ZVAL(safe);
	if (GENE_G(app_key)) {
		ZVAL_STRING(safe, GENE_G(app_key), 1);
	} else {
		ZVAL_STRING(safe, GENE_G(directory), 1);
	}
    init();
	get_router_content_run(methodin, pathin, safe TSRMLS_CC);
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
	PHP_ME(gene_application, setMode, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_application, setView, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_application, error, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_application, exception, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_application, run, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_application, urlParams, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_application, getMethod, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_application, getPath, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_application, getModule, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_application, getController, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_application, getAction, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_application, getRouterUri, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_application, getEnvironment, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_application, config, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_application, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	{ NULL, NULL, NULL }
};
/* }}} */

/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(application) {
	zend_class_entry gene_application;
	GENE_INIT_CLASS_ENTRY(gene_application, "Gene_Application", "Gene\\Application", gene_application_methods);
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
