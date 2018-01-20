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
#include "main/SAPI.h"
#include "Zend/zend_API.h"
#include "zend_exceptions.h"

#include "php_gene.h"
#include "gene_application.h"
#include "gene_controller.h"
#include "gene_request.h"
#include "gene_response.h"
#include "gene_cache.h"
#include "gene_router.h"
#include "gene_view.h"

zend_class_entry * gene_controller_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(gene_controller_void_arginfo, 0, 0, 0) ZEND_END_ARG_INFO()
/* }}} */

/*
 * {{{ gene_controller
 */
PHP_METHOD(gene_controller, __construct) {
	int debug = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &debug) == FAILURE) {
		RETURN_NULL();
	}
	gene_ini_router(TSRMLS_C);
	RETURN_NULL()
	;
}
/* }}} */

/** {{{ public gene_controller::get(mixed $name, mixed $default = NULL)
 */
GENE_REQUEST_METHOD(gene_controller, get, TRACK_VARS_GET);
/* }}} */

/** {{{ public gene_controller::post(mixed $name, mixed $default = NULL)
 */
GENE_REQUEST_METHOD(gene_controller, post, TRACK_VARS_POST);
/* }}} */

/** {{{ public gene_controller::request(mixed $name, mixed $default = NULL)
 */
GENE_REQUEST_METHOD(gene_controller, request, TRACK_VARS_REQUEST);
/* }}} */

/** {{{ public gene_controller::files(mixed $name, mixed $default = NULL)
 */
GENE_REQUEST_METHOD(gene_controller, files, TRACK_VARS_FILES);
/* }}} */

/** {{{ public gene_controller::cookie(mixed $name, mixed $default = NULL)
 */
GENE_REQUEST_METHOD(gene_controller, cookie, TRACK_VARS_COOKIE);
/* }}} */

/** {{{ public gene_controller::server(mixed $name, mixed $default = NULL)
 */
GENE_REQUEST_METHOD(gene_controller, server, TRACK_VARS_SERVER);
/* }}} */

/** {{{ public gene_controller::env(mixed $name, mixed $default = NULL)
 */
GENE_REQUEST_METHOD(gene_controller, env, TRACK_VARS_ENV);
/* }}} */

/** {{{ public gene_controller::isGet(void)
 */
GENE_REQUEST_IS_METHOD(gene_controller, Get);
/* }}} */

/** {{{ public gene_controller::isPost(void)
 */
GENE_REQUEST_IS_METHOD(gene_controller, Post);
/* }}} */

/** {{{ public gene_controller::isPut(void)
 */
GENE_REQUEST_IS_METHOD(gene_controller, Put);
/* }}} */

/** {{{ public gene_controller::isHead(void)
 */
GENE_REQUEST_IS_METHOD(gene_controller, Head);
/* }}} */

/** {{{ public gene_controller::isOptions(void)
 */
GENE_REQUEST_IS_METHOD(gene_controller, Options);
/* }}} */

/** {{{ public gene_controller::isOptions(void)
 */
GENE_REQUEST_IS_METHOD(gene_controller, Delete);
/* }}} */

/** {{{ public gene_controller::isCli(void)
 */
GENE_REQUEST_IS_METHOD(gene_controller, Cli);
/* }}} */

/** {{{ proto public gene_controller::isAjax()
 */
PHP_METHOD(gene_controller, isAjax) {
	zval * header = request_query(TRACK_VARS_SERVER,
			ZEND_STRL("HTTP_X_REQUESTED_WITH") TSRMLS_CC);
	if (Z_TYPE_P(header) == IS_STRING
			&& strncasecmp("XMLHttpRequest", Z_STRVAL_P(header),
					Z_STRLEN_P(header)) == 0) {
		zval_ptr_dtor(&header);
		RETURN_TRUE
		;
	}
	zval_ptr_dtor(&header);
	RETURN_FALSE
	;
}
/* }}} */

/*
 * {{{ public gene_controller::getMethod()
 */
PHP_METHOD(gene_controller, getMethod) {
	if (GENE_G(method)) {
		RETURN_STRING(GENE_G(method), 1);
	}
	RETURN_NULL()
	;
}
/* }}} */

/*
 * {{{ public gene_controller::urlParams()
 */
PHP_METHOD(gene_controller, urlParams) {
	zval **ret = NULL, **val = NULL;
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

/** {{{ public gene_response::redirect(string $url)
 */
PHP_METHOD(gene_controller, redirect) {
	char *url;
	int url_len;
	long code = 302;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &url, &url_len) == FAILURE) {
		RETURN_NULL();
	}

	if (!url_len) {
		RETURN_NULL();
	}

	RETURN_BOOL(gene_response_set_redirect(url, code TSRMLS_CC));
}
/* }}} */

/** {{{ public gene_controller::display(string $file)
 */
PHP_METHOD(gene_controller, display) {
	char *file;
	int file_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &file, &file_len) == FAILURE) {
		RETURN_NULL();
	}
	if (file_len) {
		gene_view_display(file TSRMLS_CC);
	}
}
/* }}} */

/** {{{ public gene_controller::display(string $file)
 */
PHP_METHOD(gene_controller, displayExt) {
	char *file = NULL, *parent_file = NULL;
	int file_len = 0, parent_file_len = 0;
	zend_bool isCompile = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sb", &file, &file_len, &parent_file, &parent_file_len, &isCompile) == FAILURE) {
		return;
	}
	if (parent_file_len) {
		if (GENE_G(child_views)) {
			efree(GENE_G(child_views));
			GENE_G(child_views) = NULL;
		}
		GENE_G(child_views) = estrndup(file, file_len);
		gene_view_display_ext(parent_file, isCompile TSRMLS_CC);
	} else {
		gene_view_display_ext(file, isCompile TSRMLS_CC);
	}
	RETURN_NULL();
}
/* }}} */

/** {{{ public gene_controller::contains(string $file)
 */
PHP_METHOD(gene_controller, contains) {
	gene_view_display_ext(GENE_G(child_views), 0 TSRMLS_CC);
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ gene_controller_methods
 */
zend_function_entry gene_controller_methods[] = {
	PHP_ME(gene_controller, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, request, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, post, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, cookie, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, files, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, server, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, env, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, isAjax, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, urlParams, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, getMethod, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, isGet, gene_controller_void_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, isPost, gene_controller_void_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, isPut, gene_controller_void_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, isHead, gene_controller_void_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, isOptions, gene_controller_void_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, isDelete, gene_controller_void_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, isCli, gene_controller_void_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, redirect, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_controller, display, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_controller, displayExt, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_controller, contains, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_controller, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	{ NULL, NULL, NULL }
};
/* }}} */

/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(controller) {
	zend_class_entry gene_controller;
	GENE_INIT_CLASS_ENTRY(gene_controller, "Gene_Controller",
			"Gene\\Controller", gene_controller_methods);
	gene_controller_ce = zend_register_internal_class(
			&gene_controller TSRMLS_CC);

	//debug
	//zend_declare_property_null(gene_application_ce, GENE_EXECUTE_DEBUG, strlen(GENE_EXECUTE_DEBUG), ZEND_ACC_PUBLIC TSRMLS_CC);
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
